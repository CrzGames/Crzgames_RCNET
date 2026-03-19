#include "services/http/entrypoint.h"

#include "services/http/process/simulation_dispatcher.h"
#include "core/threading/queues/simulation_to_http.h"
#include "core/context.h"

#include <deque>

void ServerHttp_WaitAndProcessOneSimulationMessage_And_RunHttpLogic(void)
{
    // Récupère la référence vers la queue qui transporte les jobs
    // envoyés par le thread simulation vers le thread HTTP.
    SimulationToHttpQueue& simulationToHttpQueue = GetSimulationToHttpQueue();

    // Déclare le buffer local qui recevra un batch de jobs à traiter.
    std::deque<SimulationToHttpMessage> batch;

    // Attend de façon bloquante qu'au moins un message soit disponible,
    // puis draine tous les messages en attente en une seule prise de lock.
    // - Retourne true si un batch a bien été récupéré.
    // - Retourne false si la queue a été arrêtée (stop demandé au shutdown).
    if (!simulationToHttpQueue.waitAndDrain(batch))
    {
        // Si le wait s'arrête parce qu'on shutdown,
        // on quitte simplement cette itération du thread HTTP.
        return;
    }

    // Le traitement est fait hors lock pour minimiser la contention
    // avec le thread simulation qui pousse de nouveaux jobs.
    while (!batch.empty())
    {
        SimulationToHttpMessage message = std::move(batch.front());
        batch.pop_front();

        // Le dispatcher choisit le bon traitement selon message.type.
        ServerHttp_ProcessSimulationDispatcher(message);
    }
}