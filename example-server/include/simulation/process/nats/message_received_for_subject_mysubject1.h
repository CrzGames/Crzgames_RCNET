#pragma once

#include "core/context.h" // NetworkState
#include "core/threading/queues/simulation_to_network_outgoing.h" // SimulationToNetworkOUTQueue
#include "core/threading/queues/nats_to_simulation.h" // NatsToSimulationMessage

void ServerSimulation_ProcessNatsDispatcher_HandleMessageReceivedForSubjectMySubject1(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const NatsToSimulationMessage& natsMessage);