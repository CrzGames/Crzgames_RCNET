#include "services/http/process/simulation/auth_signuprequest_message.h"

#include "services/http/types/auth/responses.h"
#include "core/context.h"
#include "services/http/requests/auth_signuprequest.h"

void ClientHttp_ProcessSimulationDispatcher_HandleAuthSignupRequestMessage(const SimulationToHttpMessage& simToHttpMessage)
{
    // Récupérer la queue de messages de http vers simulation pour pouvoir 
    // envoyer la réponse à la simulation une fois la requête HTTP traitée
    HttpToSimulationQueue& httpToSimulationQueue = GetHttpToSimulationQueue();

    // 1) faire la requête HTTP
    AuthSignupHTTPResponse response = ClientHttp_Auth_SignupRequest(simToHttpMessage.authSignupRequest);

    // 2) construire le message de retour de http vers simulation
    HttpToSimulationMessage httpToSimulationMessage{};
    httpToSimulationMessage.type = HttpToSimulationMessageType::AUTH_SIGNUP_RESPONSE;
    httpToSimulationMessage.authSignupResponse = std::move(response);

    // 3) push vers simulation
    httpToSimulationQueue.push(httpToSimulationMessage);
}