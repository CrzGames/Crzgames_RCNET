#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.

// ============================================================================
// Configurations "design" (data-driven)
// Ces données ne sont PAS runtime, elles décrivent le comportement
// des sous-types d'entités.
// ============================================================================

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
    const uint8_t normalizedId =
        (id < static_cast<uint8_t>(ProjectileType::Count))
            ? id
            : static_cast<uint8_t>(ProjectileType::None);

    return projectileConfigs[normalizedId];
}
