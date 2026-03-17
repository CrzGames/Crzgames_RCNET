#include "simulation/process/http_dispatcher.h"

#include "simulation/process/http/auth_signup_response_message.h"
#include "simulation/process/http/auth_signin_response_message.h"

void ClientSimulation_ProcessHttpDispatcher(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const std::deque<HttpToSimulationMessage>& httpToSimulationMessages)
{
    // Parcourir tous les messages provenant du thread HTTP
    // qui ont été drainés pendant ce tick.
    for (std::deque<HttpToSimulationMessage>::const_iterator it = httpToSimulationMessages.begin();
         it != httpToSimulationMessages.end();
         ++it)
    {
        // Référence directe vers le message HTTP courant.
        const HttpToSimulationMessage& msg = *it;

        // Dispatch du traitement selon le type de message HTTP reçu.
        if (msg.type == HttpToSimulationMessageType::AUTH_SIGNUP_RESPONSE)
        {
            // Traiter la réponse du backend à notre requête d'inscription.
            ClientSimulation_ProcessHttpDispatcher_HandleAuthSignupResponseMessage(
                networkState,
                simToNetQueue,
                msg);
        }
        else if (msg.type == HttpToSimulationMessageType::AUTH_SIGNIN_RESPONSE)
        {
            // Traiter la réponse du backend à notre requête de connexion.
            ClientSimulation_ProcessHttpDispatcher_HandleAuthSigninResponseMessage(
                networkState,
                simToNetQueue,
                msg);
        }
        else
        {
            RCNET_log(RCNET_LOG_ERROR, "Received unknown HttpToSimulationMessageType: %d\n", static_cast<uint8_t>(msg.type));
        }
    }
}
