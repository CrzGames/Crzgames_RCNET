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
    const uint8_t normalizedId =
        (id < static_cast<uint8_t>(PlayerType::Count))
            ? id
            : static_cast<uint8_t>(PlayerType::None);

    return playerConfigs[normalizedId];
}
