#pragma once

#include <cstdint> // uint32_t

#include <rcenet/RCENET_enet.h> // ENetEvent

#include "core/threading/queues/network_incoming_to_simulation.h"

void ServerNetworkIncomingUpdate_HandleReceiveEvent_Channel1AuthReliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue);