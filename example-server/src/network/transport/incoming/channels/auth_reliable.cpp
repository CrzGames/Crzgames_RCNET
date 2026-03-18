#include "network/transport/incoming/channels/auth_reliable.h"

#include "network/packets/client/reliable.h"
#include "network/serialization/deserialize_packets_client.h"

#include <RCNET/RCNET.h>

void ServerNetworkIncoming_Channel_AuthReliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de désérialiser le packet envoyé par le client.
    ClientAuthPacketReliable authPacket{};
    if (!deserializeClientAuthPacketReliable(
            event->packet->data,
            event->packet->dataLength,
            authPacket))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet d’authentification est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [AUTH] - connectionId: %u - Failed to deserialize auth packet\n",
                  connectionId);
        return;
    }

    // Log d’information indiquant que le packet d’authentification a été reçu avec succès.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [NETWORK_IN] [AUTH] - connectionId: %u - Auth packet received (size: %u bytes)\n",
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

    // Envoie le message à la simulation via la queue thread-safe.
    netToSimQueue.push(message);
}
