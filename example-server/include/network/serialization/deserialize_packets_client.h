#pragma once

#include <cstddef> // size_t

#include "network/packets/client/reliable.h"
#include "network/packets/client/unreliable.h"

bool deserializeClientSecureSessionHelloPacketReliable(const void* data, size_t size, ClientSecureSessionHelloPacketReliable& outPacket);
bool deserializeClientAuthPacketReliable(const void* data, size_t size, ClientAuthPacketReliable& outPacket);
bool deserializeClientReadyForMatchPacketReliable(const void* data, size_t size, ClientReadyForMatchPacketReliable& outPacket);
bool deserializeClientInputPacketUnreliable(const void* data, size_t size, ClientInputPacketUnreliable& outPacket);