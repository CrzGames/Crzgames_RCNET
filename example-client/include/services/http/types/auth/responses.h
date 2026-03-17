#pragma once

#include <string>  // std::string

// ============================================================================
// Response HTTP du backend d'authentification vers le client pour la requete de creation de compte.
// ============================================================================
struct AuthSignUpHTTPResponse
{
    bool success;
    std::string message; // message d'erreur en cas d'echec (ex: "Email already in use")
};

// ============================================================================
// Response HTTP du backend d'authentification vers le client pour la requete de connexion.
// ============================================================================
struct AuthSignInHTTPResponse
{
    bool success;
    std::string message; // message d'erreur en cas d'echec (ex: "Invalid email or password")
};