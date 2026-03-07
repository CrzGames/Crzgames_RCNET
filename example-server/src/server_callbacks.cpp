#include "server_callbacks.h"
#include "server_network_incoming_update.h"
#include "server_network_outgoing_update.h"
#include "server_simulation_update.h"
#include "server_debug_network_stats.h"
#include "server_crypto_kx.h"

void rcnet_load(void)
{
    NetworkState& networkState = GetNetworkState();
    if (!ServerCryptoKx_Initialize(networkState.cryptoKxState))
    {
        // fatal error
    }
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
    ServerDebugNetworkStats_OnNetworkOutTick();
    ServerNetworkOutgoingUpdate_DrainCoalesceAndSendMessages(host);
}

void rcnet_simulation_update(uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt)
{
    ServerDebugNetworkStats_OnSimulationTick();
    ServerSimulationUpdate_RunFullSimulationPipelineForCurrentTick(currentTick, serverTimeNs, dtNs, dt);
}