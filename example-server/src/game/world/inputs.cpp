#include "game/world/inputs.h"

void ServerWorld_ApplyPlayerInputs(GameState& gameState, uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt)
{
    // Parcourir les joueurs actifs dans le gameState
    // Récupérer leurs inputs provenant du système réseau
    // Appliquer les intentions de mouvement (forward, strafe, jump, etc.)
    // Mettre à jour les vitesses ou intentions de déplacement dans leurs entités
}