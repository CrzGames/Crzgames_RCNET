#include "server_context.h"

static GameState g_gameState;
static NetworkState g_networkState;
static NetworkINToSimulationQueue g_netToSimQueue;
static SimulationToNetworkOUTQueue g_simToNetQueue;

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