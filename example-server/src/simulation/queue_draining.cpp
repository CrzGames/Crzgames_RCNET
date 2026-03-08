#include "simulation/queue_draining.h"

void ServerSimulationUpdate_DrainNetworkIncomingToSimulationMessages(
    NetworkINToSimulationQueue& netToSimQueue,
    std::deque<NetworkINToSimulationMessage>& outMessages)
{
    // Drainer toute la queue réseau -> simulation
    // dans la deque locale fournie en sortie.
    netToSimQueue.drain(outMessages);
}

void ServerSimulationUpdate_DrainHttpToSimulationMessages(
    HttpToSimulationQueue& httpToSimQueue,
    std::deque<HttpToSimulationMessage>& outMessages)
{
    // Drainer toute la queue HTTP -> simulation
    // dans la deque locale fournie en sortie.
    httpToSimQueue.drain(outMessages);
}