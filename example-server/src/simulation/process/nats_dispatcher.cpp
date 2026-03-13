#include "simulation/process/nats_dispatcher.h"

#include "simulation/process/nats/message_received_for_subject_mysubject1.h"

#include <RCNET/RCNET.h>

void ServerSimulation_ProcessNatsDispatcher(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const std::deque<NatsToSimulationMessage>& natsMessages)
{
    // Parcourir tous les messages provenant du thread NATS
    // qui ont Ã©tÃ© drainÃ©s pendant ce tick.
    for (std::deque<NatsToSimulationMessage>::const_iterator it = natsMessages.begin();
         it != natsMessages.end();
         ++it)
    {
        // RÃ©fÃ©rence directe vers le message NATS courant.
        const NatsToSimulationMessage& msg = *it;   

        // Dispatch du traitement selon le type de message NATS reÃ§u.
        switch (msg.type)
        {
            case NatsToSimulationMessageType::MESSAGE_RECEIVED_FOR_SUBJECT_MYSUBJECT1:
                ServerSimulation_ProcessNatsDispatcher_HandleMessageReceivedForSubjectMySubject1(networkState, simToNetQueue, msg);
                break;

            default:
                RCNET_log(RCNET_LOG_ERROR, "Received unknown NatsToSimulationMessageType: %d", static_cast<uint8_t>(msg.type));
                break;
        }
    }
}


