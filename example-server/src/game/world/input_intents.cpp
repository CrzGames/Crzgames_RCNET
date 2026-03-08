#include "game/world/input_intents.h"

void ServerWorld_BuildInputIntents(
    GameState& gameState,
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt)
{
    // Cette étape doit lire les inputs gameplay disponibles
    // pour les joueurs actifs pendant le tick courant.

    // L'objectif n'est pas encore de déplacer les entités directement,
    // mais de transformer les inputs reçus en intentions exploitables
    // par le système de mouvement.

    // Exemples d'intentions à construire :
    // - déplacement avant / arrière
    // - déplacement latéral
    // - saut
    // - tir
    // - visée
    // - sprint
    // - crouch

    // Cette logique pourra plus tard :
    // - parcourir les joueurs actifs
    // - consommer leurs inputs en attente
    // - écrire le résultat dans le runtime ou l'état des entités
    // - préparer le mouvement du tick courant
}