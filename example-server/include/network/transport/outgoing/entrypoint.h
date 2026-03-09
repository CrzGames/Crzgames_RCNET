#pragma once

#include <rcenet/RCENET_enet.h> // EnetHost

void ServerNetworkOutgoingUpdate_DrainCoalesceAndSendMessages(ENetHost* host);