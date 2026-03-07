#pragma once

#include <cstdint>

// channel 0 = échange de clés (packet non chiffré)
// channel 1 = messages de jeu fiables (packet cryptés)
// channel 2 = messages de jeu non fiables (packet cryptés)
enum class NetworkChannel : uint8_t
{
    SECURE_SESSION_RELIABLE = 0,
    AUTH_RELIABLE = 1,
    GAME_RELIABLE = 2,
    GAME_UNRELIABLE = 3,

    COUNT
};