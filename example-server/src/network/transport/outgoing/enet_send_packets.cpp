#include "network/transport/outgoing/enet_send_packets.h"
#include "network/channels/channel.h" // NetworkChannel

#include <RCNET/RCNET.h> // RCNET_log

// Callback pour les acknowledgments de packets fiables qui déclenche la déconnexion du peer associé.
// Cette fonction est appelée par ENet lorsque le packet fiable est accusé de réception par le client.
static void ENET_CALLBACK ServerNetworkOutgoing_OnReliablePacketAcknowledged_DisconnectPeer(ENetPacket* packet)
{
    if (packet == nullptr)
    {
        return;
    }

    ENetPeer* peer = static_cast<ENetPeer*>(packet->userData);
    if (peer == nullptr)
    {
        return;
    }

    RCNET_log(RCNET_LOG_INFO, "[SERVER] [NETWORK_OUT] [RELIABLE_ACK] - ACK received, disconnecting peer=%p\n", static_cast<void*>(peer));
    enet_peer_disconnect_later(peer, 0);
}

static bool ServerNetworkOutgoing_SendPacket(
    ENetPeer* peer,
    NetworkChannel channel,
    const std::vector<uint8_t>& bytes,
    enet_uint32 flags,
    bool disconnectAfterAck)
{
    // Vérification de la validité du peer avant d'essayer d'envoyer un packet.
    if (peer == nullptr)
    {
        return false;
    }

    // Création d'un ENetPacket à partir des données sérialisées.
    ENetPacket* enetPacket = enet_packet_create(
        bytes.data(),
        bytes.size(),
        flags
    );

    // Vérification de la création du packet avant d'essayer de l'envoyer.
    if (enetPacket == nullptr)
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to create ENet packet for sending");
        return false;
    }

    // Envoi du packet via ENet sur le channel spécifié.
    if (disconnectAfterAck)
    {
        if ((flags & ENET_PACKET_FLAG_RELIABLE) == 0)
        {
            RCNET_log(RCNET_LOG_WARN, "[SERVER] [NETWORK_OUT] [RELIABLE_ACK] - disconnectAfterAck ignored on non-reliable packet");
        }
        else
        {
            enetPacket->userData = peer;
            enet_packet_set_acknowledge_callback(
                enetPacket,
                ServerNetworkOutgoing_OnReliablePacketAcknowledged_DisconnectPeer);
        }
    }

    const int sendResult = enet_peer_send(
        peer,
        static_cast<enet_uint8>(channel),
        enetPacket
    );

    // Vérification du résultat de l'envoi avant de retourner.
    if (sendResult < 0)
    {
        // Fallback : si ce packet devait aboutir a une deconnexion post-ACK,
        // demander quand meme la deconnexion meme si l'envoi a echoue.
        if (disconnectAfterAck)
        {
            enet_peer_disconnect_later(peer, 0);
        }

        // En cas d'échec de l'envoi, il faut détruire le packet manuellement 
        // pour éviter les fuites de mémoire, car ENet ne l'a pas pris en charge.
        enet_packet_destroy(enetPacket);
        RCNET_log(RCNET_LOG_ERROR, "Failed to send ENet packet");
        return false;
    }

    return true;
}

bool ServerNetworkOutgoing_SendMatchInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        disconnectAfterAck
    );
}

bool ServerNetworkOutgoing_SendWorldStaticStateInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        disconnectAfterAck
    );
}

bool ServerNetworkOutgoing_SendMatchStartPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        disconnectAfterAck
    );
}

bool ServerNetworkOutgoing_SendSnapshotFullPacketUnreliable(ENetPeer* peer, const std::vector<uint8_t>& bytes)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::GAME_UNRELIABLE,
        bytes,
        0, // 0 signifie que le packet est envoyé de manière non fiable (unreliable)
        false
    );
}

bool ServerNetworkOutgoing_SendSecureSessionHelloResponsePacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::SECURE_SESSION_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        disconnectAfterAck
    );
}

bool ServerNetworkOutgoing_SendAuthResponsePacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::AUTH_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        disconnectAfterAck
    );
}
