#pragma once

#include "core/threading/queues/simulation_to_nats.h"

#include <RCNET/RCNET.h> // RCNET_NATSContext

void ServerNats_ProcessSimulationDispatcher(
    RCNET_NATSContext* natsContext,
    const SimulationToNatsMessage& message);
