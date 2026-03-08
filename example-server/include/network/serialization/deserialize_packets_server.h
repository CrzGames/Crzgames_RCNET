#pragma once

#include <cstddef> // size_t

#include "network/packets/server/unreliable.h"
#include "network/packets/server/reliable.h"

bool deserializeServerSnapshotFullPacketUnreliable(const void* data, size_t size, ServerSnapshotFullPacketUnreliable& outPacket);