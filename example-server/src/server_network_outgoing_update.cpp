#include "server_context.h"
#include "server_network_packets_server_unreliable.h"
#include "server_network_packets_server_reliable.h"
#include "server_debug_network_stats.h"
#include "server_queues.h"
#include "server_network_send_packets_server.h"

#include <RCNET/RCNET.h>

#include <deque>         // std::deque
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector
#include <cstdint>       // uint32_t, uint64_t
#include <cstring>       // std::memcpy

// ======================================================================================
// Helpers - draine la queue de messages de la simulation vers le réseau OUT
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
    // Rechercher le peer ENet correspondant à cette connectionId
    std::unordered_map<uint32_t, ENetPeer*>::iterator it = networkState.connectionIdToEnetPeer.find(connectionId);

    // Si aucune entrée n’existe pour cette connectionId, on ne peut pas poursuivre.
    if (it == networkState.connectionIdToEnetPeer.end())
    {
        // Log d’erreur : aucune correspondance trouvée pour cette connectionId.
        RCNET_log(RCNET_LOG_ERROR,
                    "[SERVER] [NETWORK_OUT] [FIND_PEER] - No ENet peer found for connectionId=%u\n",
                    connectionId);
        return nullptr;
    }

    // Récupérer le peer ENet à partir de l’itérateur.
    ENetPeer* peer = it->second;

    // Vérifier que le peer n’est pas null avant de le retourner.
    if (peer == nullptr)
    {
        RCNET_log(RCNET_LOG_ERROR,
                    "[SERVER] [NETWORK_OUT] [FIND_PEER] - ENet peer is null for connectionId=%u\n",
                    connectionId);
        return nullptr;
    }

    // Retourner le peer ENet correspondant à cette connectionId.
    return peer;
}


// ======================================================================================
// Helpers - gestion des messages sortants de la simulation vers le réseau OUT
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

        if (msg.type == SimulationToNetworkOUTMessageType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE ||
            msg.type == SimulationToNetworkOUTMessageType::SERVER_AUTH_RESPONSE_PACKET_RELIABLE ||
            msg.type == SimulationToNetworkOUTMessageType::SERVER_MATCH_INIT_PACKET_RELIABLE ||
            msg.type == SimulationToNetworkOUTMessageType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE ||
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
static void ServerNetworkOutgoingUpdate_SendReliableMessages(NetworkState& networkState, const std::deque<SimulationToNetworkOUTMessage>& reliableMessages)
{
    // Boucler sur les messages fiables à envoyer et les envoyer via ENet.
    for (std::deque<SimulationToNetworkOUTMessage>::const_iterator it = reliableMessages.begin();
         it != reliableMessages.end();
         ++it)
    {
        // Récupérer le message courant dans la boucle.
        const SimulationToNetworkOUTMessage& msg = *it;

        // Trouver le peer ENet correspondant à la connectionId du message. 
        ENetPeer* peer = ServerNetworkOutgoingUpdate_FindPeerByConnectionId(
            networkState,
            msg.connectionId
        );

        // Si aucun peer n’est trouvé pour cette connectionId, on ne peut pas envoyer ce message.
        if (peer == nullptr)
            continue;

        // En fonction du type de message fiable, appeler la fonction d’envoi correspondante.
        if (msg.type == SimulationToNetworkOUTMessageType::SERVER_MATCH_INIT_PACKET_RELIABLE)
        {
            sendServerMatchInitPacketReliable(peer, msg.serializedPacket);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_WORLD_STATIC_STATE_INIT_PACKET_RELIABLE)
        {
            sendServerWorldStaticStateInitPacketReliable(peer, msg.serializedPacket);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE)
        {
            sendServerSecureSessionHelloResponsePacketReliable(peer, msg.serializedPacket);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_AUTH_RESPONSE_PACKET_RELIABLE)
        {
            sendServerAuthResponsePacketReliable(peer, msg.serializedPacket);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_MATCH_START_PACKET_RELIABLE)
        {
            sendServerMatchStartPacketReliable(peer, msg.serializedPacket);
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

    if (msg.serializedPacket.size() != sizeof(ServerSnapshotFullPacketUnreliable))
        return;

    std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
    if (sit == networkState.sessions.end())
        return;

    ClientSession& session = sit->second;

    // Lire la version construite côté simulation.
    ServerSnapshotFullPacketUnreliable snapshotPacket{};
    std::memcpy(&snapshotPacket, msg.serializedPacket.data(), sizeof(ServerSnapshotFullPacketUnreliable));

    // Assigner le snapshotId au moment de l'envoi réel.
    const uint32_t snapshotId = session.serverNextSnapshotId++;
    snapshotPacket.snapshotId = snapshotId;

    // Mémoriser le dernier snapshot réellement envoyé.
    session.serverLastSentSnapshotId = snapshotId;

    // Recréer un payload patché car le message source est const.
    std::vector<uint8_t> patchedPayload = msg.serializedPacket;
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

    // Récupérer une référence vers la queue simulation -> réseau pour drainer les messages produits par la simulation.
    SimulationToNetworkOUTQueue& simToNetQueue = GetSimulationToNetworkOUTQueue();

    // Récupérer une référence vers le network state pour accéder aux sessions et peers.
    NetworkState& networkState = GetNetworkState();

    // Drainer tous les messages produits par la simulation depuis le dernier tick réseau OUT.
    std::deque<SimulationToNetworkOUTMessage> outMessages;
    ServerNetworkOutgoingUpdate_DrainSimulationToNetworkQueue(simToNetQueue, outMessages);

    // Séparer :
    //    - les reliable : tous envoyés
    //    - les snapshots : coalescés par client
    std::deque<SimulationToNetworkOUTMessage> reliableMessages;
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage> lastSnapshotPerConnectionId;

    // Classifyier les messages sortants de la simulation vers le réseau en messages fiables à envoyer tels quels et snapshots à coalescer.
    ServerNetworkOutgoingUpdate_ClassifyOutgoingMessages(
        outMessages,
        reliableMessages,
        lastSnapshotPerConnectionId
    );

    // Envoyer d'abord les messages reliable.
    ServerNetworkOutgoingUpdate_SendReliableMessages(
        networkState,
        reliableMessages
    );

    // Envoyer ensuite les snapshots unreliable coalescés.
    ServerNetworkOutgoingUpdate_SendLatestSnapshots(
        networkState,
        lastSnapshotPerConnectionId
    );

    // Flush explicite pour limiter la latence d'envoi.
    enet_host_flush(host);
}