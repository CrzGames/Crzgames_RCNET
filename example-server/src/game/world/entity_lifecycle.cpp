#include "game/world/entity_lifecycle.h"

void ServerWorld_UpdateEntityLifecycle(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt)
{
    // Cette étape gère la maintenance runtime générale des entités.

    // Exemples de responsabilités :
    // - décrémenter des timers ou cooldowns
    // - faire avancer des états temporaires
    // - traiter des transitions runtime
    // - nettoyer les entités marquées pour suppression
    // - synchroniser certaines structures runtime associées

    // L'objectif de ce module n'est pas de gérer les règles métier du jeu,
    // mais la vie technique et runtime des entités serveur.
}