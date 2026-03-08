#pragma once

#include "game/state.h"
#include "game/config.h"

// Spawn d'un joueur
uint32_t ServerWorld_SpawnPlayer(GameState& gameState, PlayerType type);