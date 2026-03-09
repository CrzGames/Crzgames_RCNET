#include "core/callbacks.h"
#include "network/server_config.h"

#include <cstring> // memset

#include <RCNET/RCNET.h>

int main(int argc, char* argv[])
{
    RCNET_log(RCNET_LOG_INFO, "Server Started");

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
    myServerCallbacks.rcnet_http_update = rcnet_http_update;
    myServerCallbacks.rcnet_nats_update = rcnet_nats_update;

    // Construire la config serveur
    RCNET_ServerConfig config;
    config.port = ServerConfig::serverPort;
    config.maxClients = ServerConfig::maxClientsConnected;
    config.channelCount = ServerConfig::channelCount;
    config.simulationTickHz = ServerConfig::simulationTickRateHz;
    config.networkOutgoingTickHz = ServerConfig::networkOutgoingTickRateHz;
    config.networkIncomingPollTimeoutMs = ServerConfig::networkIncomingPollTimeoutMs;
    config.httpThreadSleepMs = ServerConfig::httpThreadSleepMs;
    config.natsThreadSleepMs = ServerConfig::natsThreadSleepMs;

    // Lancer le moteur avec nos callbacks et les tick rates désirés
    if(!rcnet_engine_run(&myServerCallbacks, &config))
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to start the engine\n");
        return 1;
    }

    return 0;
}