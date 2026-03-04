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

    // Créer une nouvelle configuration de runtime pour le joueur
    const PlayerConfig& cfg = GetPlayerConfigFromId(entity.subtypeId);

    PlayerRuntime runtime{};
    runtime.health = cfg.maxHealth;

    // Ajouter l'entité du joueur au game state
    gameState.players[entityId] = runtime;

    return entityId;
}