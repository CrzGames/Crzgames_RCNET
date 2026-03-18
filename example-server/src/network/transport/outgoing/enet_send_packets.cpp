#include "network/transport/outgoing/enet_send_packets.h"
#include "network/channels/channel.h" // NetworkChannel
#include "core/context.h"

#include <cstdint>       // uintptr_t
#include <mutex>         // std::lock_guard
#include <new>           // std::nothrow
#include <unordered_map> // std::unordered_map

#include <RCNET/RCNET.h> // RCNET_log

// ============================================================================
// Contexte des actions post-ACK
// ============================================================================
//
// Ce contexte est attache a ENetPacket::userData uniquement pour les paquets
// fiables qui doivent executer une action APRES un ACK reel du pair distant.
//
// Pourquoi:
// - le callback ACK d'ENet recoit seulement ENetPacket*;
// - on stocke donc dans userData l'etat necessaire aux actions post-ACK.
//
// Cycle de vie:
// 1) alloue avant enet_peer_send,
// 2) lu dans le callback ACK,
// 3) toujours detruit dans le callback free du paquet (ACK ou non).
struct ServerNetworkOutgoingReliableAckActionContext
{
    // Peer cible par ce paquet.
    ENetPeer* peer = nullptr;

    // Connection ciblee par ce paquet.
    // Stockee pour se proteger contre la reutilisation d'un slot ENetPeer*.
    uint32_t connectionId = 0;

    // Deconnecter le peer uniquement apres un vrai ACK.
    bool disconnectAfterAck = false;

    // Activer le chiffrement uniquement apres un vrai ACK.
    bool enableEncryptionAfterAck = false;
};

// Extrait le connectionId stocke dans ENetPeer::data.
// Retourne false si peer/data est invalide ou si connectionId == 0.
static bool ServerNetworkOutgoing_TryGetConnectionIdFromPeer(const ENetPeer* peer, uint32_t& outConnectionId)
{
    if (peer == nullptr || peer->data == nullptr)
    {
        return false;
    }

    const uint32_t connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(peer->data));
    if (connectionId == 0)
    {
        return false;
    }

    outConnectionId = connectionId;
    return true;
}

// Verifie que le pointeur peer correspond toujours a la meme connexion logique.
// Cela protege les actions post-ACK contre la reutilisation d'un ENetPeer* stale.
static bool ServerNetworkOutgoing_IsPeerStillBoundToConnectionId(ENetPeer* peer, uint32_t expectedConnectionId)
{
    if (peer == nullptr || expectedConnectionId == 0)
    {
        return false;
    }

    uint32_t currentConnectionId = 0;
    if (!ServerNetworkOutgoing_TryGetConnectionIdFromPeer(peer, currentConnectionId))
    {
        return false;
    }

    if (currentConnectionId != expectedConnectionId)
    {
        return false;
    }

    const NetworkState& networkState = GetNetworkState();
    std::unordered_map<uint32_t, ENetPeer*>::const_iterator it =
        networkState.connectionIdToEnetPeer.find(expectedConnectionId);
    if (it == networkState.connectionIdToEnetPeer.end())
    {
        return false;
    }

    return it->second == peer;
}

// Active/desactive l'etat de chiffrement pour la connexion liee a ce peer.
//
// Verifications defensives:
// - peer doit exister,
// - peer->data doit contenir un connectionId valide,
// - connectionId doit toujours pointer vers le meme ENetPeer*.
static void ServerNetworkOutgoing_SetPeerEncryptionEnabled(ENetPeer* peer, bool enabled)
{
    if (peer == nullptr)
    {
        return;
    }

    uint32_t connectionId = 0;
    if (!ServerNetworkOutgoing_TryGetConnectionIdFromPeer(peer, connectionId))
    {
        RCNET_log(RCNET_LOG_WARN, "[SERVER] [NETWORK_OUT] [ENCRYPTION] - Cannot toggle packet encryption: missing connectionId in peer->data");
        return;
    }
    NetworkState& networkState = GetNetworkState();

    std::unordered_map<uint32_t, ENetPeer*>::const_iterator it =
        networkState.connectionIdToEnetPeer.find(connectionId);
    if (it == networkState.connectionIdToEnetPeer.end() || it->second != peer)
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "[SERVER] [NETWORK_OUT] [ENCRYPTION] - connectionId: %u - Cannot toggle packet encryption (peer mismatch)",
            connectionId);
        return;
    }

    networkState.connectionIdToEncryptionEnabled[connectionId] = enabled;

    // Replique l'etat de chiffrement dans la session pour que les callbacks
    // ENet encrypt/decrypt lisent "enabled" + les cles peer sous le meme mutex.
    {
        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);
        std::unordered_map<uint32_t, ClientSession>::iterator sit =
            networkState.sessions.find(connectionId);
        if (sit != networkState.sessions.end())
        {
            sit->second.isPacketEncryptionEnabled = enabled;
        }
    }

    RCNET_log(
        RCNET_LOG_INFO,
        "[SERVER] [NETWORK_OUT] [ENCRYPTION] - connectionId: %u - packetEncryptionEnabled: %u",
        connectionId,
        enabled ? 1u : 0u);
}

// Callback free d'ENet sur paquet.
//
// ENet appelle ce callback lorsqu'il libere finalement le paquet de ses files.
// On detruit ici le contexte alloue avec new pour eviter les fuites memoire.
static void ENET_CALLBACK ServerNetworkOutgoing_OnReliablePacketFreed_DestroyAckContext(ENetPacket* packet)
{
    if (packet == nullptr)
    {
        return;
    }

    ServerNetworkOutgoingReliableAckActionContext* context =
        static_cast<ServerNetworkOutgoingReliableAckActionContext*>(packet->userData);
    if (context != nullptr)
    {
        delete context;
        packet->userData = nullptr;
    }
}

// Callback ACK d'ENet pour les paquets fiables.
//
// Ce callback s'execute uniquement quand ENet confirme que le paquet a ete ACK
// par le pair distant. On lance ici les actions metier post-livraison reelle.
static void ENET_CALLBACK ServerNetworkOutgoing_OnReliablePacketAcknowledged_RunActions(ENetPacket* packet)
{
    if (packet == nullptr)
    {
        return;
    }

    ServerNetworkOutgoingReliableAckActionContext* context =
        static_cast<ServerNetworkOutgoingReliableAckActionContext*>(packet->userData);
    if (context == nullptr || context->peer == nullptr)
    {
        return;
    }

    if (!ServerNetworkOutgoing_IsPeerStillBoundToConnectionId(context->peer, context->connectionId))
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "[SERVER] [NETWORK_OUT] [RELIABLE_ACK] - connectionId: %u - Ignored stale ACK action",
            context->connectionId);
        return;
    }

    if (context->enableEncryptionAfterAck)
    {
        ServerNetworkOutgoing_SetPeerEncryptionEnabled(context->peer, true);
    }

    if (context->disconnectAfterAck)
    {
        RCNET_log(
            RCNET_LOG_INFO,
            "[SERVER] [NETWORK_OUT] [RELIABLE_ACK] - connectionId: %u - ACK received, scheduling disconnect\n",
            context->connectionId);
        enet_peer_disconnect_later(context->peer, 0);
    }
}

// Verifie si ce peer a une session autorisee pour envoyer du trafic gameplay.
//
// Lorsque requireValidatedSession == true, on exige:
// - transport connecte
// - secure session etablie
// - chiffrement paquet actif
// - authStatus == Valid
//
// Lorsque requireValidatedSession == false, aucun blocage n'est applique.
static bool ServerNetworkOutgoing_IsPeerAllowedToSendPacket(
    ENetPeer* peer,
    bool requireValidatedSession)
{
    if (!requireValidatedSession)
    {
        return true;
    }

    uint32_t connectionId = 0;
    if (!ServerNetworkOutgoing_TryGetConnectionIdFromPeer(peer, connectionId))
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "[SERVER] [NETWORK_OUT] [GUARD] - blocked send: missing connectionId in peer->data");
        return false;
    }

    const NetworkState& networkState = GetNetworkState();
    std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

    std::unordered_map<uint32_t, ClientSession>::const_iterator sit =
        networkState.sessions.find(connectionId);
    if (sit == networkState.sessions.end())
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "[SERVER] [NETWORK_OUT] [GUARD] - blocked send: session not found for connectionId=%u",
            connectionId);
        return false;
    }

    const ClientSession& session = sit->second;
    const bool allowed =
        session.isTransportConnected &&
        session.isSecureSessionEstablished &&
        session.isPacketEncryptionEnabled &&
        session.authStatus == AuthStatus::Valid;

    if (!allowed)
    {
        /*RCNET_log(
            RCNET_LOG_DEBUG,
            "[SERVER] [NETWORK_OUT] [GUARD] - blocked send packet for connectionId=%u (transport=%u secureSession=%u encryptedPacket=%u authTokenVerifiedOrNotValid=%u)",
            connectionId,
            session.isTransportConnected ? 1u : 0u,
            session.isSecureSessionEstablished ? 1u : 0u,
            session.isPacketEncryptionEnabled ? 1u : 0u,
            static_cast<unsigned>(session.authStatus));*/
    }

    return allowed;
}

static bool ServerNetworkOutgoing_SendPacket(
    ENetPeer* peer,
    NetworkChannel channel,
    const std::vector<uint8_t>& bytes,
    enet_uint32 flags,
    bool disconnectAfterAck,
    bool enableEncryptionAfterAck,
    bool requireValidatedSession)
{
    // Le peer doit etre valide avant de creer/envoyer un paquet.
    if (peer == nullptr)
    {
        return false;
    }

    if (!ServerNetworkOutgoing_IsPeerAllowedToSendPacket(peer, requireValidatedSession))
    {
        return false;
    }

    // Warn si la taille depasse la cible "ultra-safe" de payload UDP.
    if (bytes.size() > kServerNetworkOutgoingPayloadMaxBytes)
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "[SERVER] [NETWORK_OUT] [PAYLOAD] - Payload size=%llu bytes exceeds target max=%u bytes (channel=%u).",
            (unsigned long long)bytes.size(),
            (unsigned)kServerNetworkOutgoingPayloadMaxBytes,
            (unsigned)channel);
    }

    // Cree le paquet ENet a partir des bytes serialises.
    ENetPacket* enetPacket = enet_packet_create(
        bytes.data(),
        bytes.size(),
        flags
    );

    // Echec si la creation du paquet ENet a echoue.
    if (enetPacket == nullptr)
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to create ENet packet for sending");
        return false;
    }

    // Si une action post-ACK est demandee, preparer le contexte callback.
    //
    // Important:
    // - les actions post-ACK n'ont de sens que sur paquet fiable;
    // - les paquets non fiables n'ont pas d'ACK de livraison dans ENet.
    if (disconnectAfterAck || enableEncryptionAfterAck)
    {
        if ((flags & ENET_PACKET_FLAG_RELIABLE) == 0)
        {
            RCNET_log(
                RCNET_LOG_WARN,
                "[SERVER] [NETWORK_OUT] [RELIABLE_ACK] - post-ACK actions ignored on non-reliable packet");
        }
        else
        {
            uint32_t connectionId = 0;
            if (!ServerNetworkOutgoing_TryGetConnectionIdFromPeer(peer, connectionId))
            {
                if (disconnectAfterAck)
                {
                    enet_peer_disconnect_later(peer, 0);
                }
                enet_packet_destroy(enetPacket);
                RCNET_log(
                    RCNET_LOG_ERROR,
                    "[SERVER] [NETWORK_OUT] [RELIABLE_ACK] - Failed to resolve connectionId for post-ACK actions");
                return false;
            }

            // Alloue le contexte callback avec nothrow pour garder
            // un chemin d'echec explicite.
            ServerNetworkOutgoingReliableAckActionContext* context =
                new (std::nothrow) ServerNetworkOutgoingReliableAckActionContext();
            if (context == nullptr)
            {
                // Fallback: si l'appelant voulait disconnect-after-ACK,
                // on deconnecte immediatement car on ne peut pas suivre l'ACK
                // sans contexte.
                if (disconnectAfterAck)
                {
                    enet_peer_disconnect_later(peer, 0);
                }

                // ENet n'a pas encore pris possession de ce paquet.
                enet_packet_destroy(enetPacket);
                RCNET_log(RCNET_LOG_ERROR, "Failed to allocate post-ACK context");
                return false;
            }

            context->peer = peer;
            context->connectionId = connectionId;
            context->disconnectAfterAck = disconnectAfterAck;
            context->enableEncryptionAfterAck = enableEncryptionAfterAck;

            // Attache le contexte au paquet et enregistre les deux callbacks.
            enetPacket->userData = context;
            enet_packet_set_acknowledge_callback(
                enetPacket,
                ServerNetworkOutgoing_OnReliablePacketAcknowledged_RunActions);
            enet_packet_set_free_callback(
                enetPacket,
                ServerNetworkOutgoing_OnReliablePacketFreed_DestroyAckContext);
        }
    }

    const int sendResult = enet_peer_send(
        peer,
        static_cast<enet_uint8>(channel),
        enetPacket
    );

    // Gestion d'echec d'envoi.
    if (sendResult < 0)
    {
        // Fallback: conserve l'intention de deconnexion meme si l'envoi echoue.
        if (disconnectAfterAck)
        {
            enet_peer_disconnect_later(peer, 0);
        }

        // Envoi echoue: ENet n'a pas queue ce paquet, il faut le detruire manuellement.
        enet_packet_destroy(enetPacket);
        RCNET_log(RCNET_LOG_ERROR, "Failed to send ENet packet");
        return false;
    }

    return true;
}

bool ServerNetworkOutgoing_SendMatchInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        false,
        false,
        true
    );
}

bool ServerNetworkOutgoing_SendWorldStaticStateInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        false,
        false,
        true
    );
}

bool ServerNetworkOutgoing_SendMatchStartPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        false,
        false,
        true
    );
}

bool ServerNetworkOutgoing_SendSnapshotFullPacketUnreliable(ENetPeer* peer, const std::vector<uint8_t>& bytes)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::GAME_UNRELIABLE,
        bytes,
        0, // 0 = paquet non fiable
        false,
        false,
        true
    );
}

bool ServerNetworkOutgoing_SendSecureSessionHelloResponsePacketReliable(
    ENetPeer* peer,
    const std::vector<uint8_t>& bytes,
    bool disconnectAfterAck,
    bool enableEncryptionAfterAck)
{
    // La reponse secure-session peut faire deux actions post-ACK:
    // - deconnecter apres ACK si la secure session a echoue,
    // - activer le chiffrement apres ACK si la secure session a reussi.
    //
    // Activer le chiffrement seulement apres ACK evite de basculer trop tot,
    // avant confirmation client de reception de cette reponse secure-session.
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::SECURE_SESSION_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        disconnectAfterAck,
        enableEncryptionAfterAck,
        false
    );
}

bool ServerNetworkOutgoing_SendAuthResponsePacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::AUTH_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        disconnectAfterAck,
        false,
        false
    );
}
