#include "simulation/queue_draining.h"

void ServerSimulation_DrainNetworkIncomingToSimulationMessages(
    NetworkINToSimulationQueue& netToSimQueue,
    std::deque<NetworkINToSimulationMessage>& outMessages)
{
    // Drainer toute la queue réseau -> simulation
    // dans la deque locale fournie en sortie.
    netToSimQueue.drain(outMessages);
}

void ServerSimulation_DrainHttpToSimulationMessages(
    HttpToSimulationQueue& httpToSimQueue,
    std::deque<HttpToSimulationMessage>& outMessages)
{
    // Drainer toute la queue HTTP -> simulation
    // dans la deque locale fournie en sortie.
    httpToSimQueue.drain(outMessages);
}

void ServerSimulation_DrainNatsToSimulationMessages(
    NatsToSimulationQueue& natsToSimQueue,
    std::deque<NatsToSimulationMessage>& outMessages)
{
    // Drainer toute la queue NATS -> simulation
    // dans la deque locale fournie en sortie.
    natsToSimQueue.drain(outMessages);
}
