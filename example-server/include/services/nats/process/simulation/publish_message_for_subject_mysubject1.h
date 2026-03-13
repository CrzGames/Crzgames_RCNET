#pragma once

#include "core/threading/queues/simulation_to_nats.h" // SimulationToNatsMessage

#include <RCNET/RCNET.h> // RCNET_NATSContext

void ServerNats_ProcessSimulationDispatcher_HandlePublishMessageForSubjectMySubject1(
    RCNET_NATSContext* natsContext,
    const SimulationToNatsMessage& message);