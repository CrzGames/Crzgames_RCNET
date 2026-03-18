#include "simulation/process/incoming/disconnect_message.h"

#include <mutex>         // std::lock_guard
#include <unordered_map> // std::unordered_map

#include <RCNET/RCNET.h>

void ServerSimulation_ProcessNetworkIncomingDispatcher_HandleDisconnectMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& msg)
{
    {
        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

        // Rechercher la session correspondant à cette connexion.
        std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);

        // Si aucune session n'existe, on ignore simplement le message.
        if (sit == networkState.sessions.end())
        {
            // Log d'avertissement pour signaler une déconnexion inconnue.
            RCNET_log(RCNET_LOG_WARN,
                      "[SERVER] [SIMULATION] [DISCONNECT] - connectionId: %u - Disconnect received for unknown session (ignored)\n",
                      msg.connectionId);

            // Abandon du traitement.
            return;
        }

        // Supprimer la session du tableau des sessions actives.
        networkState.sessions.erase(sit);
    }

    // TODO :
    // - nettoyer les ressources gameplay liées à cette session
    // - notifier les autres joueurs si nécessaire
    // - retirer une éventuelle entité monde associée

    // Log d'information confirmant la déconnexion.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [DISCONNECT] - connectionId: %u - Deleted session\n",
              msg.connectionId);
}
