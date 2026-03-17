#include "network/transport/incoming/channels/secure_session_reliable.h"

#include "network/packets/server/reliable.h"
#include "network/serialization/deserialize_packets_server.h"

void ClientNetworkIncoming_Channel_SecureSessionReliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de désérialiser le packet envoyé par le serveur.
    ServerSecureSessionHelloResponsePacketReliable secureSessionHelloResponsePacket{};
    if (!deserializeServerSecureSessionHelloResponsePacketReliable(
            event->packet->data,
            event->packet->dataLength,
            secureSessionHelloResponsePacket))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet de handshake est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [SECURE_SESSION] - Failed to deserialize secure session packet from server\n");
        return;
    }

    // Log d’information indiquant que le packet de session sécurisée a été reçu avec succès.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [NETWORK_IN] [SECURE_SESSION] - Packet received from connectionId=%u (size=%u bytes)\n",
              connectionId,
              (unsigned)event->packet->dataLength);

    // Crée un message destiné à la simulation.
    NetworkINToSimulationMessage message{};

    // Indique que ce message transporte un handshake reliable.
    message.type = NetworkINToSimulationMessageType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE;
    // Attache le packet de secure session reçu au message.
    message.secureSessionHelloResponsePacket = secureSessionHelloResponsePacket;

    // Envoie le message à la simulation via la queue thread-safe.
    netToSimQueue.push(message);
}