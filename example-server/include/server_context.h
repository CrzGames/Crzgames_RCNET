#pragma once

#include "server_game_state.h"
#include "server_network_state.h"
#include "server_queues_network_and_simulation.h"

// Accès global au game state
GameState& GetGameState();

// Accès global au network state
NetworkState& GetNetworkState();

// Accès global aux queues de communication entre réseau et simulation
NetworkINToSimulationQueue& GetNetworkINToSimulationQueue();

// Accès global à la queue de communication de la simulation vers le réseau
SimulationToNetworkOUTQueue& GetSimulationToNetworkOUTQueue();