#include "services/nats/entrypoint.h"

#include "core/context.h"
#include "core/threading/queues/simulation_to_nats.h"
#include "services/nats/process/simulation_dispatcher.h"
#include "services/nats/subscriptions.h"

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

    // Declare l'objet qui recevra le prochain message a traiter.
    SimulationToNatsMessage message;

    // Attend de facon bloquante qu'un message soit disponible dans la queue.
    // - Retourne true si un message a bien ete recupere.
    // - Retourne false si la queue a ete arretee (stop demande au shutdown).
    if (!simulationToNatsQueue.waitAndPop(message))
    {
        // Si le wait s'arrete parce qu'on shutdown,
        // on quitte simplement cette iteration du thread NATS.
        return;
    }

    // Une fois le message recupere, on le transmet au dispatcher NATS.
    // Le dispatcher choisira le bon traitement selon message.type.
    ServerNats_ProcessSimulationDispatcher(natsContext, message);
}
