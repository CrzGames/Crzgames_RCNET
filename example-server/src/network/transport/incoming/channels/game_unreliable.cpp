#include "network/transport/incoming/channels/game_unreliable.h"

#include "network/transport/incoming/packet_type_reader.h"
#include "network/transport/incoming/packets/input.h"

#include <RCNET/RCNET.h>

void ServerNetworkIncoming_Channel_GameUnreliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de lire le type de packet non fiable envoyé par le client.
    ClientUnreliablePacketType packetType{};
    if (!ServerNetworkIncoming_ReadClientUnreliablePacketType(event, packetType))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet non fiable est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [UNRELIABLE] - Failed to read unreliable packet type from connectionId=%u\n",
                  connectionId);
        return;
    }

    // Dispatch le traitement selon le type de packet non fiable reçu.
    switch (packetType)
    {
        case ClientUnreliablePacketType::CLIENT_INPUT_PACKET_UNRELIABLE:
            ServerNetworkIncoming_HandlePacket_Input(
                event, connectionId, netToSimQueue);
            break;

        default:
            // Si le type de packet est inconnu ou inattendu, log d’avertissement.
            RCNET_log(RCNET_LOG_WARN,
                      "[SERVER] [NETWORK_IN] [UNRELIABLE] - Unknown or unexpected unreliable packet type=%u from connectionId=%u\n",
                      static_cast<unsigned>(packetType),
                      connectionId);
            break;
    }
}