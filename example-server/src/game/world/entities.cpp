#include "game/world/entities.h"

void ServerWorld_UpdateEntities(GameState& gameState, uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt)
{
    // Mettre à jour les états internes des entités
    // Gérer les timers, cooldowns, animations serveur
    // Nettoyer les entités détruites ou marquées pour suppression
}