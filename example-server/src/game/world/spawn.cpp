#include "game/world/spawn.h"

uint32_t ServerWorld_SpawnPlayer(GameState& gameState, PlayerType type)
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