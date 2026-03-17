#include "network/transport/incoming/events/disconnect.h"

#include <cstdint> // uint32_t, uint64_t

void ClientNetworkIncoming_Event_HandleDisconnect(
    const ENetEvent* event,
    NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Vérifie que le peer associé à l'événement de déconnexion n'est pas nul.
    if (event->peer == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[CLIENT] [NETWORK_IN] [DISCONNECT] - Invalid disconnect event: event->peer == nullptr\n");
        return;
    }

    // Supprime le référence au peer du serveur dans l'état réseau, car la connexion est maintenant fermée.
    networkState.peerServer = nullptr;

    // Crée un message destiné au thread simulation.
    NetworkINToSimulationMessage message{};

    // Indique que ce message correspond à une déconnexion client.
    message.type = NetworkINToSimulationMessageType::SERVER_DISCONNECT_EVENT;

    // Envoie le message au thread simulation via la queue thread-safe. 
    // Le thread simulation traitera ce message pour mettre à jour son état en conséquence.
    netToSimQueue.push(message);

    // Log l'événement de déconnexion avec l'identifiant de connexion concerné.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [NETWORK_IN] [DISCONNECT] - Disconnect event received from server\n");
}
