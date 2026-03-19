#pragma once

#include <stdint.h> // uint64_t

#include <rcenet/RCENET_enet.h> // ENetHost, ENetEvent

#include <RCNET/RCNET.h>

void rcnet_unload(void);
void rcnet_load(void);
void rcnet_network_incoming_update(ENetHost* host, const ENetEvent* event);
void rcnet_network_host_setup(ENetHost* host);
void rcnet_network_outgoing_update(ENetHost* host);
void rcnet_simulation_update(uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt);
void rcnet_http_update(void);
void rcnet_nats_update(RCNET_NATSContext* natsContext);
void rcnet_wake_blocking_threads(void);