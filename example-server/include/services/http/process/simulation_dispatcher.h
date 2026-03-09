#pragma once

#include <deque> // std::deque

#include "network/state.h"
#include "core/threading/queues/simulation_to_http.h"

void ServerHttpUpdate_ProcessHttpDispatcher(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    std::deque<HttpToSimulationMessage>& messages);