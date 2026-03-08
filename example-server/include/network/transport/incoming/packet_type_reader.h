#pragma once

#include <rcenet/RCENET_enet.h> // ENetEvent

#include "network/packets/client/reliable.h"
#include "network/packets/client/unreliable.h"

bool ServerNetworkIncomingUpdate_ReadClientReliablePacketType(
    const ENetEvent* event,
    ClientReliablePacketType& outType);

bool ServerNetworkIncomingUpdate_ReadClientUnreliablePacketType(
    const ENetEvent* event,
    ClientUnreliablePacketType& outType);