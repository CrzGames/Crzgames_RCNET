#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.

// ============================================================================
// Données d’input envoyées par le client au serveur
// ============================================================================
enum class MovementFlags : uint8_t
{
    None  = 0,
    Up    = 1u << 0,
    Down  = 1u << 1,
    Left  = 1u << 2,
    Right = 1u << 3,
    // etc. (ajoute autant de directions que nécessaire, jusqu’à 8)
};

enum class ActionFlags : uint64_t
{
    None   = 0,
    Jump   = 1ull << 0,
    Dash   = 1ull << 1,
    Sprint = 1ull << 2,
    Attack = 1ull << 3,
    // etc. (ajoute autant d’actions que nécessaire, jusqu’à 64)
};

// ============================================================================
// Commande d'input (ce que le client envoie)
// ============================================================================
struct ClientInputCommand
{
    // =========================================================================
    // Ordonnancement / Synchronisation
    // =========================================================================

    // Numéro de séquence strictement monotone côté client.
    // Permet au serveur :
    //  - de traiter les inputs dans l'ordre correct
    //  - d’ignorer les doublons (retransmissions)
    //  - d’envoyer un ACK pour la reconciliation côté client
    uint32_t inputSequenceNumber = 0;

    // Tick logique client au moment où cet input a été généré.
    // Utilisé pour :
    //  - debug
    //  - estimation de latence
    //  - éventuelle lag compensation (avec validation serveur)
    uint32_t clientTick = 0;


    // =========================================================================
    // MOUVEMENT
    // =========================================================================
    // Etat des directions du joueur (bitmask MovementFlags).
    // Représente uniquement des intentions gameplay, indépendantes du clavier.

    // Directions actuellement maintenues.
    // C’est la source principale utilisée par le serveur pour
    // calculer la vitesse et la direction à chaque tick.
    uint8_t movementHeldFlags = 0;

    // Directions qui viennent d’être activées sur CE tick (transition 0 -> 1).
    // Utile pour détecter un "tap" directionnel ou déclencher
    // une action dépendante d’une direction précise (ex: dash).
    // Optionnel si ton gameplay ne nécessite pas de détection de transition.
    uint8_t movementPressedFlags = 0;

    // Directions qui viennent d’être désactivées sur CE tick (transition 1 -> 0).
    // Rarement nécessaire pour un mouvement classique,
    // mais utile si certaines mécaniques dépendent du relâchement.
    uint8_t movementReleasedFlags = 0;


    // =========================================================================
    // ACTIONS (Jump, Dash, Sprint, Shoot, etc.)
    // =========================================================================
    // Intentions gameplay abstraites (indépendantes du périphérique d’entrée).

    // Actions actuellement maintenues.
    // Utilisé pour les actions continues : sprint, tir automatique, etc.
    uint64_t actionHeldFlags = 0;

    // Actions déclenchées exactement sur ce tick (transition 0 -> 1).
    // Indispensable pour les actions instantanées : jump, dash,
    // tir semi-automatique, interaction.
    // Garantit qu’une action ne soit déclenchée qu’une seule fois
    // même en cas de maintien du bouton ou de reconciliation réseau.
    uint64_t actionPressedFlags = 0;

    // Actions relâchées sur ce tick (transition 1 -> 0).
    // Utile pour les mécaniques dépendant du relâchement :
    // attaque chargée, arrêt d’un sprint, fin d’une visée, etc.
    // Peut être omis si ton gameplay ne l’utilise pas.
    uint64_t actionReleasedFlags = 0;
};