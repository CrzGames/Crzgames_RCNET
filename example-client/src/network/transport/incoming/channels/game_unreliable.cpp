#include "network/transport/incoming/channels/game_unreliable.h"

#include "network/transport/incoming/packet_type_reader.h"

void ClientNetworkIncoming_Channel_GameUnreliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de lire le type de packet non fiable envoyé par le client.
    ServerUnreliablePacketType packetType{};
    if (!ClientNetworkIncoming_ReadServerUnreliablePacketType(event, packetType))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet non fiable est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [UNRELIABLE] - Failed to read unreliable packet type from server\n");
        return;
    }

    // Dispatch le traitement selon le type de packet non fiable reçu.
    switch (packetType)
    {
        // Ajouter ici les cas pour les différents types de packets non fiables attendus sur ce channel.

        default:
            // Si le type de packet est inconnu ou inattendu, log d’avertissement.
            RCNET_log(RCNET_LOG_WARN,
                      "[SERVER] [NETWORK_IN] [UNRELIABLE] - Unknown or unexpected unreliable packet type=%u from server\n",
                      static_cast<unsigned>(packetType));
            break;
    }
}