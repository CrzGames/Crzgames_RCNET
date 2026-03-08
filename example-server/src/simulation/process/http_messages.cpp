#include "simulation/process/http_messages.h"

#include "simulation/process/http/auth_validate_token_response_message.h"

void ServerSimulationUpdate_ProcessHttpMessages(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    std::deque<HttpToSimulationMessage>& httpMessages)
{
    // Parcourir tous les messages provenant du thread HTTP
    // qui ont été drainés pendant ce tick.
    for (std::deque<HttpToSimulationMessage>::iterator it = httpMessages.begin();
         it != httpMessages.end();
         ++it)
    {
        // Référence directe vers le message HTTP courant.
        HttpToSimulationMessage& msg = *it;

        // Dispatch du traitement selon le type de message HTTP reçu.
        if (msg.type == HttpToSimulationMessageType::AUTH_VALIDATE_TOKEN_RESPONSE)
        {
            // Traiter la réponse backend de validation de token.
            ServerSimulationUpdate_ProcessHttpMessages_HandleAuthValidateTokenResponseMessage(
                networkState,
                simToNetQueue,
                msg);
        }
    }
}