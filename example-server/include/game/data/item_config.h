#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.

// ============================================================================
// Configurations "design" (data-driven)
// Ces données ne sont PAS runtime, elles décrivent le comportement
// des sous-types d'entités.
// ============================================================================

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
    const uint8_t normalizedId =
        (id < static_cast<uint8_t>(ItemType::Count))
            ? id
            : static_cast<uint8_t>(ItemType::None);

    return itemConfigs[normalizedId];
}
