#pragma once

#include <RCNET/RCNET.h>

void ServerNats_OnMessageReceivedForSubjectMySubject1(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure);