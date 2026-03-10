#include "services/http/process/simulation/auth_validatetokenrequest_message.h"

#include "services/http/types/auth/responses.h"
#include "core/context.h"
#include "services/http/requests/auth_validatetokenrequest.h"

void ServerHttp_ProcessSimulationDispatcher_HandleAuthValidateTokenRequestMessage(const SimulationToHttpMessage& message)
{
    // Récupérer la queue de messages de http vers simulation pour pouvoir 
    // envoyer la réponse à la simulation une fois la requête HTTP traitée
    HttpToSimulationQueue& httpToSimulationQueue = GetHttpToSimulationQueue();

    // 1) faire la requête HTTP
    AuthTokenVerificationHTTPResponse response = ServerHttp_Auth_ValidateTokenRequest(message.authTokenVerificationRequest);

    // 2) construire le message de retour vers simulation
    HttpToSimulationMessage out{};
    out.type = HttpToSimulationMessageType::AUTH_VALIDATE_TOKEN_RESPONSE;
    out.connectionId = message.connectionId;
    out.authTokenVerificationResponse = std::move(response);;

    // 3) push vers simulation
    httpToSimulationQueue.push(out);
}