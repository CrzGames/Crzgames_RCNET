#include "server_context.h"
#include "server_network_packets_client_reliable.h"
#include "server_network_packets_client_unreliable.h"
#include "server_network_protocol_version.h"
#include "server_network_deserialize_packets_client.h"
#include "server_network_byte_reader.h"
#include "server_queues.h"
#include "server_network_channels.h"

#include <RCNET/RCNET.h>

#include <cstdint>       // uintptr_t
#include <unordered_map> // std::unordered_map

static ClientSession* FindSessionByConnectionId(uint32_t connectionId)
{
    // Récupère une référence vers l’état réseau global du serveur.
    NetworkState& networkState = GetNetworkState();

    // Cherche la session correspondant à ce connectionId dans la map des sessions actives (si existante).
    std::unordered_map<uint32_t, ClientSession>::iterator it = networkState.sessions.find(connectionId);

    // Si aucune session n’est trouvée pour ce connectionId, retourne nullptr.
    if (it == networkState.sessions.end())
    {
        return nullptr;
    }

    // Retourne un pointeur vers la session trouvée.
    return &it->second;
}

// Fonction helper statique locale au fichier.
// Vérifie si une connexion possède bien une session active
// et si cette session est marquée comme transport connecté.
//
// Retourne true si :
// - la session existe
// - isTransportConnected == true
//
// Retourne false sinon.
static bool IsConnectionTransportConnected(
    // Identifiant de connexion à vérifier.
    uint32_t connectionId)
{
    // Récupérer la session correspondant à cette connexion.
    ClientSession* session = FindSessionByConnectionId(connectionId);
    if (session == nullptr)
    {
        return false;
    }

    // Retourne l’état de transport connecté de la session.
    return session->isTransportConnected;
}

// Fonction helper statique locale au fichier.
// Vérifie si une connexion possède une session sécurisée établie.
//
// Retourne true si :
// - la session existe
// - isSecureSessionEstablished == true
//
// Retourne false sinon.
static bool IsConnectionSecureSessionEstablished(
    // Identifiant de connexion à vérifier.
    uint32_t connectionId)
{
    // Récupérer la session correspondant à cette connexion.
    ClientSession* session = FindSessionByConnectionId(connectionId);
    if (session == nullptr)
    {
        return false;
    }

    // Retourne l’état de sécurisation de la session.
    return session->isSecureSessionEstablished;
}

// Fonction helper statique locale au fichier.
// Vérifie si une connexion est authentifiée côté serveur.
//
// Retourne true si :
// - la session existe
// - authStatus == AuthStatus::Valid
//
// Retourne false sinon.
static bool IsConnectionAuthenticated(
    // Identifiant de connexion à vérifier.
    uint32_t connectionId)
{
    // Récupérer la session correspondant à cette connexion.
    ClientSession* session = FindSessionByConnectionId(connectionId);
    if (session == nullptr)
    {
        return false;
    }

    // Retourne l’état d’authentification de la session.
    return session->authStatus == AuthStatus::Valid;
}

static bool IsConnectionAllowedForAuthChannel(uint32_t connectionId)
{
    return IsConnectionTransportConnected(connectionId) &&
           IsConnectionSecureSessionEstablished(connectionId);
}

static bool IsConnectionAllowedForGameplayChannels(uint32_t connectionId)
{
    return IsConnectionTransportConnected(connectionId) &&
           IsConnectionSecureSessionEstablished(connectionId) &&
           IsConnectionAuthenticated(connectionId);
}

static void ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel2GameReliable_ClientReadyForMatch(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    ClientReadyForMatchPacketReliable clientReadyPacket{};
    if (!deserializeClientReadyForMatchPacketReliable(
            event->packet->data,
            event->packet->dataLength,
            clientReadyPacket))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet est invalide.
        RCNET_log(RCNET_LOG_WARN,
                "[SERVER] [NETWORK_IN] [READY_FOR_MATCH] - Failed to deserialize ready-for-match packet from connectionId=%u\n",
                connectionId);
        return;
    }

    // Log d’information indiquant qu’un packet ready-for-match a été reçu.
    RCNET_log(RCNET_LOG_INFO,
            "[SERVER] [NETWORK_IN] [READY_FOR_MATCH] - Packet received from connectionId=%u (size=%u bytes)\n",
            connectionId,
            (unsigned)event->packet->dataLength);

    // Crée un message destiné au thread simulation.
    NetworkINToSimulationMessage message{};
    // Renseigne le type du message de simulation.
    message.type = NetworkINToSimulationMessageType::CLIENT_READY_FOR_MATCH_PACKET_RELIABLE;
    // Renseigne la connexion source.
    message.connectionId = connectionId;

    // Push le message vers la simulation.
    netToSimQueue.push(message);
}

static void ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel3GameUnreliable_InputPacket(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    ClientInputPacketUnreliable inputPacket{};
    if (!deserializeClientInputPacketUnreliable(
            event->packet->data,
            event->packet->dataLength,
            inputPacket))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet est invalide.
        RCNET_log(RCNET_LOG_WARN,
                "[SERVER] [NETWORK_IN] [INPUT] - Failed to deserialize input packet from connectionId=%u\n",
                connectionId);
        return;
    }

    // Log d’information indiquant qu’un packet input a été reçu.
    RCNET_log(RCNET_LOG_INFO,
            "[SERVER] [NETWORK_IN] [INPUT] - Packet received from connectionId=%u (size=%u bytes)\n",
            connectionId,
            (unsigned)event->packet->dataLength);

    // Crée un message destiné à la simulation.
    NetworkINToSimulationMessage message{};
    // Renseigne le type du message de simulation.
    message.type = NetworkINToSimulationMessageType::CLIENT_INPUT_PACKET_UNRELIABLE;
    // Renseigne la connexion source.
    message.connectionId = connectionId;
    // Attache le packet input au message.
    message.inputPacket = inputPacket;

    // Push le message vers la simulation.
    netToSimQueue.push(message);
}

// Fonction helper statique locale au fichier.
// Elle lit le premier octet d’un packet reçu sur un channel reliable client -> serveur
// et l’interprète comme un ClientReliablePacketType.
// Retourne true si la lecture réussit, false sinon.
static bool ServerNetworkIncomingUpdate_ReadClientReliablePacketType(
    // Événement ENet reçu à analyser.
    const ENetEvent* event,
    // Paramètre de sortie dans lequel on écrit le type lu si tout se passe bien.
    ClientReliablePacketType& outType)
{
    // Crée un lecteur binaire positionné au début du payload reçu.
    // Il lira les données depuis event->packet->data sur event->packet->dataLength octets.
    ByteReader reader(event->packet->data, event->packet->dataLength);

    // Variable temporaire brute qui recevra le premier octet du packet.
    // Ce premier octet correspond au type de packet dans ton protocole.
    uint8_t rawType = 0;

    // Essaie de lire un octet depuis le buffer réseau.
    // Si le packet est vide ou trop court, la lecture échoue.
    if (!reader.readU8(rawType))
    {
        // Retourne false pour signaler l’échec.
        return false;
    }

    // Convertit l’octet brut lu en valeur de l’enum ClientReliablePacketType.
    // On suppose ici que le protocole encode bien le type sur 1 octet.
    outType = static_cast<ClientReliablePacketType>(rawType);

    // Retourne true pour signaler que la lecture du type a réussi.
    return true;
}

// Fonction helper statique locale au fichier.
// Elle lit le premier octet d’un packet reçu sur un channel unreliable client -> serveur
// et l’interprète comme un ClientUnreliablePacketType.
// Retourne true si la lecture réussit, false sinon.
static bool ServerNetworkIncomingUpdate_ReadClientUnreliablePacketType(
    // Événement ENet reçu à analyser.
    const ENetEvent* event,
    // Paramètre de sortie dans lequel on écrit le type lu si tout se passe bien.
    ClientUnreliablePacketType& outType)
{
    // Crée un lecteur binaire sur le payload du packet reçu.
    ByteReader reader(event->packet->data, event->packet->dataLength);

    // Variable temporaire brute qui recevra le premier octet du packet.
    uint8_t rawType = 0;

    // Essaie de lire le premier octet du payload.
    // Si la lecture échoue, le packet est invalide ou vide.
    if (!reader.readU8(rawType))
    {
        // Retourne false pour signaler l’échec.
        return false;
    }

    // Convertit l’octet brut lu en valeur de l’enum ClientUnreliablePacketType.
    outType = static_cast<ClientUnreliablePacketType>(rawType);

    // Retourne true pour signaler que le type a été lu correctement.
    return true;
}

// Fonction helper statique locale au fichier.
// Elle valide qu’un ENetEvent reçu correspond bien à une connexion connue,
// et retourne le connectionId associé.
// Si quelque chose est invalide, elle retourne 0.
static uint32_t ServerNetworkIncomingUpdate_GetValidatedConnectionIdOrZero(
    // Événement ENet à valider.
    const ENetEvent* event,
    // Référence en lecture seule vers l’état réseau global du serveur.
    const NetworkState& networkState)
{
    // Vérifie que le peer associé à l'événement existe.
    // Sans peer, on ne peut pas identifier la connexion source.
    if (event->peer == nullptr)
    {
        // Log d’avertissement indiquant que l’événement est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - event->peer == nullptr\n");

        // Retourne 0 pour signaler une connexion invalide.
        return 0;
    }

    // Vérifie que peer->data contient bien quelque chose.
    // Dans ton architecture, peer->data est censé stocker le connectionId.
    // Si c'est nul, alors aucune connexion valide n'est associée à ce peer.
    if (event->peer->data == nullptr)
    {
        // Log d’avertissement indiquant qu’aucun connectionId n’est attaché à ce peer.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - peer->data == nullptr\n");

        // Retourne 0 pour signaler une connexion invalide.
        return 0;
    }

    // Convertit le void* stocké dans peer->data en uintptr_t,
    // puis en uint32_t pour récupérer le connectionId.
    const uint32_t connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));

    // Vérifie que le connectionId n’est pas 0.
    // 0 signifie "ID invalide / non initialisé".
    if (connectionId == 0)
    {
        // Log d’avertissement indiquant qu’un ID invalide a été lu.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - connectionId == 0\n");

        // Retourne 0 pour signaler une connexion invalide.
        return 0;
    }

    // Cherche le connectionId dans la table des connexions actives.
    std::unordered_map<uint32_t, ENetPeer*>::const_iterator it = networkState.connectionIdToEnetPeer.find(connectionId);

    // Vérifie que le connectionId existe bien dans la map.
    if (it == networkState.connectionIdToEnetPeer.end())
    {
        // Log d’avertissement indiquant que l’ID n’est pas connu du serveur.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - Unknown connectionId=%u (not found in connectionIdToEnetPeer)\n",
                  connectionId);

        // Retourne 0 pour signaler une connexion invalide.
        return 0;
    }

    // Vérifie que le ENetPeer* trouvé dans la map est exactement celui de l’événement reçu.
    if (it->second != event->peer)
    {
        // Log d’avertissement indiquant une incohérence entre peer->data et la map serveur.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - Peer mismatch for connectionId=%u (map peer=%p, event peer=%p)\n",
                  connectionId,
                  static_cast<void*>(it->second),
                  static_cast<void*>(event->peer));

        // Retourne 0 pour signaler une connexion invalide.
        return 0;
    }

    // Tous les checks sont bons : on retourne le connectionId validé.
    return connectionId;
}

// Fonction helper statique locale au fichier.
// Elle traite un événement ENet de type CONNECT.
static void ServerNetworkIncomingUpdate_HandleConnectEvent(
    // Événement ENet reçu.
    const ENetEvent* event,
    // Référence modifiable vers l’état réseau global.
    NetworkState& networkState,
    // Référence vers la queue réseau -> simulation.
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Vérifie que le peer existe bien.
    if (event->peer == nullptr)
    {
        // Log d’avertissement si l’événement de connexion ne contient pas de peer valide.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [CONNECT] - Invalid connect event: event->peer == nullptr\n");

        // Abandonne le traitement.
        return;
    }

    // Récupère le prochain ID de connexion disponible puis l’incrémente.
    uint32_t connectionId = networkState.nextConnectionId++;

    // Stocke ce connectionId dans event->peer->data pour pouvoir retrouver cette connexion plus tard.
    event->peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(connectionId));

    // Enregistre le mapping connectionId -> ENetPeer* dans l’état réseau.
    networkState.connectionIdToEnetPeer[connectionId] = event->peer;

    // Crée un message destiné au thread simulation.
    NetworkINToSimulationMessage message{};
    // Indique que ce message correspond à un événement de connexion client.
    message.type = NetworkINToSimulationMessageType::CLIENT_EVENT_CONNECT;
    // Renseigne l’ID de connexion qui vient d’être créée.
    message.connectionId = connectionId;

    // Push le message dans la queue pour que la simulation crée la session, etc.
    netToSimQueue.push(message);

    // Log d’information indiquant qu’une nouvelle connexion a été acceptée.
    RCNET_log(RCNET_LOG_INFO, "[SERVER] [NETWORK_IN] [CONNECT] - connectionId=%u\n", connectionId);
}

// Fonction helper statique locale au fichier.
// Elle traite un événement ENet de type DISCONNECT.
static void ServerNetworkIncomingUpdate_HandleDisconnectEvent(
    // Événement ENet reçu.
    const ENetEvent* event,
    // Référence modifiable vers l’état réseau global.
    NetworkState& networkState,
    // Référence vers la queue réseau -> simulation.
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Vérifie que le peer existe.
    if (event->peer == nullptr)
    {
        // Log d’avertissement si le peer est absent.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [DISCONNECT] - Invalid disconnect event: event->peer == nullptr\n");

        // Abandonne le traitement.
        return;
    }

    // Vérifie que peer->data contient quelque chose.
    if (event->peer->data == nullptr)
    {
        // Log d’avertissement si aucun connectionId n’est stocké dans le peer.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [DISCONNECT] - Invalid disconnect event: peer->data == nullptr\n");

        // Abandonne le traitement.
        return;
    }

    // Récupère le connectionId depuis peer->data.
    uint32_t connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));

    // Supprime le mapping connectionId -> ENetPeer*.
    networkState.connectionIdToEnetPeer.erase(connectionId);

    // Remet peer->data à nullptr par sécurité.
    event->peer->data = nullptr;

    // Crée un message destiné au thread simulation.
    NetworkINToSimulationMessage message{};
    // Indique que ce message correspond à une déconnexion client.
    message.type = NetworkINToSimulationMessageType::CLIENT_EVENT_DISCONNECT;
    // Associe l’ID de connexion qui vient d’être fermée.
    message.connectionId = connectionId;

    // Push le message dans la queue pour que la simulation nettoie la session.
    netToSimQueue.push(message);

    // Log d’information indiquant la déconnexion.
    RCNET_log(RCNET_LOG_INFO, "[SERVER] [NETWORK_IN] [DISCONNECT] - connectionId=%u\n", connectionId);
}

// Fonction helper statique locale au fichier.
// Elle traite les messages reçus sur le channel 0, réservé ici pour établir une session sécurisée
// via des échange de clés via libsodium.
static void ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel0SecureSessionReliable(
    // Événement ENet de réception.
    const ENetEvent* event,
    // ID de connexion validé en amont.
    uint32_t connectionId,
    // Référence vers la queue réseau -> simulation.
    NetworkINToSimulationQueue& netToSimQueue)
{
    ClientSecureSessionHelloPacketReliable secureSessionHelloPacket{};
    if (!deserializeClientSecureSessionHelloPacketReliable(
            event->packet->data,
            event->packet->dataLength,
            secureSessionHelloPacket))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet de handshake est invalide.
        RCNET_log(RCNET_LOG_WARN,
                "[SERVER] [NETWORK_IN] [SECURE_SESSION] - Failed to deserialize secure session packet from connectionId=%u\n",
                connectionId);
        return;
    }

    // Log d’information indiquant qu’un handshake a été reçu.
    RCNET_log(RCNET_LOG_INFO,
            "[SERVER] [NETWORK_IN] [SECURE_SESSION] - Packet received from connectionId=%u (size=%u bytes)\n",
            connectionId,
            (unsigned)event->packet->dataLength);

    // Vérifie que la version protocole envoyée par le client est compatible avec celle du serveur.
    if (secureSessionHelloPacket.networkProtocolVersion != SERVER_NETWORK_PROTOCOL_VERSION)
    {
        // Log d’erreur indiquant un mismatch de version.
        RCNET_log(RCNET_LOG_ERROR,
                "[SERVER] [NETWORK_IN] [SECURE_SESSION] - Network protocol version mismatch with connectionId=%u: client=%u vs server=%u. Disconnecting client.\n",
                connectionId,
                secureSessionHelloPacket.networkProtocolVersion,
                SERVER_NETWORK_PROTOCOL_VERSION);

        // Déconnecte immédiatement le client.
        enet_peer_disconnect(event->peer, 0);

        // Abandonne le traitement.
        return;
    }

    // Crée un message destiné à la simulation.
    NetworkINToSimulationMessage message{};
    // Indique que ce message transporte un handshake reliable.
    message.type = NetworkINToSimulationMessageType::CLIENT_SECURE_SESSION_HELLO_PACKET_RELIABLE;
    // Attache le connectionId source.
    message.connectionId = connectionId;
    // Attache le packet de secure session reçu au message.
    message.secureSessionHelloPacket = secureSessionHelloPacket;

    // Push le message vers la simulation.
    netToSimQueue.push(message);
}

static void ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel1AuthReliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    ClientAuthPacketReliable authPacket{};
    if (!deserializeClientAuthPacketReliable(
            event->packet->data,
            event->packet->dataLength,
            authPacket))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet d’authentification est invalide.
        RCNET_log(RCNET_LOG_WARN,
                "[SERVER] [NETWORK_IN] [AUTH] - Failed to deserialize auth packet from connectionId=%u\n",
                connectionId);
        return;
    }

    // Log d’information indiquant qu’un packet d’authentification a été reçu.
    RCNET_log(RCNET_LOG_INFO,
            "[SERVER] [NETWORK_IN] [AUTH] - Packet received from connectionId=%u (size=%u bytes)\n",
            connectionId,
            (unsigned)event->packet->dataLength);

    // Crée un message destiné à la simulation.
    NetworkINToSimulationMessage message{};
    // Indique que ce message transporte un packet d’authentification reliable.
    message.type = NetworkINToSimulationMessageType::CLIENT_AUTH_PACKET_RELIABLE;
    // Attache le connectionId source.
    message.connectionId = connectionId;
    // Attache le packet d’authentification reçu au message.
    message.authPacket = authPacket;

    // Push le message vers la simulation.
    netToSimQueue.push(message);
}

// Fonction helper statique locale au fichier.
// Elle traite les messages reçus sur le channel 2, réservé ici aux packets reliable gameplay.
static void ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel2GameReliable(
    // Événement ENet de réception.
    const ENetEvent* event,
    // ID de connexion validé en amont.
    uint32_t connectionId,
    // Référence vers la queue réseau -> simulation.
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Lit le type de packet fiable envoyé par le client.
    ClientReliablePacketType packetType{};
    if (!ServerNetworkIncomingUpdate_ReadClientReliablePacketType(event, packetType))
    {
        // Si la lecture du type échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet est invalide.
        RCNET_log(RCNET_LOG_WARN,
                "[SERVER] [NETWORK_IN] [RELIABLE] - Failed to read reliable packet type from connectionId=%u\n",
                connectionId);
        return;
    }

    // Dispatch le traitement selon le type de packet fiable reçu.
    switch (packetType)
    {
        case ClientReliablePacketType::CLIENT_READY_FOR_MATCH_PACKET_RELIABLE:
        {
            ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel2GameReliable_ClientReadyForMatch(event, connectionId, netToSimQueue);
            break;
        }

        default:
            // type inconnu ou interdit sur ce channel
            RCNET_log(RCNET_LOG_WARN,
                    "[SERVER] [NETWORK_IN] [RELIABLE] - Unknown or unexpected reliable packet type=%u from connectionId=%u\n",
                    static_cast<unsigned>(packetType),
                    connectionId);
            break;
    }
}

// Fonction helper statique locale au fichier.
// Elle traite les messages reçus sur le channel 3, réservé ici aux packets unreliable gameplay.
static void ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel3GameUnreliable(
    // Événement ENet de réception.
    const ENetEvent* event,
    // ID de connexion validé en amont.
    uint32_t connectionId,
    // Référence vers la queue réseau -> simulation.
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Lit le type de packet non fiable envoyé par le client.
    ClientUnreliablePacketType packetType{};
    if (!ServerNetworkIncomingUpdate_ReadClientUnreliablePacketType(event, packetType))
    {
        // Si la lecture du type échoue, le packet est invalide ou mal formé.
        // Log d'avertissement indiquant que le packet est invalide.
        RCNET_log(RCNET_LOG_WARN,
                "[SERVER] [NETWORK_IN] [UNRELIABLE] - Failed to read unreliable packet type from connectionId=%u\n",
                connectionId);
        return;
    }

    // Dispatch le traitement selon le type de packet non fiable reçu.
    switch (packetType)
    {
        case ClientUnreliablePacketType::CLIENT_INPUT_PACKET_UNRELIABLE:
            ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel3GameUnreliable_InputPacket(event, connectionId, netToSimQueue);
            break;

        default:
            // type inconnu ou interdit sur ce channel
            RCNET_log(RCNET_LOG_WARN,
                    "[SERVER] [NETWORK_IN] [UNRELIABLE] - Unknown or unexpected unreliable packet type=%u from connectionId=%u\n",
                    static_cast<unsigned>(packetType),
                    connectionId);
            break;
    }
}

// Fonction helper statique locale au fichier.
// Elle choisit quel handler appeler selon le channel ENet sur lequel le packet a été reçu.
static void ServerNetworkIncomingUpdate_HandleReceiveEvent_DispatchByChannel(
    // Événement ENet de réception.
    const ENetEvent* event,
    // ID de connexion validé.
    uint32_t connectionId,
    // Référence vers la queue réseau -> simulation.
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Récupère le channel sur lequel le packet est arrivé.
    const NetworkChannel channel = static_cast<NetworkChannel>(event->channelID);

    // Vérifie que la connexion a le droit d’envoyer sur ce channel selon son état de session.
    if (channel == NetworkChannel::GAME_RELIABLE || channel == NetworkChannel::GAME_UNRELIABLE)
    {
        if (!IsConnectionAllowedForGameplayChannels(connectionId))
        {
            RCNET_log(RCNET_LOG_WARN,
                    "[SERVER] [NETWORK_IN] [RECEIVE] - ConnectionId=%u is not allowed to send on gameplay channel %u (not authenticated or secure session not established)\n",
                    connectionId,
                    static_cast<unsigned>(channel));
            return;
        }
    }
    else if (channel == NetworkChannel::AUTH_RELIABLE)
    {
        if (!IsConnectionAllowedForAuthChannel(connectionId))
        {
            RCNET_log(RCNET_LOG_WARN,
                    "[SERVER] [NETWORK_IN] [RECEIVE] - ConnectionId=%u is not allowed to send on auth channel %u (secure session not established)\n",
                    connectionId,
                    static_cast<unsigned>(channel));
            return;
        }
    }

    // Dispatch le traitement du packet selon le channel ENet utilisé.
    switch (channel)
    {
        case NetworkChannel::SECURE_SESSION_RELIABLE:
            ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel0SecureSessionReliable(event, connectionId, netToSimQueue);
            break;

        case NetworkChannel::AUTH_RELIABLE:
            ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel1AuthReliable(event, connectionId, netToSimQueue);
            break;

        case NetworkChannel::GAME_RELIABLE:
            ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel2GameReliable(event, connectionId, netToSimQueue);
            break;

        case NetworkChannel::GAME_UNRELIABLE:
            ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel3GameUnreliable(event, connectionId, netToSimQueue);
            break;

        default:
            RCNET_log(RCNET_LOG_WARN,
                    "[SERVER] [NETWORK_IN] [RECEIVE] - Unknown channel=%u for connectionId=%u\n",
                    static_cast<unsigned>(event->channelID),
                    connectionId);
            break;
    }
}

// Point d’entrée public appelé depuis server_callbacks.cpp.
// Cette fonction traite un événement ENet reçu par le serveur.
void ServerNetworkIncomingUpdate_ProcessENetEvent(ENetHost* host, const ENetEvent* event)
{
    // Vérifie que les pointeurs d’entrée sont valides.
    if (host == nullptr || event == nullptr)
        return;

    // Récupère une référence vers l’état réseau global.
    NetworkState& networkState = GetNetworkState();

    // Récupère la queue réseau -> simulation.
    NetworkINToSimulationQueue& netToSimQueue = GetNetworkINToSimulationQueue();

    // Vérifie si l’événement est une connexion.
    if (event->type == ENET_EVENT_TYPE_CONNECT)
    {
        // Traite l’événement de connexion.
        ServerNetworkIncomingUpdate_HandleConnectEvent(event, networkState, netToSimQueue);
    }
    // Vérifie si l’événement est une déconnexion normale ou timeout.
    else if (event->type == ENET_EVENT_TYPE_DISCONNECT || event->type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
    {
        // Traite l’événement de déconnexion.
        ServerNetworkIncomingUpdate_HandleDisconnectEvent(event, networkState, netToSimQueue);
    }
    // Vérifie si l’événement est une réception de packet.
    else if (event->type == ENET_EVENT_TYPE_RECEIVE)
    {
        // Valide la connexion source et récupère son connectionId.
        uint32_t connectionId = ServerNetworkIncomingUpdate_GetValidatedConnectionIdOrZero(event, networkState);

        // Si l’ID est invalide, on ignore le packet.
        if (connectionId == 0)
            return;

        // Dispatch le traitement du packet selon le channel ENet utilisé.
        ServerNetworkIncomingUpdate_HandleReceiveEvent_DispatchByChannel(event, connectionId, netToSimQueue);
    }
}