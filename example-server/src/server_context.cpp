#include "server_context.h"

static GameState g_gameState;
static NetworkToSimulationQueue g_netToSimQueue;

GameState& GetGameState()
{
    return g_gameState;
}

NetworkToSimulationQueue& GetNetToSimQueue()
{
    return g_netToSimQueue;
}