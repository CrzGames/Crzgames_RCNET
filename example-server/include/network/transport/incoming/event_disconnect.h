#pragma once

#include <rcenet/RCENET_enet.h> // EnetEvent

#include "network/state.h"
#include "core/threading/queues/network_incoming_to_simulation.h"

void ServerNetworkIncomingUpdate_HandleDisconnectEvent(
    const ENetEvent* event,
    NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue);