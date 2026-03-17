#include "network/transport/incoming/channels/auth_reliable.h"

#include "network/packets/server/reliable.h"
#include "network/serialization/deserialize_packets_server.h"

void ClientNetworkIncoming_Channel_AuthReliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de désérialiser le packet envoyé par le serveur.
    ServerAuthResponsePacketReliable authResponsePacket{};
    if (!deserializeServerAuthPacketReliable(
            event->packet->data,
            event->packet->dataLength,
            authResponsePacket))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet d’authentification est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[CLIENT] [NETWORK_IN] [AUTH] - Failed to deserialize auth response packet from server (size=%u bytes)",
                  (unsigned)event->packet->dataLength);
        return;
    }

    // Log d’information indiquant que le packet d’authentification a été reçu avec succès.
    RCNET_log(RCNET_LOG_INFO,
              "[CLIENT] [NETWORK_IN] [AUTH] - Packet received from server (size=%u bytes)\n",
              (unsigned)event->packet->dataLength);

    // Crée un message destiné à la simulation.
    NetworkINToSimulationMessage message{};

    // Indique que ce message transporte un packet d’authentification reliable.
    message.type = NetworkINToSimulationMessageType::SERVER_AUTH_RESPONSE_PACKET_RELIABLE;
    // Attache le packet d’authentification reçu au message.
    message.authResponsePacket = authResponsePacket;

    // Envoie le message à la simulation via la queue thread-safe.
    netToSimQueue.push(message);
}