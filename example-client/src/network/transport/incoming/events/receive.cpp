#include "network/transport/incoming/events/receive.h"

#include "network/transport/incoming/dispatch_by_channel.h"

void ServerNetworkIncoming_Event_HandleReceive(
    const ENetEvent* event,
    const NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Dispatch le traitement du packet selon le channel ENet utilisé.
    ServerNetworkIncoming_DispatchByChannel(event, netToSimQueue);
}
