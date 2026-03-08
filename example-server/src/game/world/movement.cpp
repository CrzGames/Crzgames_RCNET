#include "game/world/movement.h"

void ServerWorld_RunMovement(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt)
{
    // Cette étape applique le mouvement voulu par les intentions construites
    // pendant la phase précédente.

    // Exemples de responsabilités :
    // - transformer une intention de déplacement en vitesse cible
    // - appliquer accélération et décélération
    // - appliquer friction
    // - mettre à jour la position provisoire des entités
    // - calculer le déplacement brut avant collisions

    // Cette phase prépare l'état spatial du tick,
    // avant la phase de résolution des collisions.
}