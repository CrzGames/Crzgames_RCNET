#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <unordered_map> // std::unordered_map pour stocker les entités, sessions, etc.

#include "server_game_entities.h"
#include "server_player_runtime.h"
#include "server_sessions.h"

struct GameState
{
    // ------------------------------------------------------------------------
    // World state
    // ------------------------------------------------------------------------

    // Incrémenté à chaque nouvelle entité pour lui donner un ID unique
    uint32_t nextEntityId = 1;

    // --- entities (runtime minimal physique + type) ---
    std::unordered_map<uint32_t, EntityState> entities; // key = entityId

    // --- runtime spécifique par type ---
    std::unordered_map<uint32_t, PlayerRuntime> players; // key = entityId
};