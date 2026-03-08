#include "server_callbacks.h"
#include "server_network_incoming_update.h"
#include "server_network_outgoing_update.h"
#include "server_simulation_update.h"
#include "server_debug_network_stats.h"
#include "server_crypto_kx.h"
#include "server_context.h"

#include <RCNET/RCNET.h>

void rcnet_load(void)
{
    NetworkState& networkState = GetNetworkState();
    if (!ServerCryptoKx_Initialize(networkState.cryptoKxState))
    {
        // fatal error
        RCNET_log(RCNET_LOG_ERROR, "Failed to initialize server crypto KX state\n");
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

void rcnet_http_update(void)
{

}