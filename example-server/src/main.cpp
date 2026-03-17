#include "core/callbacks.h"
#include "core/config/server.h"

#include <cstdlib> // std::getenv

#include <RCNET/RCNET.h>

int main(int argc, char* argv[])
{
#ifdef NDEBUG // Si on est en Release mode
    rcnet_logger_set_priority(RCNET_LOG_ERROR);
#else // Sinon, en Debug mode, on veut tout logger
    rcnet_logger_set_priority(RCNET_LOG_DEBUG);
#endif

    // Préparer les callbacks
    RCNET_Callbacks myServerCallbacks{};
    myServerCallbacks.rcnet_unload = rcnet_unload;
    myServerCallbacks.rcnet_load = rcnet_load;
    myServerCallbacks.rcnet_network_incoming_update = rcnet_network_incoming_update;
    myServerCallbacks.rcnet_network_host_setup = rcnet_network_host_setup;
    myServerCallbacks.rcnet_network_outgoing_update = rcnet_network_outgoing_update;
    myServerCallbacks.rcnet_simulation_update = rcnet_simulation_update;
    myServerCallbacks.rcnet_http_update = rcnet_http_update;
    myServerCallbacks.rcnet_nats_update = rcnet_nats_update;
    myServerCallbacks.rcnet_wake_blocking_threads = rcnet_wake_blocking_threads;

    // Construire la config serveur
    RCNET_ServerConfig myServerConfig{};
    myServerConfig.port = ServerConfig::serverPort;
    myServerConfig.maxClients = ServerConfig::maxClientsConnected;
    myServerConfig.channelCount = ServerConfig::channelCount;
    myServerConfig.simulationTickHz = ServerConfig::simulationTickRateHz;
    myServerConfig.networkOutgoingTickHz = ServerConfig::networkOutgoingTickRateHz;
    myServerConfig.networkIncomingPollTimeoutMs = ServerConfig::networkIncomingPollTimeoutMs;
    // Configuration NATS
    myServerConfig.natsConfig.natsServerURL = ServerConfig::natsServerURL;
    myServerConfig.natsConfig.useTLS = ServerConfig::natsUseTLS;
    myServerConfig.natsConfig.skipVerifyCertsServer = ServerConfig::natsSkipVerifyCertsServer;
#if SERVER_ENV_DEV
    myServerConfig.natsConfig.publicKeyNKey = ServerConfig::natsPublicKeyNKey;
    myServerConfig.natsConfig.privateKeySeedNKey = ServerConfig::natsPrivateKeySeedNKey;
#else
    myServerConfig.natsConfig.publicKeyNKey = std::getenv("NATS_NKEY_PUBLIC_KEY");
    myServerConfig.natsConfig.privateKeySeedNKey = std::getenv("NATS_NKEY_PRIVATE_KEY");
#endif

    // Lancer le moteur avec nos callbacks et les tick rates désirés
    if(!rcnet_engine_run(&myServerCallbacks, &myServerConfig))
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to start the engine\n");
        return 1;
    }

    return 0;
}
