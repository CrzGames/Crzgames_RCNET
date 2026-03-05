#include "server_context.h"
#include "server_network_snapshot_packets.h"
#include "server_debug_network_stats.h"

#include <RCNET/RCNET.h>

#include <deque>           // std::deque
#include <unordered_map>   // std::unordered_map

static void ServerNetworkOutgoingUpdate_DrainOutgoingQueue_IntoLocalDeque(
    SimulationToNetworkOUTQueue& simToNetQueue,
    std::deque<SimulationToNetworkOUTMessage>& outMessages)
{
    // 3) Drainer la queue simulation -> réseau (moins de lock)
    simToNetQueue.drain(outMessages);
}

static void ServerNetworkOutgoingUpdate_CoalesceOutgoingMessages_KeepLastPerConnectionId(
    const std::deque<SimulationToNetworkOUTMessage>& outMessages,
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastMsgPerConnectionId)
{
    // 4) COALESCING : garder uniquement le DERNIER message par client (connectionId)
    //
    // Pourquoi:
    // - Si la simulation pousse plus vite que NET OUT (ou backlog), la queue peut contenir plusieurs snapshots par client.
    // - Envoyer tous les snapshots est inutile : le client veut le dernier état.
    // - Pour delta compression plus tard, tu feras quelque chose de plus fin (ACK + history), mais ce coalescing reste utile.
    for (std::deque<SimulationToNetworkOUTMessage>::const_iterator it = outMessages.begin();
         it != outMessages.end();
         ++it)
    {
        const SimulationToNetworkOUTMessage& msg = *it;

        // Remplace l'ancien => on conserve le dernier snapshot de cette connectionId
        lastMsgPerConnectionId[msg.connectionId] = msg;
    }
}

static ENetPeer* ServerNetworkOutgoingUpdate_FindPeerForConnectionIdOrNull(
    NetworkState& networkState,
    uint32_t connectionId)
{
    // Identifier le client (ENetPeer*) à qui envoyer ce message en utilisant msg.connectionId et le mapping dans networkState
    std::unordered_map<uint32_t, ENetPeer*>::iterator pit = networkState.connectionIdToEnetPeer.find(connectionId);
    if (pit == networkState.connectionIdToEnetPeer.end())
        return nullptr;

    // ENetPeer* trouvé pour ce connectionId, envoyer le message à ce client
    ENetPeer* peer = pit->second;
    if (!peer)
        return nullptr;

    return peer;
}

static void ServerNetworkOutgoingUpdate_HandleMessageType_SnapshotFull_SendUnreliable(
    NetworkState& networkState,
    ENetPeer* peer,
    const SimulationToNetworkOUTMessage& msg)
{
    if (msg.type != SimulationToNetworkOUTMessageType::SNAPSHOT_FULL)
        return;

    if (msg.payload.size() < sizeof(SnapshotHeader))
        return;

    // 1) Trouver la session pour assigner un snapshotId "envoyé réellement"
    std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
    if (sit == networkState.sessions.end())
        return;

    ClientSession& session = sit->second;

    // 2) Lire le header (copie), patcher snapshotId, réécrire dans payload
    SnapshotHeader header{};
    std::memcpy(&header, msg.payload.data(), sizeof(SnapshotHeader));

    const uint32_t snapshotId = session.serverNextSnapshotId++;
    header.snapshotId = snapshotId;
    header.serverTimeNs = rcnet_engine_getCurrentServerTimeNsMonotonic();

    // Mettre à jour le "dernier envoyé" maintenant (car c'est VRAIMENT envoyé)
    session.serverLastSentSnapshotId = snapshotId;

    // Créer un payload patché (car msg est const)
    std::vector<uint8_t> patchedPayload = msg.payload;
    std::memcpy(patchedPayload.data(), &header, sizeof(SnapshotHeader));

    // 3) ENet send
    const enet_uint8 channelId = 2;

    ENetPacket* packet = enet_packet_create(
        patchedPayload.data(),
        patchedPayload.size(),
        0 // UNRELIABLE
    );

    if (packet)
    {
        enet_peer_send(peer, channelId, packet);
        g_dbg_snapshotsSent++;
    }

    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [NETWORK_OUT] [SNAPSHOT_FULL] - Sent snapshotId=%u to connectionId=%u (size=%zu bytes)\n",
              snapshotId,
              msg.connectionId,
              patchedPayload.size());
}

static void ServerNetworkOutgoingUpdate_SendCoalescedMessagesToAllPeers(
    NetworkState& networkState,
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastMsgPerConnectionId)
{
    // 5) Envoyer uniquement le dernier snapshot par client
    for (std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>::iterator it = lastMsgPerConnectionId.begin();
         it != lastMsgPerConnectionId.end();
         ++it)
    {
        SimulationToNetworkOUTMessage& msg = it->second;

        ENetPeer* peer = ServerNetworkOutgoingUpdate_FindPeerForConnectionIdOrNull(networkState, msg.connectionId);
        if (!peer)
            continue;

        ServerNetworkOutgoingUpdate_HandleMessageType_SnapshotFull_SendUnreliable(networkState, peer, msg);
    }
}

// ======================================================================================
// Public entry point called by server_callbacks.cpp
// ======================================================================================

void ServerNetworkOutgoingUpdate_DrainCoalesceAndSendMessages(ENetHost* host)
{
    if (host == nullptr)
        return;

    // 1) Récupérer la queue simulation -> réseau pour envoyer des messages à la fin de ce tick
    SimulationToNetworkOUTQueue& simToNetQueue = GetSimulationToNetworkOUTQueue();

    // 2) Accès au network state pour récupérer le mapping connectionId -> ENetPeer*
    NetworkState& networkState = GetNetworkState();

    // 3) Drainer la queue simulation -> réseau (moins de lock)
    std::deque<SimulationToNetworkOUTMessage> outMessages;
    ServerNetworkOutgoingUpdate_DrainOutgoingQueue_IntoLocalDeque(simToNetQueue, outMessages);

    // 4) COALESCING : garder uniquement le DERNIER message par client (connectionId)
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage> lastMsgPerConnectionId;
    ServerNetworkOutgoingUpdate_CoalesceOutgoingMessages_KeepLastPerConnectionId(outMessages, lastMsgPerConnectionId);

    // 5) Envoyer uniquement le dernier snapshot par client
    ServerNetworkOutgoingUpdate_SendCoalescedMessagesToAllPeers(networkState, lastMsgPerConnectionId);
}