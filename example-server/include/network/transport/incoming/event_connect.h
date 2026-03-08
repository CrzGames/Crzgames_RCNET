#pragma once

#include <rcenet/RCENET_enet.h> // For ENetEvent

#include "network/state.h"
#include "core/threading/queues/network_incoming_to_simulation.h"

void ServerNetworkIncomingUpdate_HandleConnectEvent(
    const ENetEvent* event,
    NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue);