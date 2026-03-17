#include "network/transport/incoming/channels/auth_reliable.h"

#include "network/serialization/deserialize_packets_server.h"

#include <RC2D/RC2D.h>

void ClientNetworkIncoming_Channel_AuthReliable(
    const ENetEvent* event,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de deserialiser le packet envoye par le serveur.
    ServerAuthResponsePacketReliable authResponsePacket{};
    if (!deserializeServerAuthResponsePacketReliable(
            event->packet->data,
            event->packet->dataLength,
            authResponsePacket))
    {
        // Si la deserialisation echoue, le packet est invalide ou mal forme.
        RC2D_log(
            RC2D_LOG_WARN,
            "[CLIENT] [NETWORK_IN] [AUTH] - Failed to deserialize auth response packet from server (size=%u bytes).",
            static_cast<unsigned>(event->packet->dataLength));
        return;
    }

    // Log d'information indiquant que le packet d'authentification est bien recu.
    RC2D_log(
        RC2D_LOG_INFO,
        "[CLIENT] [NETWORK_IN] [AUTH] - Packet received from server (size=%u bytes).",
        static_cast<unsigned>(event->packet->dataLength));

    // Creer un message destination simulation.
    NetworkINToSimulationMessage message{};

    // Le message transporte un packet d'authentification reliable serveur.
    message.type = NetworkINToSimulationMessageType::SERVER_AUTH_RESPONSE_PACKET_RELIABLE;
    message.authResponsePacket = authResponsePacket;

    // Envoyer le message a la simulation via la queue thread-safe.
    netToSimQueue.push(message);
}

