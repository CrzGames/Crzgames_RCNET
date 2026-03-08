#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.
#include <string>  // std::string

// ============================================================================
// Statut d'authentification pour la session d'un client connecté.
// ============================================================================
enum class AuthStatus : uint8_t
{
    // Aucun statut d'authentification défini (par défaut à la connexion)
    None = 0,
    // En attente de validation du token d'authentification auprès du backend
    WaitingAuth,
    // Authentification validée avec succès auprès du backend
    Valid,
    // Authentification refusée par le backend (ex: token invalide, token expiré, compte banni, etc.)
    Invalid,
};

// ============================================================================
// Request HTTP du serveur vers le backend d'authentification pour vérifier un token d'authentification.
// ============================================================================
struct AuthVerificationHTTPRequest
{
    // Token d'authentification à vérifier auprès du backend.
    std::string authToken;
};

// ============================================================================
// Response HTTP du backend d'authentification vers le serveur après vérification du token d'authentification.
// ============================================================================
struct AuthVerificationHTTPResponse
{
    bool isValid = false;
    uint64_t accountIdDatabase = 0;
};