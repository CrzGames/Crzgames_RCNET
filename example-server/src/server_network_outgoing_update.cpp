#include "server_context.h"
#include "server_network_packets_server_unreliable.h"
#include "server_network_packets_server_reliable.h"
#include "server_debug_network_stats.h"

#include <RCNET/RCNET.h>

#include <deque>
#include <unordered_map>
#include <cstring>

// ======================================================================================
// Helpers - 
// ======================================================================================

static void ServerNetworkOutgoingUpdate_DrainSimulationToNetworkQueue(
    SimulationToNetworkOUTQueue& simToNetQueue,
    std::deque<SimulationToNetworkOUTMessage>& outMessages)
{
    simToNetQueue.drain(outMessages);
}

// ======================================================================================
// Helpers - recherche du peer ENet par connectionId
// =====================================================================================
static ENetPeer* ServerNetworkOutgoingUpdate_FindPeerByConnectionId(
    NetworkState& networkState,
    uint32_t connectionId)
{
    std::unordered_map<uint32_t, ENetPeer*>::iterator it =
        networkState.connectionIdToEnetPeer.find(connectionId);

    if (it == networkState.connectionIdToEnetPeer.end())
        return nullptr;

    ENetPeer* peer = it->second;
    if (peer == nullptr)
        return nullptr;

    return peer;
}


// ======================================================================================
// Helpers - 
// ======================================================================================

static void ServerNetworkOutgoingUpdate_ClassifyOutgoingMessages(
    const std::deque<SimulationToNetworkOUTMessage>& outMessages,
    std::deque<SimulationToNetworkOUTMessage>& reliableMessages,
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastSnapshotPerConnectionId)
{
    for (std::deque<SimulationToNetworkOUTMessage>::const_iterator it = outMessages.begin();
         it != outMessages.end();
         ++it)
    {
        const SimulationToNetworkOUTMessage& msg = *it;

        if (msg.type == SimulationToNetworkOUTMessageType::SERVER_MATCH_INIT_PACKET_RELIABLE ||
            msg.type == SimulationToNetworkOUTMessageType::SERVER_WORLD_STATIC_STATE_INIT_PACKET_RELIABLE ||
            msg.type == SimulationToNetworkOUTMessageType::SERVER_MATCH_START_PACKET_RELIABLE)
        {
            reliableMessages.push_back(msg);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE)
        {
            // Coalescing snapshot : seul le dernier snapshot par client nous intéresse.
            lastSnapshotPerConnectionId[msg.connectionId] = msg;
        }
    }
}

// ======================================================================================
// Helpers - envoi reliable
// ======================================================================================
static void ServerNetworkOutgoingUpdate_SendReliablePacket(
    ENetPeer* peer,
    const SimulationToNetworkOUTMessage& msg,
    size_t expectedPayloadSize,
    const char* debugLabel)
{
    if (msg.payload.size() != expectedPayloadSize)
        return;

    ENetPacket* packet = enet_packet_create(
        msg.payload.data(),
        msg.payload.size(),
        ENET_PACKET_FLAG_RELIABLE
    );

    if (packet != nullptr)
    {
        enet_peer_send(peer, static_cast<enet_uint8>(NetworkChannel::GAME_RELIABLE), packet);
        RCNET_log(RCNET_LOG_INFO,
                  "[SERVER] [NETWORK_OUT] [%s] - Sent to connectionId=%u (size=%zu bytes)\n",
                  debugLabel,
                  msg.connectionId,
                  msg.payload.size());
    }
    else
    {
        RCNET_log(RCNET_LOG_ERROR,
                  "[SERVER] [NETWORK_OUT] [%s] - Failed to create ENet packet for connectionId=%u\n",
                  debugLabel,
                  msg.connectionId);
    }
}

static void ServerNetworkOutgoingUpdate_SendReliableMessages(NetworkState& networkState, const std::deque<SimulationToNetworkOUTMessage>& reliableMessages)
{
    for (std::deque<SimulationToNetworkOUTMessage>::const_iterator it = reliableMessages.begin();
         it != reliableMessages.end();
         ++it)
    {
        const SimulationToNetworkOUTMessage& msg = *it;

        ENetPeer* peer = ServerNetworkOutgoingUpdate_FindPeerByConnectionId(
            networkState,
            msg.connectionId
        );
        if (peer == nullptr)
            continue;

        if (msg.type == SimulationToNetworkOUTMessageType::SERVER_MATCH_INIT_PACKET_RELIABLE)
        {
            ServerNetworkOutgoingUpdate_SendReliablePacket(peer, msg, sizeof(ServerMatchInitPacketReliable), "MATCH_INIT_RELIABLE");
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_WORLD_STATIC_STATE_INIT_PACKET_RELIABLE)
        {
            ServerNetworkOutgoingUpdate_SendReliablePacket(peer, msg, sizeof(ServerWorldStaticStateInitPacketReliable), "WORLD_STATIC_STATE_INIT_RELIABLE");
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_MATCH_START_PACKET_RELIABLE)
        {
            ServerNetworkOutgoingUpdate_SendReliablePacket(peer, msg, sizeof(ServerMatchStartPacketReliable), "MATCH_START_RELIABLE");
        }
    }
}

// ======================================================================================
// Helpers - envoi snapshots unreliable
//
// Pour chaque client :
// - lire le ServerSnapshotFullPacketUnreliable préparé par la simulation
// - patcher snapshotId au moment de l'envoi réel
// - mettre à jour la session
// - envoyer sur le channel snapshot unreliable
// ======================================================================================

static void ServerNetworkOutgoingUpdate_SendSnapshotFullUnreliable(
    NetworkState& networkState,
    ENetPeer* peer,
    const SimulationToNetworkOUTMessage& msg)
{
    if (msg.type != SimulationToNetworkOUTMessageType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE)
        return;

    if (msg.payload.size() != sizeof(ServerSnapshotFullPacketUnreliable))
        return;

    std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
    if (sit == networkState.sessions.end())
        return;

    ClientSession& session = sit->second;

    // Lire la version construite côté simulation.
    ServerSnapshotFullPacketUnreliable snapshotPacket{};
    std::memcpy(&snapshotPacket, msg.payload.data(), sizeof(ServerSnapshotFullPacketUnreliable));

    // Assigner le snapshotId au moment de l'envoi réel.
    const uint32_t snapshotId = session.serverNextSnapshotId++;
    snapshotPacket.snapshotId = snapshotId;

    // Mémoriser le dernier snapshot réellement envoyé.
    session.serverLastSentSnapshotId = snapshotId;

    // Recréer un payload patché car le message source est const.
    std::vector<uint8_t> patchedPayload = msg.payload;
    std::memcpy(patchedPayload.data(), &snapshotPacket, sizeof(ServerSnapshotFullPacketUnreliable));

    ENetPacket* packet = enet_packet_create(
        patchedPayload.data(),
        patchedPayload.size(),
        0 // unreliable
    );

    if (packet != nullptr)
    {
        enet_peer_send(peer, static_cast<enet_uint8>(NetworkChannel::GAME_UNRELIABLE), packet);
        RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [NETWORK_OUT] [SNAPSHOT_FULL_UNRELIABLE] - Sent snapshotId=%u to connectionId=%u (size=%zu bytes)\n",
              snapshotId,
              msg.connectionId,
              patchedPayload.size());
        g_dbg_snapshotsSent++;
    }
}

static void ServerNetworkOutgoingUpdate_SendLatestSnapshots(
    NetworkState& networkState,
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastSnapshotPerConnectionId)
{
    for (std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>::iterator it =
             lastSnapshotPerConnectionId.begin();
         it != lastSnapshotPerConnectionId.end();
         ++it)
    {
        SimulationToNetworkOUTMessage& msg = it->second;

        ENetPeer* peer = ServerNetworkOutgoingUpdate_FindPeerByConnectionId(
            networkState,
            msg.connectionId
        );
        if (peer == nullptr)
            continue;

        ServerNetworkOutgoingUpdate_SendSnapshotFullUnreliable(
            networkState,
            peer,
            msg
        );
    }
}

// ======================================================================================
// Public entry point called by server_callbacks.cpp
// ======================================================================================

void ServerNetworkOutgoingUpdate_DrainCoalesceAndSendMessages(ENetHost* host)
{
    if (host == nullptr)
        return;

    SimulationToNetworkOUTQueue& simToNetQueue = GetSimulationToNetworkOUTQueue();
    NetworkState& networkState = GetNetworkState();

    // 1) Drainer tous les messages produits par la simulation depuis le dernier tick réseau OUT.
    std::deque<SimulationToNetworkOUTMessage> outMessages;
    ServerNetworkOutgoingUpdate_DrainSimulationToNetworkQueue(simToNetQueue, outMessages);

    // 2) Séparer :
    //    - les reliable : tous envoyés
    //    - les snapshots : coalescés par client
    std::deque<SimulationToNetworkOUTMessage> reliableMessages;
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage> lastSnapshotPerConnectionId;

    // Note : les messages reliable sont traités dans l'ordre de production, mais les snapshots unreliable sont coalescés pour n'envoyer que le dernier par client.
    ServerNetworkOutgoingUpdate_ClassifyOutgoingMessages(
        outMessages,
        reliableMessages,
        lastSnapshotPerConnectionId
    );

    // 3) Envoyer d'abord les messages reliable.
    ServerNetworkOutgoingUpdate_SendReliableMessages(
        networkState,
        reliableMessages
    );

    // 4) Envoyer ensuite les snapshots unreliable coalescés.
    ServerNetworkOutgoingUpdate_SendLatestSnapshots(
        networkState,
        lastSnapshotPerConnectionId
    );

    // 5) Flush explicite pour limiter la latence d'envoi.
    enet_host_flush(host);
}