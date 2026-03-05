#include "server_world.h"

uint32_t SpawnPlayer(GameState& gameState, PlayerType type)
{
    // Incrémenter le compteur d'entités pour obtenir un nouvel ID unique
    uint32_t entityId = gameState.nextEntityId++;

    // Créer une nouvelle entité pour le joueur
    EntityState entity{};
    entity.entityId = entityId;
    entity.type = EntityType::Player;
    entity.subtypeId = static_cast<uint8_t>(type);

    // Ajouter l'entité au jeu (donc au game state)
    gameState.entities[entityId] = entity;

    // Récupérer la configuration du type de joueur
    const PlayerConfig& cfg = GetPlayerConfigFromId(entity.subtypeId);

    // Initialiser le runtime du joueur
    PlayerRuntime runtime{};
    runtime.health = cfg.maxHealth;

    // Ajouter le runtime du joueur au game state
    gameState.players[entityId] = runtime;

    return entityId;
}

void ServerWorld_Simulate(GameState& gameState, uint64_t currentTick)
{
    // Appliquer les inputs des joueurs au monde
    ServerWorld_ApplyPlayerInputs(gameState, currentTick);

    // Simuler la physique globale du monde (mouvements, collisions, etc.)
    ServerWorld_RunPhysics(gameState, currentTick);

    // Mettre à jour l'état des entités (positions finales, états internes, timers...)
    ServerWorld_UpdateEntities(gameState, currentTick);

    // Exécuter la logique gameplay (dégâts, règles de jeu, score, etc.)
    ServerWorld_RunGameplay(gameState, currentTick);
}

void ServerWorld_ApplyPlayerInputs(GameState& gameState, uint64_t currentTick)
{
    // Parcourir les joueurs actifs dans le gameState
    // Récupérer leurs inputs provenant du système réseau
    // Appliquer les intentions de mouvement (forward, strafe, jump, etc.)
    // Mettre à jour les vitesses ou intentions de déplacement dans leurs entités
}

void ServerWorld_RunPhysics(GameState& gameState, uint64_t currentTick)
{
    // Simuler la physique du monde pour ce tick
    // Appliquer les vitesses aux positions
    // Appliquer la gravité si nécessaire
    // Résoudre les collisions entre entités ou avec le monde
}

void ServerWorld_UpdateEntities(GameState& gameState, uint64_t currentTick)
{
    // Mettre à jour les états internes des entités
    // Gérer les timers, cooldowns, animations serveur
    // Nettoyer les entités détruites ou marquées pour suppression
}

void ServerWorld_RunGameplay(GameState& gameState, uint64_t currentTick)
{
    // Appliquer les règles gameplay
    // Vérifier les dégâts, morts, respawns
    // Mettre à jour scores, objectifs, etc.
}