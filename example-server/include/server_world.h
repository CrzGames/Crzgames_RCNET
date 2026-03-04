#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.

#include "server_game_state.h"
#include "server_game_config.h"

uint32_t SpawnPlayer(GameState& gameState, PlayerType type);