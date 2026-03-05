#include "server_context.h"
#include "server_network_snapshot_packets.h"

#include <RCNET/RCNET.h>

#include <cstdint>         // uintptr_t
#include <cstring>         // memcpy

static uint32_t ServerNetworkIncomingUpdate_GetConnectionIdFromPeerDataOrZero(const ENetEvent* event)
{
    // Identifier la connexion réseau (connectionId) à partir de event->peer->data
    uint32_t connectionId = 0;
    if (event->peer != nullptr && event->peer->data != nullptr)
        connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));
    return connectionId;
}

static void ServerNetworkIncomingUpdate_HandleConnectEvent(
    const ENetEvent* event,
    NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Générer un connectionId unique pour cette connexion réseau qui vient d'arriver
    uint32_t connectionId = networkState.nextConnectionId++;

    // Associer ce connectionId à event->peer->data pour pouvoir l'identifier lors de futurs événements (inputs, déconnexion, etc.)
    event->peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(connectionId));

    // Stocker le mapping connectionId -> ENetPeer* pour pouvoir envoyer des messages à ce client plus tard
    networkState.connectionIdToEnetPeer[connectionId] = event->peer;

    // Push un message de connexion vers la simulation pour créer une session, etc.
    NetworkINToSimulationMessage message{};
    message.type = NetworkINToSimulationMessageType::CONNECT;
    message.connectionId = connectionId;
    netToSimQueue.push(message);

    RCNET_log(RCNET_LOG_INFO, "[SERVER] [NETWORK_IN] [CONNECT] - connectionId=%u\n", connectionId);
}

static void ServerNetworkIncomingUpdate_HandleDisconnectEvent(
    const ENetEvent* event,
    NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Identifier la connexion réseau (connectionId) à partir de event->peer->data
    uint32_t connectionId = ServerNetworkIncomingUpdate_GetConnectionIdFromPeerDataOrZero(event);

    // Supprimer le mapping connectionId -> ENetPeer*
    networkState.connectionIdToEnetPeer.erase(connectionId);

    // Supprimer event->peer->data pour éviter les problèmes si jamais on reçoit d'autres événements pour ce peer après la déconnexion
    if (event->peer != nullptr)
        event->peer->data = nullptr;

    // Push un message de déconnexion vers la simulation pour nettoyer la session, etc.
    NetworkINToSimulationMessage message{};
    message.type = NetworkINToSimulationMessageType::DISCONNECT;
    message.connectionId = connectionId;
    netToSimQueue.push(message);

    RCNET_log(RCNET_LOG_INFO, "[SERVER] [NETWORK_IN] [DISCONNECT] - connectionId=%u\n", connectionId);
}

static void ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel1Inputs(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    if (event->packet->dataLength == sizeof(ClientInputCommand))
    {
        RCNET_log(RCNET_LOG_INFO,
                  "[SERVER] [NETWORK_IN] [INPUT] - Packet received from connectionId=%u (size=%u bytes)\n",
                  connectionId,
                  (unsigned)event->packet->dataLength);

        // Initialiser une struct d'input à partir des données du packet reçu
        ClientInputCommand inputCmd{};

        // Copier les données du packet dans notre struct d'input (attention à la taille et à l'ordre des données)
        std::memcpy(&inputCmd, event->packet->data, sizeof(ClientInputCommand));

        // Push un message vers la simulation pour traiter cet input
        NetworkINToSimulationMessage message{};
        message.type = NetworkINToSimulationMessageType::PACKET_INPUT;
        message.connectionId = connectionId;
        message.input = inputCmd;

        netToSimQueue.push(message);
    }
}

static void ServerNetworkIncomingUpdate_HandleReceiveEvent_DispatchByChannel(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // channel 0 =  handshake / auth / encrypt (reliable)
    // channel 1 = inputs (unreliable)
    // channel 2 = snapshots (unreliable)
    // channel 3 = events importants (reliable)
    if (event->channelID == 0)
    {
        // TODO: traiter handshake (token / accountIdDatabase / etc.)
        // Puis push un message HANDSHAKE vers la simulation.
    }
    else if (event->channelID == 1)
    {
        ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel1Inputs(event, connectionId, netToSimQueue);
    }
    else if (event->channelID == 2)
    {
        // Pour ce channel, on n'attend rien du client, donc on peut juste ignorer les messages reçus.
    }
    else if (event->channelID == 3)
    {
        // Traiter les messages importants du client (ex: events de gameplay, chat, etc.)
    }
}

// ======================================================================================
// Public entry point called by server_callbacks.cpp
// ======================================================================================

void ServerNetworkIncomingUpdate_ProcessENetEvent(ENetHost* host, const ENetEvent* event)
{
    // Sécurité : vérifier que les pointeurs ne sont pas nuls avant de les utiliser
    if (host == nullptr || event == nullptr)
        return;

    // Accès au state pour identifier la connexion réseau (connectionId) à partir de event->peer et pour stocker le mapping connectionId <-> ENetPeer*
    NetworkState& networkState = GetNetworkState();

    // Accès à la queue réseau -> simulation pour push des messages à traiter par la simulation (ex: connexion, déconnexion, inputs reçus, etc.)
    NetworkINToSimulationQueue& netToSimQueue = GetNetworkINToSimulationQueue();

    // Traiter les événements réseau (connexion, déconnexion, message reçu)
    if (event->type == ENET_EVENT_TYPE_CONNECT)
    {
        ServerNetworkIncomingUpdate_HandleConnectEvent(event, networkState, netToSimQueue);
    }
    else if (event->type == ENET_EVENT_TYPE_DISCONNECT || event->type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
    {
        ServerNetworkIncomingUpdate_HandleDisconnectEvent(event, networkState, netToSimQueue);
    }
    else if (event->type == ENET_EVENT_TYPE_RECEIVE)
    {
        // Sécurité : vérifier que event->peer et event->packet ne sont pas nuls avant de les utiliser
        if (event->peer == nullptr || event->packet == nullptr)
            return;

        // Sécurité : identifier la connexion réseau (connectionId) à partir de event->peer->data, et vérifier 
        // que c'est un connectionId valide avant de traiter le message reçu
        uint32_t connectionId = ServerNetworkIncomingUpdate_GetConnectionIdFromPeerDataOrZero(event);
        if (connectionId == 0)
            return;

        ServerNetworkIncomingUpdate_HandleReceiveEvent_DispatchByChannel(event, connectionId, netToSimQueue);
    }
}