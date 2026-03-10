#include "services/http/requests/auth_validatetoken.h"

#include "core/context.h"

AuthTokenVerificationHTTPResponse ServerHttp_Auth_ValidateTokenRequest(const AuthTokenVerificationHTTPRequest& request)
{
    // Réponse finale renvoyée au thread HTTP appelant.
    AuthTokenVerificationHTTPResponse response{};

    // Récupère le client HTTP global initialisé au démarrage du serveur.
    httplib::Client& cli = GetHttpClient();

    // Exemple très simple :
    // - on envoie le token brut
    // - le backend répond avec un JSON
    //
    // Adapte ici selon ton API réelle.
    auto result = cli.Post(
        "/auth/validate-token",
        request.token,
        "text/plain"
    );

    // Si la requête HTTP a complètement échoué (connexion, timeout, TLS, etc.)
    if (!result)
    {
        response.isValid = false;
        response.errorMessage = "HTTP request failed";
        return response;
    }

    // Si le backend a répondu avec un code HTTP inattendu.
    if (result->status != 200)
    {
        response.isValid = false;
        response.errorMessage = "Auth API returned unexpected HTTP status";
        return response;
    }

    // Ici tu parses result->body selon ton format réel.
    // Exemple fictif :
    //
    // {
    //   "isValid": true,
    //   "accountIdDatabase": 123,
    //   "accountUsernameDatabase": "Corentin"
    // }

    // TODO: parser le body JSON ici
    // response.isValid = ...
    // response.accountIdDatabase = ...
    // response.accountUsernameDatabase = ...
    // response.errorMessage = ...

    return response;
}