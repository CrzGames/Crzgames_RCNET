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
struct AuthTokenVerificationHTTPRequest
{
    // Token d'authentification à vérifier auprès du backend.
    std::string authToken;
};

// ============================================================================
// Response HTTP du backend d'authentification vers le serveur après vérification du token d'authentification.
// ============================================================================
struct AuthTokenVerificationHTTPResponse
{
    // Indique si le token d'authentification est valide ou non.
    bool isValid = false;

    // Message d'erreur en cas de token invalide (ex: token expiré, compte banni, etc.)
    std::string errorMessage = "";

    // Identifiant unique du compte joueur dans la base de données.
    uint64_t accountIdDatabase = 0;

    // Nom d'utilisateur du compte joueur.
    std::string accountUsername = "";
};