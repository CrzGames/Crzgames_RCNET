#pragma once

#include <rcenet/RCENET_enet.h> // ENetHost, ENetEvent

void ServerNetworkIncomingUpdate_ProcessENetEvent(ENetHost* host, const ENetEvent* event);