#pragma once

#include <cstddef> // size_t

#include "server_network_packets_server_reliable.h"
#include "server_network_packets_server_unreliable.h"

bool deserializeServerSnapshotFullPacketUnreliable(const void* data, size_t size, ServerSnapshotFullPacketUnreliable& outPacket);