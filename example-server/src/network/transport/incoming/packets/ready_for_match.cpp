#include "network/transport/incoming/packets/ready_for_match.h"

#include "network/packets/client/reliable.h"
#include "network/serialization/deserialize_packets_client.h"

#include <RCNET/RCNET.h>

void ServerNetworkIncoming_HandlePacket_ReadyForMatch(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de désérialiser le packet envoyé par le client.
    ClientReadyForMatchPacketReliable clientReadyPacket{};
    if (!deserializeClientReadyForMatchPacketReliable(
            event->packet->data,
            event->packet->dataLength,
            clientReadyPacket))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet ready-for-match est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [READY_FOR_MATCH] - Failed to deserialize ready-for-match packet from connectionId=%u\n",
                  connectionId);
        return;
    }

    // Log d’information indiquant que le packet ready-for-match a été reçu avec succès.
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

    // Envoie le message à la simulation via la queue thread-safe.
    netToSimQueue.push(message);
}