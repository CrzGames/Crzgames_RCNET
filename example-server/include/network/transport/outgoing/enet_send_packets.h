#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.
#include <vector>  // std::vector

#include <rcenet/RCENET_enet.h> // ENetPeer

bool ServerNetworkOutgoing_SendMatchInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck = false);
bool ServerNetworkOutgoing_SendWorldStaticStateInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck = false);
bool ServerNetworkOutgoing_SendMatchStartPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck = false);
bool ServerNetworkOutgoing_SendSnapshotFullPacketUnreliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool ServerNetworkOutgoing_SendSecureSessionHelloResponsePacketReliable(
    ENetPeer* peer,
    const std::vector<uint8_t>& bytes,
    bool disconnectAfterAck = false,
    bool enableEncryptionAfterAck = false);
bool ServerNetworkOutgoing_SendAuthResponsePacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck = false);
