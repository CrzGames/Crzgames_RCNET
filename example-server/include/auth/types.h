#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.

// ============================================================================
// Statut d'authentification pour la session d'un client connecté.
// ============================================================================
enum class AuthStatus : uint8_t
{
    // Aucun statut d'authentification défini (par défaut à la connexion)
    None = 0,
    // Requête partie au thread HTTP, réponse pas encore revenue
    WaitingAuth,
    // Authentification validée avec succès auprès du backend
    Valid,
    // Authentification refusée par le backend (ex: token invalide, token expiré, compte banni, etc.)
    Invalid,
};