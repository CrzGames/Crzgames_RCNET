#include "game/world/rules.h"

void ServerWorld_RunRules(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt)
{
    // Cette étape applique les règles du jeu après que le monde
    // a été mis à jour spatialement et runtime.

    // Exemples de responsabilités :
    // - valider les dégâts
    // - détecter les morts
    // - gérer les respawns
    // - mettre à jour les scores
    // - vérifier les objectifs
    // - traiter les conditions de victoire ou de fin de manche

    // Ce module doit contenir les conséquences gameplay métier,
    // et non la mécanique brute de déplacement ou de collision.
}