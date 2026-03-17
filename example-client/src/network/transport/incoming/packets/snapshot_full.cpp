#include "network/transport/incoming/packets/snapshot_full.h"

#include "network/serialization/deserialize_packets_server.h"

#include <RC2D/RC2D.h>

void ClientNetworkIncoming_HandlePacket_SnapshotFull(
    const ENetEvent* event,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de deserialiser le packet snapshot full envoye par le serveur.
    ServerSnapshotFullPacketUnreliable snapshotFullPacket{};
    if (!deserializeServerSnapshotFullPacketUnreliable(
            event->packet->data,
            event->packet->dataLength,
            snapshotFullPacket))
    {
        RC2D_log(
            RC2D_LOG_WARN,
            "[CLIENT] [NETWORK_IN] [SNAPSHOT] Failed to deserialize snapshot full packet (size=%u).",
            static_cast<unsigned>(event->packet->dataLength));
        return;
    }

    // Construire le message destination simulation.
    NetworkINToSimulationMessage message{};
    message.type = NetworkINToSimulationMessageType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE;
    message.snapshotFullPacket = snapshotFullPacket;

    // Pousser le snapshot dans la queue simulation.
    netToSimQueue.push(message);
}

