#include "server_context.h"

static GameState g_gameState;
static NetworkState g_networkState;
static NetworkToSimulationQueue g_netToSimQueue;

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