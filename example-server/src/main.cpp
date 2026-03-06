#include "server_callbacks.h"
#include "server_context.h"

#include <cstring> // memset

#include <RCNET/RCNET.h>

int main(int argc, char* argv[])
{
    RCNET_log(RCNET_LOG_INFO, "Server Started\n");

#ifdef NDEBUG // Si on est en Release mode
    rcnet_logger_set_priority(RCNET_LOG_ERROR);
#else // Sinon, en Debug mode, on veut tout logger
    rcnet_logger_set_priority(RCNET_LOG_DEBUG);
#endif

    // Init à 0 pour éviter des pointeurs non initialisés
    RCNET_Callbacks myServerCallbacks;
    std::memset(&myServerCallbacks, 0, sizeof(myServerCallbacks));

    // Appliquer nos callbacks
    myServerCallbacks.rcnet_unload = rcnet_unload;
    myServerCallbacks.rcnet_load = rcnet_load;
    myServerCallbacks.rcnet_network_incoming_update = rcnet_network_incoming_update;
    myServerCallbacks.rcnet_network_outgoing_update = rcnet_network_outgoing_update;
    myServerCallbacks.rcnet_simulation_update = rcnet_simulation_update;

    // Récupérer une référence à notre état réseau global pour configurer le moteur
    NetworkState& networkState = GetNetworkState();

    // Configuration serveur (port, tick rates, etc.)
    RCNET_ServerConfig config{};
    config.port = networkState.serverPort;
    config.maxClients = networkState.maxClientsConnected;
    config.channelCount = networkState.channelCount;
    config.networkIncomingSleepMs = networkState.networkIncomingSleepMs;
    config.networkOutgoingTickHz = networkState.networkOutgoingTickRateHz;
    config.simulationTickHz = networkState.simulationTickRateHz;

    // Lancer le moteur avec nos callbacks et les tick rates désirés
    if(!rcnet_engine_run(&myServerCallbacks, &config))
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to start the engine\n");
        return 1;
    }

    return 0;
}