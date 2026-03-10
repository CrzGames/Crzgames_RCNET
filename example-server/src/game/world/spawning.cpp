#include "game/world/spawning.h"

uint32_t ServerWorld_SpawnPlayer(
    GameState& gameState,
    PlayerType type)
{
    // Allouer un nouvel identifiant d'entité unique.
    const uint32_t entityId = gameState.nextEntityId++;

    // Créer l'entité joueur.
    EntityState entity{};
    entity.entityId = entityId;
    entity.type = EntityType::Player;
    entity.subtypeId = static_cast<uint8_t>(type);

    // Enregistrer l'entité dans l'état global du jeu.
    gameState.entities[entityId] = entity;

    // Récupérer la configuration du type de joueur demandé.
    const PlayerConfig& cfg = GetPlayerConfigFromId(entity.subtypeId);

    // Initialiser le runtime associé à ce joueur.
    PlayerRuntime runtime{};
    runtime.health = cfg.maxHealth;

    // Enregistrer le runtime joueur dans l'état global.
    gameState.players[entityId] = runtime;

    // Retourner l'identifiant créé.
    return entityId;
}
