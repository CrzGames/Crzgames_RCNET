#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.
#include <string>  // std::string

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
    std::string accountUsernameDatabase = "";
};