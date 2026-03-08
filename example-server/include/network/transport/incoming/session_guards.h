#pragma once

#include <cstdint> // uint32_t

bool IsConnectionAllowedForAuthChannel(uint32_t connectionId);
bool IsConnectionAllowedForGameplayChannels(uint32_t connectionId);