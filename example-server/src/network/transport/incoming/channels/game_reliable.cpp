#include "network/transport/incoming/channels/game_reliable.h"

#include "network/transport/incoming/packet_type_reader.h"
#include "network/transport/incoming/packets/ready_for_match.h"

#include <RCNET/RCNET.h>

void ServerNetworkIncoming_Channel_GameReliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de lire le type de packet fiable envoyé par le client.
    ClientReliablePacketType packetType{};
    if (!ServerNetworkIncoming_ReadClientReliablePacketType(event, packetType))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet fiable est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [RELIABLE] - Failed to read reliable packet type from connectionId=%u\n",
                  connectionId);
        return;
    }

    // Dispatch le traitement selon le type de packet fiable reçu.
    switch (packetType)
    {
        case ClientReliablePacketType::CLIENT_READY_FOR_MATCH_PACKET_RELIABLE:
            ServerNetworkIncoming_HandlePacket_ReadyForMatch(
                event, connectionId, netToSimQueue);
            break;

        default:
            // Si le type de packet est inconnu ou inattendu, log d’avertissement.
            RCNET_log(RCNET_LOG_WARN,
                      "[SERVER] [NETWORK_IN] [RELIABLE] - Unknown or unexpected reliable packet type=%u from connectionId=%u\n",
                      static_cast<unsigned>(packetType),
                      connectionId);
            break;
    }
}