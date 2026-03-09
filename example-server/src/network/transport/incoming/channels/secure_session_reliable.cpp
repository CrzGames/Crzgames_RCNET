#include "network/transport/incoming/channels/secure_session_reliable.h"

#include "network/packets/client/reliable.h"
#include "network/protocol/version.h"
#include "network/serialization/deserialize_packets_client.h"

#include <RCNET/RCNET.h>

void ServerNetworkIncoming_Channel_SecureSessionReliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de désérialiser le packet envoyé par le client.
    ClientSecureSessionHelloPacketReliable secureSessionHelloPacket{};
    if (!deserializeClientSecureSessionHelloPacketReliable(
            event->packet->data,
            event->packet->dataLength,
            secureSessionHelloPacket))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet de handshake est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [SECURE_SESSION] - Failed to deserialize secure session packet from connectionId=%u\n",
                  connectionId);
        return;
    }

    // Log d’information indiquant que le packet de session sécurisée a été reçu avec succès.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [NETWORK_IN] [SECURE_SESSION] - Packet received from connectionId=%u (size=%u bytes)\n",
              connectionId,
              (unsigned)event->packet->dataLength);

    // Vérifie que la version du protocole réseau du client est compatible avec celle du serveur.
    if (secureSessionHelloPacket.networkProtocolVersion != SERVER_NETWORK_PROTOCOL_VERSION)
    {
        // Log d’erreur indiquant un mismatch de version.
        RCNET_log(RCNET_LOG_ERROR,
                  "[SERVER] [NETWORK_IN] [SECURE_SESSION] - Network protocol version mismatch with connectionId=%u: client=%u vs server=%u. Disconnecting client.\n",
                  connectionId,
                  secureSessionHelloPacket.networkProtocolVersion,
                  SERVER_NETWORK_PROTOCOL_VERSION);

        // Déconnecte immédiatement le client.
        enet_peer_disconnect(event->peer, 0);

        // Abandonne le traitement de ce packet, car le client n’est pas compatible.
        return;
    }

    // Crée un message destiné à la simulation.
    NetworkINToSimulationMessage message{};

    // Indique que ce message transporte un handshake reliable.
    message.type = NetworkINToSimulationMessageType::CLIENT_SECURE_SESSION_HELLO_PACKET_RELIABLE;
    // Attache le connectionId source.
    message.connectionId = connectionId;
    // Attache le packet de secure session reçu au message.
    message.secureSessionHelloPacket = secureSessionHelloPacket;

    // Envoie le message à la simulation via la queue thread-safe.
    netToSimQueue.push(message);
}