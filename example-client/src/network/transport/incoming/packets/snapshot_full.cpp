#include "network/transport/incoming/packets/snapshot_full.h"

#include "network/packets/server/unreliable.h"
#include "network/serialization/deserialize_packets_server.h"

void ClientNetworkIncoming_HandlePacket_SnapshotFull(
    const ENetEvent* event,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de désérialiser le packet envoyé par le serveur.
    ServerSnapshotFullPacketUnreliable snapshotFullPacket{};
    if (!deserializeServerSnapshotFullPacketUnreliable(
            event->packet->data,
            event->packet->dataLength,
            snapshotFullPacket))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet d’input est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[CLIENT] [NETWORK_IN] [SNAPSHOT_FULL] - Failed to deserialize snapshot full packet from server (size=%u bytes)",
                  (unsigned)event->packet->dataLength);
        return;
    }

    // Log d’information indiquant que le packet snapshot full a été reçu avec succès.
    RCNET_log(RCNET_LOG_INFO,
              "[CLIENT] [NETWORK_IN] [SNAPSHOT_FULL] - Packet received from server (size=%u bytes)\n",
              (unsigned)event->packet->dataLength);

    // Crée un message destiné à la simulation.
    NetworkINToSimulationMessage message{};
    
    // Renseigne le type du message de simulation.
    message.type = NetworkINToSimulationMessageType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE;
    // Attache le packet snapshot full au message.
    message.snapshotFullPacket = snapshotFullPacket;

    // Envoie le message à la simulation via la queue thread-safe.
    netToSimQueue.push(message);
}