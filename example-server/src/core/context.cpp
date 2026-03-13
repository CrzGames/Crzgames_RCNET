#include "core/context.h"

#include "services/http/client.h"

#include <cstdlib> // std::abort

#include <RCNET/RCNET.h>

static GameState g_gameState;
static NetworkState g_networkState;
static NetworkINToSimulationQueue g_netToSimQueue;
static SimulationToNetworkOUTQueue g_simToNetQueue;
static SimulationToHttpQueue g_simToHttpQueue;
static HttpToSimulationQueue g_httpToSimQueue;
static SimulationToNatsQueue g_simToNatsQueue;
static NatsToSimulationQueue g_natsToSimQueue;

GameState& GetGameState()
{
    return g_gameState;
}

NetworkState& GetNetworkState()
{
    return g_networkState;
}

NetworkINToSimulationQueue& GetNetworkINToSimulationQueue()
{
    return g_netToSimQueue;
}

SimulationToNetworkOUTQueue& GetSimulationToNetworkOUTQueue()
{
    return g_simToNetQueue;
}

SimulationToHttpQueue& GetSimulationToHttpQueue()
{
    return g_simToHttpQueue;
}

HttpToSimulationQueue& GetHttpToSimulationQueue()
{
    return g_httpToSimQueue;
}

NatsToSimulationQueue& GetNatsToSimulationQueue()
{
    return g_natsToSimQueue;
}

SimulationToNatsQueue& GetSimulationToNatsQueue()
{
    return g_simToNatsQueue;
}

httplib::Client& GetHttpClient()
{
    if (!g_httpClient)
    {
        RCNET_log(RCNET_LOG_CRITICAL, "HTTP client is not initialized.");
        std::abort();
    }

    return *g_httpClient;
}
