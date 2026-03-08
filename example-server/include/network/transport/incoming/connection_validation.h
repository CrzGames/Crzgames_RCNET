#pragma once

#include <rcenet/RCENET_enet.h> // ENetEvent

#include "network/state.h"

uint32_t ServerNetworkIncomingUpdate_GetValidatedConnectionIdOrZero(
    const ENetEvent* event,
    const NetworkState& networkState);