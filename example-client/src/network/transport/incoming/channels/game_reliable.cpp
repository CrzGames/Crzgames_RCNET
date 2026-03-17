#include "network/transport/incoming/channels/game_reliable.h"

#include "network/transport/incoming/packet_type_reader.h"

void ClientNetworkIncoming_Channel_GameReliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de lire le type de packet fiable envoyé par le serveur.
    ServerReliablePacketType packetType{};
    if (!ClientNetworkIncoming_ReadServerReliablePacketType(event, packetType))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet fiable est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [RELIABLE] - Failed to read reliable packet type from server\n");
        return;
    }

    // Dispatch le traitement selon le type de packet fiable reçu.
    switch (packetType)
    {
        // Ajouter ici les cas pour les différents types de packets fiables attendus sur ce channel.

        default:
            // Si le type de packet est inconnu ou inattendu, log d’avertissement.
            RCNET_log(RCNET_LOG_WARN,
                      "[SERVER] [NETWORK_IN] [RELIABLE] - Unknown or unexpected reliable packet type=%u from server\n",
                      static_cast<unsigned>(packetType));
            break;
    }
}