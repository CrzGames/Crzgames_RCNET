#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <unordered_map> // std::unordered_map pour stocker les entités, sessions, etc.

#include "server_sessions.h"

struct NetworkState
{
    // ------------------------------------------------------------------------
    // Connections et sessions
    // ------------------------------------------------------------------------
    
    // Incrémenté à chaque nouvelle connexion pour lui donner un ID unique (différent de l'accountIdDatabase)
    uint32_t nextConnectionId = 1;

    // --- sessions (par compte) ---
    std::unordered_map<uint32_t, ClientSession> sessions; // key = connectionId
};