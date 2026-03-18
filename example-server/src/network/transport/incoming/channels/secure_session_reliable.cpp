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
                  "[SERVER] [NETWORK_IN] [SECURE_SESSION] - connectionId: %u - Failed to deserialize secure-session hello packet\n",
                  connectionId);
        return;
    }

    // Log d’information indiquant que le packet de session sécurisée a été reçu avec succès.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [NETWORK_IN] [SECURE_SESSION] - connectionId: %u - Secure-session hello packet received (size: %u bytes)\n",
              connectionId,
              (unsigned)event->packet->dataLength);

    // Vérifie que la version du protocole réseau du client est compatible avec celle du serveur.
    if (secureSessionHelloPacket.networkProtocolVersion != SERVER_NETWORK_PROTOCOL_VERSION)
    {
        // Log d’erreur indiquant un mismatch de version.
        const uint16_t clientVersionMajor =
            static_cast<uint16_t>((secureSessionHelloPacket.networkProtocolVersion >> 16) & 0xFFFFu);
        const uint8_t clientVersionMinor =
            static_cast<uint8_t>((secureSessionHelloPacket.networkProtocolVersion >> 8) & 0xFFu);
        const uint8_t clientVersionPatch =
            static_cast<uint8_t>(secureSessionHelloPacket.networkProtocolVersion & 0xFFu);

        const uint16_t serverVersionMajor =
            static_cast<uint16_t>((SERVER_NETWORK_PROTOCOL_VERSION >> 16) & 0xFFFFu);
        const uint8_t serverVersionMinor =
            static_cast<uint8_t>((SERVER_NETWORK_PROTOCOL_VERSION >> 8) & 0xFFu);
        const uint8_t serverVersionPatch =
            static_cast<uint8_t>(SERVER_NETWORK_PROTOCOL_VERSION & 0xFFu);

        RCNET_log(
            RCNET_LOG_ERROR,
            "[SERVER] [NETWORK_IN] [SECURE_SESSION] - connectionId: %u - Network protocol version mismatch (client=%u.%u.%u, server=%u.%u.%u). Disconnecting client.\n",
            connectionId,
            static_cast<unsigned>(clientVersionMajor),
            static_cast<unsigned>(clientVersionMinor),
            static_cast<unsigned>(clientVersionPatch),
            static_cast<unsigned>(serverVersionMajor),
            static_cast<unsigned>(serverVersionMinor),
            static_cast<unsigned>(serverVersionPatch));

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
