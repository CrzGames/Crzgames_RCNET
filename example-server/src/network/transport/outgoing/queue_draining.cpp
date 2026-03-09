#include "network/transport/outgoing/queue_draining.h"

void ServerNetworkOutgoing_DrainSimulationToNetworkOutgoingQueue(
    SimulationToNetworkOUTQueue& simToNetQueue,
    std::deque<SimulationToNetworkOUTMessage>& outMessages)
{
    // Drainer toute la queue simulation -> réseau sortant
    // dans la deque locale fournie en sortie.
    simToNetQueue.drain(outMessages);
}