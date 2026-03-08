#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.

// ============================================================================
// Types d'entités principaux du jeu
// ============================================================================
enum class EntityType : uint8_t
{
    None       = 0,
    Player     = 1, // ex: personnage contrôlé par un client (guerrier, mage, voleur, etc.)
    Projectile = 2, // ex: boule de feu, rocket, etc.
    Item       = 3  // ex: potion de soin, bonus de vitesse, etc.
};

// ============================================================================
// Etat runtime minimal d'une entité
// ============================================================================
struct EntityState
{
    // Id runtime unique de l’entité (généré par le serveur à la création,
    // jamais réutilisé même après destruction)
    uint32_t entityId = 0;

    // Type principal de l'entité
    EntityType type = EntityType::None;

    // Sous-type interprété selon 'type' (ex: si type = Projectile, alors subtypeId = ProjectileType)
    uint8_t subtypeId = 0;

    // Entité propriétaire (utile pour projectiles, par exemple pour savoir qui a tiré la boule de feu)
    // 0 = aucun propriétaire
    uint32_t ownerEntityId = 0;

    // Etat physique simple
    float positionX = 0.0f;
    float positionY = 0.0f;
    float velocityX = 0.0f;
    float velocityY = 0.0f;

    // Pour les projectiles et items avec durée de vie limitée,
    // on peut stocker le tick de simulation où ils doivent être détruits
    uint64_t endTick = 0;
};