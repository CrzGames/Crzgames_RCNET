#pragma once

#include "game/state.h"
#include "network/state.h"
#include "core/threading/queues/http_to_simulation.h"
#include "core/threading/queues/nats_to_simulation.h"
#include "core/threading/queues/network_incoming_to_simulation.h"
#include "core/threading/queues/simulation_to_http.h"
#include "core/threading/queues/simulation_to_nats.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"
#include "services/http/httplib_wrapper.h"

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

// Accès global à la queue de communication du thread NATS vers le thread simulation
NatsToSimulationQueue& GetNatsToSimulationQueue();

// Accès global à la queue de communication de la simulation vers le thread NATS
SimulationToNatsQueue& GetSimulationToNatsQueue();

// Accès global au client HTTP utilisé par le thread HTTP
httplib::Client& GetHttpClient();
