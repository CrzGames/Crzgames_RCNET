#include "services/http/process/simulation_dispatcher.h"

#include "services/http/process/simulation/auth_validatetokenrequest_message.h"

#include <RCNET/RCNET.h>

void ClientHttp_ProcessSimulationDispatcher(const SimulationToHttpMessage& simToHttpMessage)
{
    switch (simToHttpMessage.type)
    {
        case SimulationToHttpMessageType::AUTH_SIGNUP_REQUEST:
            ClientHttp_ProcessSimulationDispatcher_HandleAuthSignupRequestMessage(simToHttpMessage);
            break;

        case SimulationToHttpMessageType::AUTH_SIGNIN_REQUEST:
            ClientHttp_ProcessSimulationDispatcher_HandleAuthSigninRequestMessage(simToHttpMessage);
            break;

        default:
            RCNET_log(RCNET_LOG_ERROR, "Received unknown SimulationToHttpMessageType: %d\n", static_cast<uint8_t>(simToHttpMessage.type));
            break;
    }
}