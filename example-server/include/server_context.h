#pragma once

#include "server_game_state.h"
#include "server_network_state.h"
#include "server_queues.h"

// Accès global au game state
GameState& GetGameState();

// Accès global au network state
NetworkState& GetNetworkState();

// Accès global aux queues de communication du réseau IN vers le thread simulation
NetworkINToSimulationQueue& GetNetworkINToSimulationQueue();

// Accès global à la queue de communication de la simulation vers le thread réseau
SimulationToNetworkOUTQueue& GetSimulationToNetworkOUTQueue();

// Accès global à la queue de communication de la simulation vers le thread HTTP
SimulationToHttpQueue& GetSimulationToHttpQueue();

// Accès global à la queue de communication du thread HTTP vers le thread simulation
HttpToSimulationQueue& GetHttpToSimulationQueue();