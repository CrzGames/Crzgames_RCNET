#include "network/transport/incoming/events/connect.h"

#include <cstdint> // uintptr_t
#include <RC2D/RC2D.h>

void ClientNetworkIncoming_Event_HandleConnect(
    const ENetEvent* event,
    NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Vérifie que le peer associé à l'événement de connexion n'est pas nul.
    if (event->peer == nullptr)
    {
        // Log un avertissement si l'événement de connexion est invalide (peer nul) et retourne sans faire d'autres traitements.
        RC2D_log(RC2D_LOG_WARN,
                 "[CLIENT] [NETWORK_IN] [CONNECT] Invalid connect event: event->peer == nullptr.");
        return;
    }

    // Stocke une référence du peer du serveur dans l'état réseau pour pouvoir l'utiliser ultérieurement 
    // lors de l'envoi de messages au serveur.
    networkState.peerServer = event->peer;

    // Crée un message destiné au thread simulation.
    NetworkINToSimulationMessage message{};

    // Spécifier le type de message pour que la simulation sache comment le traiter.
    message.type = NetworkINToSimulationMessageType::SERVER_EVENT_CONNECT;

    // Envoie le message au thread simulation via la queue thread-safe.
    netToSimQueue.push(message);

    // Log l'événement de connexion avec l'identifiant de connexion concerné.
    RC2D_log(RC2D_LOG_INFO, "[CLIENT] [NETWORK_IN] [CONNECT] Connected from server - address: %s port: %u.",
             event->peer->address.host, event->peer->address.port);
}
