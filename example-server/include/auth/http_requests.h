#pragma once

#include <string> // std::string

// ============================================================================
// Request HTTP du serveur vers le backend d'authentification pour vérifier un token d'authentification.
// ============================================================================
struct AuthTokenVerificationHTTPRequest
{
    // Token d'authentification à vérifier auprès du backend.
    std::string authToken;
};