#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <unordered_map> // std::unordered_map

#include "game/data/entity_types.h" // EntityType (types d'entités principaux du jeu : Player, Projectile, Item, etc.)
#include "game/runtime/player_runtime.h" // PlayerRuntime (runtime spécifique pour les entités de type joueur, ex: points de vie, mana, etc.)

struct GameState
{
    // ------------------------------------------------------------------------
    // Configuration du jeu
    // ------------------------------------------------------------------------

    // Booléen pour indiquer si les packets "SERVER_MATCH_INIT_RELIABLE" ont été envoyés aux clients.
    bool matchInitSent = false;

    // Booléen pour indiquer si les packets "SERVER_WORLD_STATIC_STATE_INIT_RELIABLE" ont été envoyés aux clients.
    bool worldStaticStateInitSent = false;

    // Booléen pour indiquer si les packets "SERVER_MATCH_START_RELIABLE" ont été envoyés aux clients.
    bool matchStartSent = false;

    // Booléen pour indiquer si le match a officiellement commencé.
    bool matchStarted = false;

    // Tick de simulation auquel le match commence officiellement
    uint64_t matchStartTick = 0;


    // ------------------------------------------------------------------------
    // World state
    // ------------------------------------------------------------------------

    // Incrémenté à chaque nouvelle entité pour lui donner un ID unique
    uint32_t nextEntityId = 1;

    // --- entities (runtime minimal physique + type) ---
    std::unordered_map<uint32_t, EntityState> entities; // key = entityId

    // --- runtime spécifique par type ---
    std::unordered_map<uint32_t, PlayerRuntime> players; // key = entityId
};