#pragma once

#include "game/data/entity_types.h"
#include "game/data/player_config.h"
#include "game/data/projectile_config.h"
#include "game/data/item_config.h"

// ============================================================================
// Résultat générique de config pour une entité
// (contient des pointeurs, un seul sera non-null selon le type)
// ============================================================================
struct EntityConfigView
{
    const PlayerConfig*     player     = nullptr;
    const ProjectileConfig* projectile = nullptr;
    const ItemConfig*       item       = nullptr;
};

inline EntityConfigView GetConfigFromEntity(const EntityState& entity)
{
    EntityConfigView view{};

    switch (entity.type)
    {
        case EntityType::Player:
        {
            view.player = &GetPlayerConfigFromId(entity.subtypeId);
            break;
        }

        case EntityType::Projectile:
        {
            view.projectile = &GetProjectileConfigFromId(entity.subtypeId);
            break;
        }

        case EntityType::Item:
        {
            view.item = &GetItemConfigFromId(entity.subtypeId);
            break;
        }

        default:
            break;
    }

    return view;
}