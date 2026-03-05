#pragma once

#include <stdint.h> // uint64_t

#include <rcenet/RCENET_enet.h>

void rcnet_unload(void);
void rcnet_load(void);
void rcnet_network_incoming_update(ENetHost* host, const ENetEvent* event);
void rcnet_network_outgoing_update(ENetHost* host);
void rcnet_simulation_update(uint64_t currentTick);