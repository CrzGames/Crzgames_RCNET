#include "server_context.h"
#include "server_network_snapshot_packets.h"

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
    ENetPeer* peer,
    const SimulationToNetworkOUTMessage& msg)
{
    // Traiter le message à envoyer en fonction de son type (snapshot full, snapshot delta, event, etc.)
    if (msg.type == SimulationToNetworkOUTMessageType::SNAPSHOT_FULL)
    {
        // channel snapshots (ex: 2)
        const enet_uint8 channelId = 2;

        // Créer un ENetPacket à partir du payload du message (données du snapshot)
        ENetPacket* packet = enet_packet_create(
            msg.payload.data(),
            msg.payload.size(),
            0 // UNRELIABLE pour snapshot full
        );

        // Envoyer le packet à ce client sur le channel approprié
        // Pas vraiment envoyer le packet directement ici, mais plutôt le mettre en queue d'envoi
        // d'ENet pour qu'il soit envoyé au bon moment (ENet gère ça en interne)
        if (packet)
            enet_peer_send(peer, channelId, packet);

        // Log : ici on lit le header qui est au début du payload
        const SnapshotHeader* header = reinterpret_cast<const SnapshotHeader*>(msg.payload.data());
        RCNET_log(RCNET_LOG_INFO,
                  "[SERVER] [NETWORK_OUT] [SNAPSHOT_FULL] - Sent snapshotId=%u to connectionId=%u (size=%zu bytes)\n",
                  (header != nullptr) ? header->snapshotId : 0u,
                  msg.connectionId,
                  msg.payload.size());
    }
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

        ServerNetworkOutgoingUpdate_HandleMessageType_SnapshotFull_SendUnreliable(peer, msg);
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