#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.
#include <vector>  // std::vector

#include <rcenet/RCENET_enet.h> // ENetPeer

bool sendServerMatchInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool sendServerWorldStaticStateInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool sendServerMatchStartPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool sendServerSnapshotFullPacketUnreliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool sendServerSecureSessionHelloResponsePacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool sendServerAuthResponsePacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);