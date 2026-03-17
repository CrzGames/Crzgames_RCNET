#pragma once

#include "core/threading/queues/simulation_to_http.h"

void ServerHttp_ProcessSimulationDispatcher(const SimulationToHttpMessage& message);