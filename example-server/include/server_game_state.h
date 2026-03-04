#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <unordered_map> // std::unordered_map pour stocker les entités, sessions, etc.

#include "server_game_entities.h"
#include "server_player_runtime.h"
#include "server_sessions.h"

struct GameState
{
    // --- entities (runtime minimal physique + type) ---
    std::unordered_map<uint32_t, EntityState> entities; // key = entityId

    // --- runtime spécifique par type ---
    std::unordered_map<uint32_t, PlayerRuntime> players; // key = entityId

    // --- sessions (par compte) ---
    std::unordered_map<uint64_t, ClientSession> sessions; // key = accountIdDatabase
};