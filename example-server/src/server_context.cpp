#include "server_context.h"

static GameState g_gameState;
static NetworkState g_networkState;
static NetworkToSimulationQueue g_netToSimQueue;
static SimulationToNetworkQueue g_simToNetQueue;

GameState& GetGameState()
{
    return g_gameState;
}

NetworkState& GetNetworkState()
{
    return g_networkState;
}

NetworkToSimulationQueue& GetNetToSimQueue()
{
    return g_netToSimQueue;
}

SimulationToNetworkQueue& GetSimToNetQueue()
{
    return g_simToNetQueue;
}