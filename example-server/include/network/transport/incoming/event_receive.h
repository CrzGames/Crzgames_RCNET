#pragma once

#include <rcenet/RCENET_enet.h> // ENetEvent

#include "network/state.h"
#include "core/threading/queues/network_incoming_to_simulation.h"

void ServerNetworkIncomingUpdate_HandleReceiveEvent(
    const ENetEvent* event,
    const NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue);