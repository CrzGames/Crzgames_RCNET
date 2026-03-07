#pragma once

#include <vector>  // std::vector pour stocker les données sérialisées
#include <cstdint> // uint8_t, uint32_t, etc.

#include "server_network_packets_server_reliable.h"
#include "server_network_packets_server_unreliable.h"

std::vector<uint8_t> serializeServerMatchInitPacketReliable(const ServerMatchInitPacketReliable& packet);
std::vector<uint8_t> serializeServerWorldStaticStateInitPacketReliable(const ServerWorldStaticStateInitPacketReliable& packet);
std::vector<uint8_t> serializeServerMatchStartPacketReliable(const ServerMatchStartPacketReliable& packet);
std::vector<uint8_t> serializeServerSnapshotFullPacketUnreliable(const ServerSnapshotFullPacketUnreliable& packet);
std::vector<uint8_t> serializeServerSecureSessionHelloResponsePacketReliable(const ServerSecureSessionHelloResponsePacketReliable& packet);
std::vector<uint8_t> serializeServerAuthResponsePacketReliable(const ServerAuthResponsePacketReliable& packet);