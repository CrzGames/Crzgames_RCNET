#include "game/world/collision_resolution.h"

void ServerWorld_RunCollisionResolution(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt)
{
    // Cette étape résout les collisions après le mouvement.

    // Exemples de responsabilités :
    // - empêcher les entités de traverser le décor
    // - corriger les pénétrations avec le sol ou les murs
    // - appliquer la gravité si nécessaire
    // - gérer l'état grounded / airborne
    // - résoudre les collisions entre entités si le jeu le demande

    // Cette phase doit laisser le monde dans un état spatial valide
    // avant la mise à jour lifecycle et les règles gameplay.
}