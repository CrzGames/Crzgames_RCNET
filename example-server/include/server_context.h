#pragma once

#include "server_game_state.h"
#include "server_queues_network_and_simulation.h"

GameState& GetGameState();
NetworkToSimulationQueue& GetNetToSimQueue();
//SimulationToNetworkQueue& GetSimToNetQueue();