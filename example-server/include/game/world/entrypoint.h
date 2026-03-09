#pragma once

#include <cstdint> // uint64_t

#include "game/state.h"

/**
 * @brief Simule le monde de jeu pour un tick serveur complet.
 *
 * Cette fonction orchestre les différentes phases de simulation du monde :
 * - construction des intentions depuis les inputs
 * - mouvement
 * - résolution des collisions
 * - maintenance du cycle de vie des entités
 * - application des règles du jeu
 *
 * @param gameState État global du jeu.
 * @param currentTick Tick courant de simulation.
 * @param serverTimeNs Temps serveur monotonic courant en nanosecondes.
 * @param dtNs Delta time courant en nanosecondes.
 * @param dt Delta time courant en secondes.
 */
void ServerWorld_Simulate(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt);