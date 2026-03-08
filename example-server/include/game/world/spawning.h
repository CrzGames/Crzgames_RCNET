#pragma once

#include <cstdint> // uint32_t

#include "game/state.h"
#include "game/data/player_config.h"

/**
 * @brief Crée une entité joueur et initialise son état runtime associé.
 *
 * Cette fonction alloue un nouvel identifiant d'entité, crée l'entité joueur,
 * initialise son runtime à partir de la configuration du type demandé puis
 * enregistre l'ensemble dans le `GameState`.
 *
 * @param gameState État global du jeu.
 * @param type Type de joueur à créer.
 *
 * @return L'identifiant unique de l'entité joueur créée.
 */
uint32_t ServerWorld_SpawnPlayer(
    GameState& gameState,
    PlayerType type);