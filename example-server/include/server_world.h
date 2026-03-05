#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.

#include "server_game_state.h"
#include "server_game_config.h"

// Simulation principale du monde appelée chaque tick serveur
void ServerWorld_Simulate(GameState& gameState, uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt);

// Spawn d'un joueur
uint32_t SpawnPlayer(GameState& gameState, PlayerType type);

// Internal world simulation steps
void ServerWorld_ApplyPlayerInputs(GameState& gameState, uint64_t currentTick);
void ServerWorld_RunPhysics(GameState& gameState, uint64_t currentTick);
void ServerWorld_UpdateEntities(GameState& gameState, uint64_t currentTick);
void ServerWorld_RunGameplay(GameState& gameState, uint64_t currentTick);