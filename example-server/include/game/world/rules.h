#pragma once

#include <cstdint> // uint64_t

#include "game/state.h"

/**
 * @brief Applique les règles métier du jeu pour le tick courant.
 *
 * Cette étape gère les conséquences gameplay :
 * dégâts, morts, validation des actions, score, objectifs, victoire,
 * défaite et autres règles propres au mode de jeu.
 *
 * @param gameState État global du jeu.
 * @param currentTick Tick courant de simulation.
 * @param serverTimeNs Temps serveur monotonic courant en nanosecondes.
 * @param dtNs Delta time courant en nanosecondes.
 * @param dt Delta time courant en secondes.
 */
void ServerWorld_RunRules(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt);