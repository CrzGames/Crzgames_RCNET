#include "services/nats/process/simulation_dispatcher.h"

#include "services/nats/process/simulation/publish_message_for_subject_mysubject1.h"
#include "services/nats/process/simulation/publish_message_for_subject_gameserver_simulation_etat_metrics.h"

void ServerNats_ProcessSimulationDispatcher(
    RCNET_NATSContext* natsContext,
    const SimulationToNatsMessage& message)
{
    switch (message.type)
    {
        case SimulationToNatsMessageType::PUBLISH_MESSAGE_FOR_SUBJECT_MYSUBJECT1:
        {
            ServerNats_ProcessSimulationDispatcher_HandlePublishMessageForSubjectMySubject1(natsContext, message);
            break;
        }

        case SimulationToNatsMessageType::PUBLISH_MESSAGE_FOR_SUBJECT_GAMESERVER_SIMULATION_ETAT_METRICS:
        {
            ServerNats_ProcessSimulationDispatcher_HandlePublishMessageForSubjectGameserverSimulationEtatMetrics(natsContext, message);
            break;
        }

        default:
            RCNET_log(
                RCNET_LOG_ERROR,
                "[SERVER] [NATS] - Received unknown SimulationToNatsMessageType: %d",
                static_cast<uint8_t>(message.type));
            break;
    }
}
