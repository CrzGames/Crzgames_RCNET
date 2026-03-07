#include "server_network_send_packets_server.h"

#include <RCNET/RCNET.h>

#include <vector>  // std::vector
#include <cstdint> // uint8_t, uint32_t, etc.

#include "server_network_channels.h"
#include "server_network_serialize_packets_server.h"

bool sendSerializedPacket(ENetPeer* peer, NetworkChannel channel, const std::vector<uint8_t>& bytes, enet_uint32 flags)
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

bool sendServerMatchInitPacketReliable(ENetPeer* peer, const ServerMatchInitPacketReliable& packet)
{
    const std::vector<uint8_t> bytes = serializeServerMatchInitPacketReliable(packet);

    return sendSerializedPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE
    );
}

bool sendServerWorldStaticStateInitPacketReliable(ENetPeer* peer, const ServerWorldStaticStateInitPacketReliable& packet)
{
    const std::vector<uint8_t> bytes = serializeServerWorldStaticStateInitPacketReliable(packet);

    return sendSerializedPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE
    );
}

bool sendServerMatchStartPacketReliable(ENetPeer* peer, const ServerMatchStartPacketReliable& packet)
{
    const std::vector<uint8_t> bytes = serializeServerMatchStartPacketReliable(packet);

    return sendSerializedPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE
    );
}

bool sendServerSnapshotFullPacketUnreliable(ENetPeer* peer, const ServerSnapshotFullPacketUnreliable& packet)
{
    const std::vector<uint8_t> bytes = serializeServerSnapshotFullPacketUnreliable(packet);

    return sendSerializedPacket(
        peer,
        NetworkChannel::GAME_UNRELIABLE,
        bytes,
        0 // 0 signifie que le packet est envoyé de manière non fiable (unreliable)
    );
}