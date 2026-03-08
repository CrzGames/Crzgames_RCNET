#pragma once

// ============================================================================
// Statut d'authentification du client
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
// Request HTTP du serveur vers le backend d'authentification pour valider un token d'authentification.
// ============================================================================
struct AuthHTTPRequest
{
    // Token d'authentification à valider auprès du backend.
    std::string authToken;
};

// ============================================================================
// Response HTTP du backend d'authentification vers le serveur après validation d'un token d'authentification.
// ============================================================================
struct AuthHTTPResponse
{
    bool isValid = false;
    uint64_t accountIdDatabase = 0;
};