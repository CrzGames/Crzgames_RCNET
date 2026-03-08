#include "simulation/queue_draining.h"

void ServerSimulationUpdate_DrainNetworkIncomingMessages(
    NetworkINToSimulationQueue& netToSimQueue,
    std::deque<NetworkINToSimulationMessage>& outMessages)
{
    // Drainer toute la queue réseau -> simulation
    // dans la deque locale fournie en sortie.
    netToSimQueue.drain(outMessages);
}

void ServerSimulationUpdate_DrainHttpIncomingMessages(
    HttpToSimulationQueue& httpToSimQueue,
    std::deque<HttpToSimulationMessage>& outMessages)
{
    // Drainer toute la queue HTTP -> simulation
    // dans la deque locale fournie en sortie.
    httpToSimQueue.drain(outMessages);
}