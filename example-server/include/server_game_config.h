#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.

// ============================================================================
// Configurations "design" (data-driven)
// Ces données ne sont PAS runtime, elles décrivent le comportement
// des sous-types d'entités.
// ============================================================================

// --------------------------------------------------------------------------
// Player config
// --------------------------------------------------------------------------
enum class PlayerType : uint8_t
{
    None = 0,
    Warrior,
    Mage,
    Rogue,

    Count // Toujours garder cet élément en dernier pour compter automatiquement le nombre de types
};

struct PlayerConfig
{
    PlayerType type = PlayerType::None;
    int32_t maxHealth      = 100;       // Points de vie max
    float   moveSpeed      = 6.0f;      // Vitesse de déplacement de base (ex: 6 unités/s)
    float   jumpForce      = 10.0f;     // Force appliquée lors d’un saut (ex: 10 unités/s instantané)
    float   dashSpeed      = 12.0f;     // Vitesse de dash (ex: 12 unités/s instantané)
};

// Tableau statique indexé directement par PlayerType pour un accès rapide en simulation
inline const PlayerConfig playerConfigs[] =
{
    { PlayerType::None,    0,   0.0f,  0.0f,  0.0f },

    { PlayerType::Warrior, 150, 5.0f,  9.0f, 10.0f },

    { PlayerType::Mage,    80,  6.5f,  10.0f, 11.0f },

    { PlayerType::Rogue,   100, 8.0f,  11.0f, 14.0f },
};

// Fonction d’accès rapide à la config d’un PlayerType
inline const PlayerConfig& GetPlayerConfigFromId(uint8_t id)
{
    if (id >= static_cast<uint8_t>(PlayerType::Count))
        id = static_cast<uint8_t>(PlayerType::None);

    return playerConfigs[id];
}


// --------------------------------------------------------------------------
// Projectile config
// --------------------------------------------------------------------------
enum class ProjectileType : uint8_t
{
    None = 0,
    Fireball,
    Rocket,
    IceBolt,

    Count // Toujours garder cet élément en dernier pour compter automatiquement le nombre de types
};

struct ProjectileConfig
{
    ProjectileType type = ProjectileType::None;
    float damage       = 0.0f;   // Dégâts infligés à l’impact
    float speed        = 0.0f;   // Vitesse du projectile (ex: 10 unités/s)
    uint32_t lifeDurationMs = 0; // Durée de vie en millisecondes avant auto-destruction/despawn (ex: 3000 ms = 3 secondes)
    float aoeRadius    = 0.0f;   // 0 = pas d'AOE (AOE = les dégâts s'appliquent dans une zone autour du point d'impact)
};

// Tableau statique indexé directement par ProjectileType pour un accès rapide en simulation
inline const ProjectileConfig projectileConfigs[] =
{
    { ProjectileType::None,     0.0f,  0.0f,    0, 0.0f },
    { ProjectileType::Fireball, 20.0f, 12.0f, 1000, 0.0f },
    { ProjectileType::Rocket,   60.0f,  8.0f, 2000, 2.5f },
    { ProjectileType::IceBolt,  10.0f, 14.0f,  100, 0.0f },
};

inline const ProjectileConfig& GetProjectileConfigFromId(uint8_t id)
{
    if (id >= static_cast<uint8_t>(ProjectileType::Count))
        id = static_cast<uint8_t>(ProjectileType::None);

    return projectileConfigs[id];
}


// --------------------------------------------------------------------------
// Item config
// --------------------------------------------------------------------------
enum class ItemType : uint8_t
{
    None = 0,
    HealthPotion, // Potion de soin instantanée
    SpeedBoost,   // Bonus de vitesse temporaire

    Count // Toujours garder cet élément en dernier pour compter automatiquement le nombre de types
};

struct ItemConfig
{
    ItemType type = ItemType::None;    // Type d’item
    int32_t    healAmount     = 0;     // Quantité de soin (ex: 25 points de vie rendus)
    float      speedMultiplier = 1.0f; // Multiplicateur de vitesse (ex: 1.3 = +30% de vitesse)
    uint32_t   effectDurationMs = 0;   // Durée de l’effet en ms (ex: 1800 ms = 1.8 secondes, 0 = effet instantané)
    uint32_t   lifeDurationMs = 0;     // Durée de vie en ms avant auto-destruction/despawn (ex: 30000 ms = 30 secondes, 0 = infinie)
};
    
// Tableau statique indexé directement par ItemType pour un accès rapide en simulation
inline const ItemConfig itemConfigs[] =
{
    { ItemType::None,         0, 1.0f,    0, 0 },
    { ItemType::HealthPotion, 25, 1.0f,   0, 5000 },
    { ItemType::SpeedBoost,   0, 1.3f, 1800, 5000 },
};

inline const ItemConfig& GetItemConfigFromId(uint8_t id)
{
    if (id >= static_cast<uint8_t>(ItemType::Count))
        id = static_cast<uint8_t>(ItemType::None);

    return itemConfigs[id];
}


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