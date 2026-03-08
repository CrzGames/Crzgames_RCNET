#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.

#include "game/state.h"

// Simulation principale du monde appelée chaque tick serveur
void ServerWorld_Simulate(GameState& gameState, uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt);