#pragma once

#include <cstdint> // uint64_t

#include "game/state.h"

/**
 * @brief Construit les intentions de jeu des joueurs à partir des inputs reçus.
 *
 * Cette étape lit les inputs disponibles pour les joueurs actifs, interprète
 * leurs intentions pour le tick courant (déplacement, saut, tir, etc.) puis
 * met à jour l'état runtime nécessaire aux systèmes suivants du monde.
 *
 * @param gameState État global du jeu.
 * @param currentTick Tick courant de simulation.
 * @param serverTimeNs Temps serveur monotonic courant en nanosecondes.
 * @param dtNs Delta time courant en nanosecondes.
 * @param dt Delta time courant en secondes.
 */
void ServerWorld_BuildInputIntents(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt);