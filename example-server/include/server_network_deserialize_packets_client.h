#pragma once

#include <cstddef> // size_t

#include "server_network_packets_client_reliable.h"
#include "server_network_packets_client_unreliable.h"

bool deserializeClientHandshakePacketReliable(const void* data, size_t size, ClientHandshakePacketReliable& outPacket);
bool deserializeClientReadyForMatchPacketReliable(const void* data, size_t size, ClientReadyForMatchPacketReliable& outPacket);
bool deserializeClientInputPacketUnreliable(const void* data, size_t size, ClientInputPacketUnreliable& outPacket);