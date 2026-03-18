#include "simulation/process/incoming/connect_message.h"

#include <mutex> // std::lock_guard

#include <RCNET/RCNET.h>

void ServerSimulation_ProcessNetworkIncomingDispatcher_HandleConnectMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& msg)
{
    // Enregistrer la session dans l'état réseau global.
    {
        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

        // Créer une nouvelle session vide.
        ClientSession session{};

        // Assigner l'identifiant de connexion porté par le message.
        session.connectionId = msg.connectionId;

        // Marquer la couche transport comme connectée.
        session.isTransportConnected = true;

        // Enregistrer la session dans l'état réseau global.
        networkState.sessions[msg.connectionId] = session;
    }

    // Log d'information pour indiquer que la session a bien été créée.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [CONNECT] - connectionId: %u - Session created\n",
              msg.connectionId);
}
