#include "server_context.h"
#include "server_network_packets_client_reliable.h"
#include "server_network_packets_client_unreliable.h"
#include "server_network_protocol_version.h"

#include <RCNET/RCNET.h>

#include <cstdint>         // uintptr_t
#include <cstring>         // memcpy

static uint32_t ServerNetworkIncomingUpdate_GetValidatedConnectionIdOrZero(
    const ENetEvent* event,
    const NetworkState& networkState)
{
    // Vérifie que le pointeur vers l'événement ENet est valide.
    // Si event est nul, on ne peut rien lire du tout.
    if (event == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - event == nullptr\n");
        return 0;
    }

    // Vérifie que le peer associé à l'événement existe.
    // Sans peer, on ne peut pas identifier la connexion source.
    if (event->peer == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - event->peer == nullptr\n");
        return 0;
    }

    // Vérifie que peer->data contient bien quelque chose.
    // Dans ton architecture, peer->data est censé stocker le connectionId.
    // Si c'est nul, alors aucune connexion valide n'est associée à ce peer.
    if (event->peer->data == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - peer->data == nullptr\n");
        return 0;
    }

    // Lit le connectionId stocké dans peer->data.
    // peer->data est un void*, donc on le convertit d'abord en entier de taille uintptr_t,
    // puis en uint32_t pour retrouver ton identifiant de connexion.
    const uint32_t connectionId =
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));

    // Sécurité supplémentaire :
    // dans ton système, 0 signifie "connectionId invalide / non initialisé".
    // Si on lit 0, on refuse de traiter l'événement.
    if (connectionId == 0)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - connectionId == 0\n");
        return 0;
    }

    // Recherche dans la map réseau si ce connectionId est bien connu du serveur.
    // La map connectionIdToEnetPeer contient les connexions actives côté thread réseau.
    std::unordered_map<uint32_t, ENetPeer*>::const_iterator it = networkState.connectionIdToEnetPeer.find(connectionId);

    // Si le connectionId n'existe pas dans la map,
    // alors ce peer n'est pas reconnu comme une connexion active valide.
    if (it == networkState.connectionIdToEnetPeer.end())
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - Unknown connectionId=%u (not found in connectionIdToEnetPeer)\n",
                  connectionId);
        return 0;
    }

    // Vérifie que le ENetPeer* stocké dans la map pour ce connectionId
    // est exactement le même que celui reçu dans l'événement.
    // Ça permet de détecter une incohérence entre peer->data et la table des connexions.
    if (it->second != event->peer)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - Peer mismatch for connectionId=%u (map peer=%p, event peer=%p)\n",
                  connectionId,
                  static_cast<void*>(it->second),
                  static_cast<void*>(event->peer));
        return 0;
    }

    // Si tous les checks sont passés :
    // - event est valide
    // - peer est valide
    // - peer->data contient un connectionId non nul
    // - ce connectionId existe bien dans la map
    // - le peer associé dans la map correspond bien au peer de l'événement
    // alors on peut considérer ce connectionId comme valide et le retourner.
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
    message.type = NetworkINToSimulationMessageType::CLIENT_EVENT_CONNECT;
    message.connectionId = connectionId;
    netToSimQueue.push(message);

    RCNET_log(RCNET_LOG_INFO, "[SERVER] [NETWORK_IN] [CONNECT] - connectionId=%u\n", connectionId);
}

static void ServerNetworkIncomingUpdate_HandleDisconnectEvent(
    const ENetEvent* event,
    NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue)
{
    if (event == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [DISCONNECT] - Invalid disconnect event: event == nullptr\n");
        return;
    }
    if (event->peer == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [DISCONNECT] - Invalid disconnect event: event->peer == nullptr\n");
        return;
    }
    if (event->peer->data == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [DISCONNECT] - Invalid disconnect event: peer->data == nullptr\n");
        return;
    }

    // Identifier la connexion réseau (connectionId) à partir de event->peer->data
    uint32_t connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));

    // Supprimer le mapping connectionId -> ENetPeer*
    networkState.connectionIdToEnetPeer.erase(connectionId);

    // Supprimer event->peer->data pour éviter les problèmes si jamais on reçoit d'autres événements pour ce peer après la déconnexion
    event->peer->data = nullptr;

    // Push un message de déconnexion vers la simulation pour nettoyer la session, etc.
    NetworkINToSimulationMessage message{};
    message.type = NetworkINToSimulationMessageType::CLIENT_EVENT_DISCONNECT;
    message.connectionId = connectionId;
    netToSimQueue.push(message);

    RCNET_log(RCNET_LOG_INFO, "[SERVER] [NETWORK_IN] [DISCONNECT] - connectionId=%u\n", connectionId);
}

static void ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel0Handshake(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // CLIENT_HANDSHAKE_RELIABLE
    if (event->packet->dataLength == sizeof(ClientHandshakePacketReliable))
    {
        // 1) Lire le header seul
        ClientReliablePacketHeader header{};
        std::memcpy(&header, event->packet->data, sizeof(ClientReliablePacketHeader));

        // 2) Vérifier le type attendu
        if (header.type != ClientReliablePacketType::CLIENT_HANDSHAKE_PACKET_RELIABLE)
        {
            RCNET_log(RCNET_LOG_WARN,
                    "[SERVER] [NETWORK_IN] [HANDSHAKE] - Invalid reliable packet type from connectionId=%u\n",
                    connectionId);
            return;
        }

        RCNET_log(RCNET_LOG_INFO,
                "[SERVER] [NETWORK_IN] [HANDSHAKE] - Packet received from connectionId=%u (size=%u bytes)\n",
                connectionId,
                (unsigned)event->packet->dataLength);

        // 3) Copier le packet complet
        ClientHandshakePacketReliable handshakePacket{};
        std::memcpy(&handshakePacket, event->packet->data, sizeof(ClientHandshakePacketReliable));

        // 5) Vérification version protocole
        if (handshakePacket.networkProtocolVersion != NETWORK_PROTOCOL_VERSION)
        {
            RCNET_log(RCNET_LOG_ERROR,
                    "[SERVER] [NETWORK_IN] [HANDSHAKE] - Network protocol version mismatch with connectionId=%u: client=%u vs server=%u. Disconnecting client.\n",
                    connectionId,
                    handshakePacket.networkProtocolVersion,
                    NETWORK_PROTOCOL_VERSION);

            // Déconnecter le client
            enet_peer_disconnect(event->peer, 0);
            return;
        }

        // 6) Push vers la simulation
        NetworkINToSimulationMessage message{};
        message.type = NetworkINToSimulationMessageType::CLIENT_HANDSHAKE_PACKET_RELIABLE;
        message.connectionId = connectionId;
        message.handshakePacket = handshakePacket;

        netToSimQueue.push(message);
    }
}

static void ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel1Reliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // CLIENT_READY_FOR_MATCH_RELIABLE
    if (event->packet->dataLength == sizeof(ClientReadyForMatchPacketReliable))
    {
        // 1) Lire le header seul
        ClientReliablePacketHeader header{};
        std::memcpy(&header, event->packet->data, sizeof(ClientReliablePacketHeader));

        // 2) Vérifier le type attendu
        if (header.type != ClientReliablePacketType::CLIENT_READY_FOR_MATCH_PACKET_RELIABLE)
        {
            RCNET_log(RCNET_LOG_WARN,
                    "[SERVER] [NETWORK_IN] [READY_FOR_MATCH] - Invalid reliable packet type from connectionId=%u\n",
                    connectionId);
            return;
        }

        RCNET_log(RCNET_LOG_INFO,
                "[SERVER] [NETWORK_IN] [READY_FOR_MATCH] - Packet received from connectionId=%u (size=%u bytes)\n",
                connectionId,
                (unsigned)event->packet->dataLength);

        // Copier le packet complet
        ClientReadyForMatchPacketReliable readyForMatchPacket{};
        std::memcpy(&readyForMatchPacket, event->packet->data, sizeof(ClientReadyForMatchPacketReliable));

        // Créer un message de type CLIENT_READY_FOR_MATCH_PACKET_RELIABLE pour la simulation
        NetworkINToSimulationMessage message{};
        message.type = NetworkINToSimulationMessageType::CLIENT_READY_FOR_MATCH_PACKET_RELIABLE;
        message.connectionId = connectionId;

        // Push vers la simulation pour marquer cette session comme prête pour le match
        netToSimQueue.push(message);
    }
}

static void ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel2Unreliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // CLIENT_INPUT_UNRELIABLE
    if (event->packet->dataLength == sizeof(ClientInputPacketUnreliable))
    {
        // 1) Lire le header seul
        ClientUnreliablePacketHeader header{};
        std::memcpy(&header, event->packet->data, sizeof(ClientUnreliablePacketHeader));

        // 2) Vérifier le type attendu
        if (header.type != ClientUnreliablePacketType::CLIENT_INPUT_PACKET_UNRELIABLE)
        {
            RCNET_log(RCNET_LOG_WARN,
                    "[SERVER] [NETWORK_IN] [INPUT] - Invalid unreliable packet type from connectionId=%u\n",
                    connectionId);
            return;
        }

        RCNET_log(RCNET_LOG_INFO,
                "[SERVER] [NETWORK_IN] [INPUT] - Packet received from connectionId=%u (size=%u bytes)\n",
                connectionId,
                (unsigned)event->packet->dataLength);

        // 3) Copier le packet complet
        ClientInputPacketUnreliable inputPacket{};
        std::memcpy(&inputPacket, event->packet->data, sizeof(ClientInputPacketUnreliable));

        // 4) Push vers la simulation
        NetworkINToSimulationMessage message{};
        message.type = NetworkINToSimulationMessageType::CLIENT_INPUT_PACKET_UNRELIABLE;
        message.connectionId = connectionId;
        message.inputPacket = inputPacket;

        netToSimQueue.push(message);
    }
}

static void ServerNetworkIncomingUpdate_HandleReceiveEvent_DispatchByChannel(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    if (event->channelID == 0)
    {
        // TODO: traiter handshake (token / accountIdDatabase / etc.)
        ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel0Handshake(event, connectionId, netToSimQueue);
    }
    else if (event->channelID == 1)
    {
        ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel1Reliable(event, connectionId, netToSimQueue);
    }
    else if (event->channelID == 2)
    {
        ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel2Unreliable(event, connectionId, netToSimQueue);
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
        // Sécurité : valider que le connectionId associé à ce message reçu est bien valide avant de tenter de le traiter
        uint32_t connectionId = ServerNetworkIncomingUpdate_GetValidatedConnectionIdOrZero(event, networkState);
        if (connectionId == 0)
            return;

        // Traiter le message reçu en fonction du channel sur lequel il est arrivé
        ServerNetworkIncomingUpdate_HandleReceiveEvent_DispatchByChannel(event, connectionId, netToSimQueue);
    }
}