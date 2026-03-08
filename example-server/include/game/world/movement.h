#pragma once

#include <cstdint> // uint64_t

#include "game/state.h"

/**
 * @brief Applique le mouvement des entités pour le tick courant.
 *
 * Cette étape transforme les intentions de déplacement et les vitesses en
 * mouvement effectif dans le monde. Elle prépare l'état spatial avant la phase
 * de résolution des collisions.
 *
 * @param gameState État global du jeu.
 * @param currentTick Tick courant de simulation.
 * @param serverTimeNs Temps serveur monotonic courant en nanosecondes.
 * @param dtNs Delta time courant en nanosecondes.
 * @param dt Delta time courant en secondes.
 */
void ServerWorld_RunMovement(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt);