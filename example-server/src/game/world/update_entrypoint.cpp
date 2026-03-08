#include "game/world/update_entrypoint.h"

#include "game/world/inputs.h"
#include "game/world/physics.h"
#include "game/world/entities.h"
#include "game/world/gameplay.h"

void ServerWorld_Simulate(GameState& gameState, uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt)
{
    // Si la partie n'a pas encore commencé, on ne simule rien
    if (!gameState.matchStarted)
        return;

    // Appliquer les inputs des joueurs au monde
    ServerWorld_ApplyPlayerInputs(gameState, currentTick, serverTimeNs, dtNs, dt);

    // Simuler la physique globale du monde (mouvements, collisions, etc.)
    ServerWorld_RunPhysics(gameState, currentTick, serverTimeNs, dtNs, dt);

    // Mettre à jour l'état des entités (positions finales, états internes, timers...)
    ServerWorld_UpdateEntities(gameState, currentTick, serverTimeNs, dtNs, dt);

    // Exécuter la logique gameplay (dégâts, règles de jeu, score, etc.)
    ServerWorld_RunGameplay(gameState, currentTick, serverTimeNs, dtNs, dt);
}