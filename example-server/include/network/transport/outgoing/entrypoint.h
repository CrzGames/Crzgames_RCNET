#pragma once

#include <rcenet/RCENET_enet.h> // EnetHost

void ServerNetworkOutgoing_DrainCoalesceAndSendMessages(ENetHost* host);