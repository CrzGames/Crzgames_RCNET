#pragma once

#include <cstdint> // uint64_t

#include "game/state.h"
#include "game/config.h"

/**
 * @brief Applique les inputs joueurs au monde pour le tick courant.
 *
 * Cette fonction lit les inputs en attente des joueurs, met à jour
 * leurs intentions de déplacement et prépare l'état qui sera ensuite
 * consommé par la physique et le gameplay.
 *
 * @param gameState État global du jeu.
 * @param currentTick Tick courant de simulation.
 * @param serverTimeNs Temps serveur monotonic courant en nanosecondes.
 * @param dtNs Delta time courant en nanosecondes.
 * @param dt Delta time courant en secondes.
 */
void ServerWorld_ApplyPlayerInputs(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt);