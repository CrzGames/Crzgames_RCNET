#pragma once

#include <rcenet/RCENET_enet.h>

#include "server_network_packets_server_reliable.h"
#include "server_network_packets_server_unreliable.h"

bool sendServerMatchInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool sendServerWorldStaticStateInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool sendServerMatchStartPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool sendServerSnapshotFullPacketUnreliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool sendServerSecureSessionHelloResponsePacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool sendServerAuthResponsePacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);