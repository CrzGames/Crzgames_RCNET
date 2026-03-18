#include "network/transport/incoming/events/disconnect.h"

#include <cstdint> // uint32_t, uint64_t

#include <RCNET/RCNET.h>

void ServerNetworkIncoming_Event_HandleDisconnect(
    const ENetEvent* event,
    NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Vérifie que le peer associé à l'événement de déconnexion n'est pas nul.
    if (event->peer == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [DISCONNECT] - Invalid disconnect event: event->peer == nullptr\n");
        return;
    }

    // Vérifie que les données associées au peer ne sont pas nulles.
    if (event->peer->data == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [DISCONNECT] - Invalid disconnect event: peer->data == nullptr\n");
        return;
    }

    // Récupère l'identifiant de connexion à partir des données du peer.
    const uint32_t connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));

    // Supprime l'entrée correspondante dans la map connectionIdToEnetPeer.
    networkState.connectionIdToEnetPeer.erase(connectionId);

    // Nettoie l'état de chiffrement réseau associé à cette connexion.
    networkState.connectionIdToEncryptionEnabled.erase(connectionId);

    // Réinitialise les données du peer à nullptr pour éviter toute utilisation ultérieure.
    event->peer->data = nullptr;

    // Crée un message destiné au thread simulation.
    NetworkINToSimulationMessage message{};

    // Indique que ce message correspond à une déconnexion client.
    message.type = NetworkINToSimulationMessageType::CLIENT_EVENT_DISCONNECT;
    // Associe l’ID de connexion qui vient d’être fermée.
    message.connectionId = connectionId;

    // Envoie le message au thread simulation via la queue thread-safe. 
    // Le thread simulation traitera ce message pour mettre à jour son état en conséquence.
    netToSimQueue.push(message);

    // Log l'événement de déconnexion avec l'identifiant de connexion concerné.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [NETWORK_IN] [DISCONNECT] - connectionId: %u - Transport disconnected\n",
              connectionId);
}
