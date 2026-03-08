#include "network/transport/outgoing/enet_send_packets.h"
#include "network/channels/channel.h" // NetworkChannel

#include <RCNET/RCNET.h> // RCNET_log

static bool sendSerializedPacket(ENetPeer* peer, NetworkChannel channel, const std::vector<uint8_t>& bytes, enet_uint32 flags)
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
    const int sendResult = enet_peer_send(
        peer,
        static_cast<enet_uint8>(channel),
        enetPacket
    );

    // Vérification du résultat de l'envoi avant de retourner.
    if (sendResult < 0)
    {
        // En cas d'échec de l'envoi, il faut détruire le packet manuellement 
        // pour éviter les fuites de mémoire, car ENet ne l'a pas pris en charge.
        enet_packet_destroy(enetPacket);
        RCNET_log(RCNET_LOG_ERROR, "Failed to send ENet packet");
        return false;
    }

    return true;
}

bool sendServerMatchInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes)
{
    return sendSerializedPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE
    );
}

bool sendServerWorldStaticStateInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes)
{
    return sendSerializedPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE
    );
}

bool sendServerMatchStartPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes)
{
    return sendSerializedPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE
    );
}

bool sendServerSnapshotFullPacketUnreliable(ENetPeer* peer, const std::vector<uint8_t>& bytes)
{
    return sendSerializedPacket(
        peer,
        NetworkChannel::GAME_UNRELIABLE,
        bytes,
        0 // 0 signifie que le packet est envoyé de manière non fiable (unreliable)
    );
}

bool sendServerSecureSessionHelloResponsePacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes)
{
    return sendSerializedPacket(
        peer,
        NetworkChannel::SECURE_SESSION_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE
    );
}

bool sendServerAuthResponsePacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes)
{
    return sendSerializedPacket(
        peer,
        NetworkChannel::AUTH_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE
    );
}