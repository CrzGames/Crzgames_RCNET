#pragma once

#include <cstddef> // size_t

#include "server_network_packets_client_reliable.h"
#include "server_network_packets_client_unreliable.h"

bool deserializeClientSecureSessionHelloPacketReliable(const void* data, size_t size, ClientSecureSessionHelloPacketReliable& outPacket);
bool deserializeClientAuthPacketReliable(const void* data, size_t size, ClientAuthPacketReliable& outPacket);
bool deserializeClientReadyForMatchPacketReliable(const void* data, size_t size, ClientReadyForMatchPacketReliable& outPacket);
bool deserializeClientInputPacketUnreliable(const void* data, size_t size, ClientInputPacketUnreliable& outPacket);