#pragma once

#include <rcenet/RCENET_enet.h>

#include "server_network_packets_server_reliable.h"
#include "server_network_packets_server_unreliable.h"

bool sendServerMatchInitPacketReliable(ENetPeer* peer, const ServerMatchInitPacketReliable& packet);
bool sendServerWorldStaticStateInitPacketReliable(ENetPeer* peer, const ServerWorldStaticStateInitPacketReliable& packet);
bool sendServerMatchStartPacketReliable(ENetPeer* peer, const ServerMatchStartPacketReliable& packet);
bool sendServerSnapshotFullPacketUnreliable(ENetPeer* peer, const ServerSnapshotFullPacketUnreliable& packet);
bool sendServerSecureSessionHelloResponsePacketReliable(ENetPeer* peer, const ServerSecureSessionHelloResponsePacketReliable& packet);
bool sendServerAuthResponsePacketReliable(ENetPeer* peer, const ServerAuthResponsePacketReliable& packet);