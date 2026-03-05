#include "server_callbacks.h"
#include "server_network_incoming_update.h"
#include "server_network_outgoing_update.h"
#include "server_simulation_update.h"

void rcnet_load(void)
{
}

void rcnet_unload(void)
{
}

void rcnet_network_incoming_update(ENetHost* host, const ENetEvent* event)
{
    ServerNetworkIncomingUpdate_ProcessENetEvent(host, event);
}

void rcnet_network_outgoing_update(ENetHost* host)
{
    ServerNetworkOutgoingUpdate_DrainCoalesceAndSendMessages(host);
}

void rcnet_simulation_update(uint64_t currentTick)
{
    ServerSimulationUpdate_RunFullSimulationPipelineForCurrentTick(currentTick);
}