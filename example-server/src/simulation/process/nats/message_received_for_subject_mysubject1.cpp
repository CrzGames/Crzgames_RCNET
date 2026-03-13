#include "simulation/process/nats/message_received_for_subject_mysubject1.h"

void ServerSimulation_ProcessNatsDispatcher_HandleMessageReceivedForSubjectMySubject1(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const NatsToSimulationMessage& natsMessage)
{
    // Ici, on peut faire du traitement métier avec le message reçu depuis NATS.
    // Par exemple, on peut vouloir retransmettre ce message à tous les clients connectés.
}