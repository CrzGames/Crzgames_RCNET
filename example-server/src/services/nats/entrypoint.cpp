#include "services/nats/entrypoint.h"

#include "core/context.h"
#include "core/threading/queues/simulation_to_nats.h"
#include "services/nats/process/simulation_dispatcher.h"
#include "services/nats/subscriptions.h"

#include <deque>

void ServerNats_WaitAndProcessOneSimulationMessage_And_RunNatsLogic(RCNET_NATSContext* natsContext)
{
    if (natsContext == nullptr)
    {
        RCNET_log(RCNET_LOG_ERROR, "[SERVER] [NATS] - natsContext is null in simulation dispatcher");
        return;
    }

    static bool subscriptionsInitialized = false;
    if (!subscriptionsInitialized)
    {
        if (!ServerNats_SubscribeAllSubjects(natsContext))
        {
            RCNET_log(RCNET_LOG_ERROR, "[SERVER] [NATS] - Failed to initialize subscriptions");
            return;
        }

        subscriptionsInitialized = true;
    }

    // Recupere la reference vers la queue qui transporte les jobs
    // envoyes par le thread simulation vers le thread NATS.
    SimulationToNatsQueue& simulationToNatsQueue = GetSimulationToNatsQueue();

    // Declare le buffer local qui recevra un batch de jobs a traiter.
    std::deque<SimulationToNatsMessage> batch;

    // Attend de facon bloquante qu'au moins un message soit disponible,
    // puis draine tous les messages en attente en une seule prise de lock.
    // - Retourne true si un batch a bien ete recupere.
    // - Retourne false si la queue a ete arretee (stop demande au shutdown).
    if (!simulationToNatsQueue.waitAndDrain(batch))
    {
        // Si le wait s'arrete parce qu'on shutdown,
        // on quitte simplement cette iteration du thread NATS.
        return;
    }

    // Le traitement est fait hors lock pour minimiser la contention
    // avec le thread simulation qui pousse de nouveaux jobs.
    while (!batch.empty())
    {
        SimulationToNatsMessage message = std::move(batch.front());
        batch.pop_front();

        // Le dispatcher choisit le bon traitement selon message.type.
        ServerNats_ProcessSimulationDispatcher(natsContext, message);
    }
}
