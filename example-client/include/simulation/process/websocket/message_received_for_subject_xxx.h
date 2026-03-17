#pragma once

#include "core/context.h" // NetworkState
#include "core/threading/queues/simulation_to_network_outgoing.h" // SimulationToNetworkOUTQueue
#include "core/threading/queues/websocket_to_simulation.h" // WebSocketToSimulationMessage

void ClientSimulation_ProcessWebSocketDispatcher_HandleMessageReceivedForSubjectXXX(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const WebSocketToSimulationMessage& websocketToSimMessage);