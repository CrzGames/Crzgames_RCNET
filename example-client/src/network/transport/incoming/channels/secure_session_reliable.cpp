#include "network/transport/incoming/channels/secure_session_reliable.h"

#include "network/serialization/deserialize_packets_server.h"

#include <RC2D/RC2D.h>

void ClientNetworkIncoming_Channel_SecureSessionReliable(
    const ENetEvent* event,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de deserialiser la reponse secure-session du serveur.
    ServerSecureSessionHelloResponsePacketReliable secureSessionHelloResponsePacket{};
    if (!deserializeServerSecureSessionHelloResponsePacketReliable(
            event->packet->data,
            event->packet->dataLength,
            secureSessionHelloResponsePacket))
    {
        RC2D_log(
            RC2D_LOG_WARN,
            "[CLIENT] [NETWORK_IN] [SECURE_SESSION] Failed to deserialize secure-session response (size=%u).",
            static_cast<unsigned>(event->packet->dataLength));
        return;
    }

    // Construire le message destination simulation.
    NetworkINToSimulationMessage message{};
    message.type = NetworkINToSimulationMessageType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE;
    message.secureSessionHelloResponsePacket = secureSessionHelloResponsePacket;

    // Push vers la queue simulation.
    netToSimQueue.push(message);
}

