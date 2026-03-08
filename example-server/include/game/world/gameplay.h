#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.

#include "game/state.h"
#include "game/config.h"

void ServerWorld_RunGameplay(GameState& gameState, uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt);