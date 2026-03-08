#pragma once

#include <cstdint> // uint64_t

#include "game/state.h"

/**
 * @brief Résout les collisions du monde pour le tick courant.
 *
 * Cette étape applique les corrections spatiales nécessaires après le mouvement :
 * collisions avec le décor, le sol, les murs ou d'autres entités selon les règles
 * de collision du serveur.
 *
 * @param gameState État global du jeu.
 * @param currentTick Tick courant de simulation.
 * @param serverTimeNs Temps serveur monotonic courant en nanosecondes.
 * @param dtNs Delta time courant en nanosecondes.
 * @param dt Delta time courant en secondes.
 */
void ServerWorld_RunCollisionResolution(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt);