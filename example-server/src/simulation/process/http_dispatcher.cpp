#include "simulation/process/http_dispatcher.h"

#include "simulation/process/http/auth_validatetokenresponse_message.h"

#include <RCNET/RCNET.h>

void ServerSimulation_ProcessHttpDispatcher(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const std::deque<HttpToSimulationMessage>& httpMessages)
{
    // Parcourir tous les messages provenant du thread HTTP
    // qui ont été drainés pendant ce tick.
    for (std::deque<HttpToSimulationMessage>::const_iterator it = httpMessages.begin();
         it != httpMessages.end();
         ++it)
    {
        // Référence directe vers le message HTTP courant.
        const HttpToSimulationMessage& msg = *it;

        // Dispatch du traitement selon le type de message HTTP reçu.
        if (msg.type == HttpToSimulationMessageType::AUTH_VALIDATE_TOKEN_RESPONSE)
        {
            // Traiter la réponse backend de validation de token.
            ServerSimulation_ProcessHttpDispatcher_HandleAuthValidateTokenResponseMessage(
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
