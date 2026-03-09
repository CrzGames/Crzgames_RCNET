#include "core/callbacks.h"

#include "core/context.h"
#include "network/transport/incoming/entrypoint.h"
#include "network/transport/outgoing/entrypoint.h"
#include "simulation/entrypoint.h"
#include "crypto/kx.h"

#include <RCNET/RCNET.h>

void rcnet_load(void)
{
    // Log de la configuration effective du moteur
    RCNET_log(RCNET_LOG_INFO, "Simulation tick rate: %u Hz", rcnet_engine_getSimulationTickRateHz());
    RCNET_log(RCNET_LOG_INFO, "Network outgoing tick rate: %u Hz", rcnet_engine_getNetworkOutgoingTickRateHz());
    RCNET_log(RCNET_LOG_INFO, "Network incoming poll timeout: %u ms", rcnet_engine_getNetworkIncomingPollTimeoutMs());
    RCNET_log(RCNET_LOG_INFO, "HTTP thread sleep duration: %u ms", rcnet_engine_getHttpThreadSleepMs());

    // Initialisation des clés de cryptographie KX du serveur
    NetworkState& networkState = GetNetworkState();
    if (!ServerCryptoKx_Initialize(networkState.cryptoKxState))
    {
        rcnet_engine_eventQuit(); // Arrêt du moteur en cas d'échec de l'initialisation
        RCNET_log(RCNET_LOG_ERROR, "Failed to initialize server crypto KX state\n");
    }
}

void rcnet_unload(void)
{

}

void rcnet_network_incoming_update(ENetHost* host, const ENetEvent* event)
{
    ServerNetworkIncoming_ProcessENetEvent(host, event);
}

void rcnet_network_outgoing_update(ENetHost* host)
{
    ServerNetworkOutgoing_DrainSimulationMessagesAndSendPackets(host);
}

void rcnet_simulation_update(uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt)
{
    ServerSimulation_RunCurrentTick(currentTick, serverTimeNs, dtNs, dt);
}

void rcnet_http_update(void)
{

}

void rcnet_nats_update(RCNET_NATSClient* natsClient)
{

}