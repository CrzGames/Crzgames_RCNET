#pragma once

#include <cstdint>

// channel 0 = échange de clés, auth, handshake, contrôle fiable
// channel 1 = messages de jeu fiables (match start, events importants, etc.)
// channel 2 = messages de jeu non fiables (inputs, snapshots, sync temps, etc.)
enum class NetworkChannel : uint8_t
{
    HANDSHAKE_RELIABLE = 0,
    GAME_RELIABLE = 1,
    GAME_UNRELIABLE = 2,

    COUNT
};