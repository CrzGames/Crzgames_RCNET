#include "core/callbacks.h"

#include "core/context.h"
#include "network/transport/incoming/entrypoint.h"
#include "network/transport/outgoing/entrypoint.h"
#include "simulation/entrypoint.h"
#include "crypto/kx.h"

#include <RCNET/RCNET.h>

void rcnet_load(void)
{
    // Initialisation des clés de cryptographie KX du serveur
    NetworkState& networkState = GetNetworkState();
    if (!ServerCryptoKx_Initialize(networkState.cryptoKxState))
    {
        rcnet_engine_eventQuit(); // Arrêt du moteur en cas d'échec de l'initialisation
        RCNET_log(RCNET_LOG_ERROR, "Failed to initialize server crypto KX state");
    }

    // Dire que le serveur est prêt dès que la fin de cette fonction est atteinte.
    // En effet, le moteur ne commencera à traiter les connexions entrantes et les ticks de simulation qu'après l'appel de cette fonction.
    RCNET_log(RCNET_LOG_INFO, "Server is ready");
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
    ServerNetworkOutgoing_DrainSimulationMessages_And_SendPackets(host);
}

void rcnet_simulation_update(uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt)
{
    ServerSimulation_DrainNetworkIncomingAndHttpAndNatsMessages_And_RunSimulationForCurrentTick(currentTick, serverTimeNs, dtNs, dt);
}

void rcnet_http_update(void)
{

}

void rcnet_nats_update(RCNET_NATSContext* natsContext)
{

}