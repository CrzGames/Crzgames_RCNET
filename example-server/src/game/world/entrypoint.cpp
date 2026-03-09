#include "game/world/entrypoint.h"

#include "game/world/input_intents.h"
#include "game/world/movement.h"
#include "game/world/collision_resolution.h"
#include "game/world/entity_lifecycle.h"
#include "game/world/rules.h"

void ServerWorld_Simulate(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt)
{
    // Si le match n'a pas encore commencé,
    // le monde ne doit pas être simulé.
    if (!gameState.matchStarted)
    {
        return;
    }

    // Construire les intentions de jeu à partir des inputs du tick.
    ServerWorld_BuildInputIntents(
        gameState,
        currentTick,
        serverTimeNs,
        dtNs,
        dt);

    // Appliquer le mouvement des entités.
    ServerWorld_RunMovement(
        gameState,
        currentTick,
        serverTimeNs,
        dtNs,
        dt);

    // Résoudre les collisions et corriger l'état spatial.
    ServerWorld_RunCollisionResolution(
        gameState,
        currentTick,
        serverTimeNs,
        dtNs,
        dt);

    // Mettre à jour le cycle de vie runtime des entités.
    ServerWorld_UpdateEntityLifecycle(
        gameState,
        currentTick,
        serverTimeNs,
        dtNs,
        dt);

    // Appliquer les règles métier du jeu.
    ServerWorld_RunRules(
        gameState,
        currentTick,
        serverTimeNs,
        dtNs,
        dt);
}