#pragma once

#include <cstdint> // uint64_t

#include "game/state.h"

/**
 * @brief Met à jour le cycle de vie runtime des entités du monde.
 *
 * Cette étape gère la maintenance générale des entités :
 * timers, cooldowns, états temporaires, transitions runtime, nettoyage
 * des entités détruites ou marquées pour suppression.
 *
 * @param gameState État global du jeu.
 * @param currentTick Tick courant de simulation.
 * @param serverTimeNs Temps serveur monotonic courant en nanosecondes.
 * @param dtNs Delta time courant en nanosecondes.
 * @param dt Delta time courant en secondes.
 */
void ServerWorld_UpdateEntityLifecycle(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt);