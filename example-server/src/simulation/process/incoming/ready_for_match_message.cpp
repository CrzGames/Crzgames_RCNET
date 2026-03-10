#include "simulation/process/incoming/ready_for_match_message.h"

#include <mutex>         // std::lock_guard
#include <unordered_map> // std::unordered_map

#include <RCNET/RCNET.h>

void ServerSimulation_ProcessNetworkIncomingDispatcher_HandleReadyForMatchMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& msg)
{
    {
        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

        // Rechercher la session correspondant à cette connexion.
        std::unordered_map<uint32_t, ClientSession>::iterator sit =
            networkState.sessions.find(msg.connectionId);

        // Si la session n'existe pas, le message ne peut pas être traité.
        if (sit == networkState.sessions.end())
        {
            // Log d'avertissement pour signaler une connexion inconnue.
            RCNET_log(RCNET_LOG_WARN,
                      "[SERVER] [SIMULATION] [READY_FOR_MATCH] - Unknown connectionId=%u\n",
                      msg.connectionId);

            // Abandon du traitement.
            return;
        }

        // Référence directe vers la session concernée.
        ClientSession& session = sit->second;

        // Marquer cette session comme prête pour le match.
        session.isReadyForMatch = true;
    }

    // Log d'information confirmant l'état prêt.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [READY_FOR_MATCH] - connectionId=%u\n",
              msg.connectionId);
}
