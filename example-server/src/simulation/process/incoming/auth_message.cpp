#include "simulation/process/incoming/auth_message.h"

#include "auth/types.h"

#include <mutex>         // std::lock_guard
#include <unordered_map> // std::unordered_map

#include <RCNET/RCNET.h>

void ServerSimulation_ProcessNetworkIncomingDispatcher_HandleAuthMessage(
    NetworkState& networkState,
    SimulationToHttpQueue& simToHttpQueue,
    const NetworkINToSimulationMessage& msg)
{
    {
        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

        // Rechercher la session correspondant à cette connexion.
        std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);

        // Si la session n'existe pas, on ne peut pas traiter l'auth.
        if (sit == networkState.sessions.end())
        {
            // Log d'avertissement pour signaler une connexion inconnue.
            RCNET_log(RCNET_LOG_WARN,
                      "[SERVER] [SIMULATION] [AUTH] - connectionId: %u - Session not found\n",
                      msg.connectionId);

            // Abandon du traitement.
            return;
        }

        // Référence directe vers la session trouvée.
        ClientSession& session = sit->second;

        // Si l'auth a deja ete invalidee, ignorer toute nouvelle tentative.
        if (session.authStatus == AuthStatus::Invalid)
        {
            RCNET_log(RCNET_LOG_INFO,
                      "[SERVER] [SIMULATION] [AUTH] - connectionId: %u - Auth already invalid, ignoring new request\n",
                      msg.connectionId);
            return;
        }

        // Si l'utilisateur est déjà authentifié, il n'y a rien à faire.
        if (session.authStatus == AuthStatus::Valid)
        {
            // Log d'information indiquant que l'auth existe déjà.
            RCNET_log(RCNET_LOG_INFO,
                      "[SERVER] [SIMULATION] [AUTH] - connectionId: %u - Already authenticated\n",
                      msg.connectionId);

            // Abandon du traitement.
            return;
        }

        // Si une auth est déjà en attente, ne pas spammer le backend.
        if (session.authStatus == AuthStatus::WaitingAuth)
        {
            // Log d'information indiquant qu'une demande est déjà en cours.
            RCNET_log(RCNET_LOG_INFO,
                      "[SERVER] [SIMULATION] [AUTH] - connectionId: %u - Auth already pending\n",
                      msg.connectionId);

            // Abandon du traitement.
            return;
        }

        // Marquer la session comme en attente de validation backend.
        session.authStatus = AuthStatus::WaitingAuth;
    }

    // Construire le message à envoyer au thread HTTP.
    SimulationToHttpMessage httpMessage{};

    // Renseigner le type logique de la requête HTTP.
    httpMessage.type = SimulationToHttpMessageType::AUTH_VALIDATE_TOKEN_REQUEST;

    // Associer la connexion source.
    httpMessage.connectionId = msg.connectionId;

    // Copier le token à valider.
    httpMessage.authTokenVerificationRequest.authToken = msg.authPacket.authToken;

    // Envoyer la requête vers le thread HTTP.
    simToHttpQueue.push(httpMessage);

    // Log d'information confirmant l'envoi de la demande d'authentification.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [AUTH] - connectionId: %u - Auth validation request sent to HTTP thread\n",
              msg.connectionId);
}
