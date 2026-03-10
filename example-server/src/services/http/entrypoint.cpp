#include "services/http/entrypoint.h"

#include "services/http/process/simulation_dispatcher.h"
#include "core/threading/queues/simulation_to_http.h"
#include "core/context.h"

void ServerHttp_ProcessSimulationMessage(void)
{
    SimulationToHttpQueue& simulationToHttpQueue = GetSimulationToHttpQueue();

    SimulationToHttpMessage message;
    if (!simulationToHttpQueue.waitAndPop(message))
    {
        return; // stop demandé
    }

    ServerHttp_ProcessSimulationDispatcher(message);
}