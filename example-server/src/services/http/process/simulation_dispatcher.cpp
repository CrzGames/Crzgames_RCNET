#include "services/http/process/simulation_dispatcher.h"

#include "services/http/process/simulation/auth_validatetokenrequest_message.h"

#include <RCNET/RCNET.h>

void ServerHttp_ProcessSimulationDispatcher(const SimulationToHttpMessage& message)
{
    switch (message.type)
    {
        case SimulationToHttpMessageType::AUTH_VALIDATE_TOKEN_REQUEST:
            ServerHttp_ProcessSimulationDispatcher_HandleAuthValidateTokenRequestMessage(message);
            break;

        default:
            RCNET_log(RCNET_LOG_ERROR, "Received unknown SimulationToHttpMessageType: %d\n", static_cast<uint8_t>(message.type));
            break;
    }
}