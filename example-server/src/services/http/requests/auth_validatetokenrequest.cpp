#include "services/http/requests/auth_validatetokenrequest.h"

#include "core/context.h"

#include <cJSON.h>

AuthTokenVerificationHTTPResponse ServerHttp_Auth_ValidateTokenRequest(const AuthTokenVerificationHTTPRequest& request)
{
    // =========================================================================
    // Réponse finale qui sera renvoyée au thread HTTP appelant
    // =========================================================================
    AuthTokenVerificationHTTPResponse response{};

    // Récupère le client HTTP global initialisé au démarrage du serveur.
    httplib::Client& cli = GetHttpClient();

    // =========================================================================
    // 1) Construire les headers HTTP
    // =========================================================================
    //
    // Le backend attend un header standard de la forme :
    // Authorization: Bearer <token>
    //
    // Exemple :
    // Authorization: Bearer eyJhbGciOi...
    //
    httplib::Headers headers = {
        { "Authorization", "Bearer " + request.authToken }
    };

    // =========================================================================
    // 2) Envoyer la requête HTTP au backend d'authentification
    // =========================================================================
    //
    // Ici on conserve une requête POST sur /auth/validate-token.
    // Le body est vide car le token est transmis dans le header Authorization.
    //
    auto result = cli.Post(
        "/auth/validate-token",
        headers,
        std::string{},
        "application/json"
    );

    // Si la requête HTTP a complètement échoué :
    // - serveur backend inaccessible
    // - timeout
    // - erreur TLS
    // - connexion coupée
    // etc.
    if (!result)
    {
        response.isValid = false;
        response.errorMessage = "HTTP request failed";
        return response;
    }

    // =========================================================================
    // 3) Parser le JSON de réponse du backend
    // =========================================================================
    //
    // Réponses attendues côté backend :
    //
    // Cas succès (HTTP 200) :
    // {
    //   "user": {
    //     "id": 123,
    //     "username": "Corentin"
    //   }
    // }
    //
    // Cas erreur (HTTP 401) :
    // {
    //   "message": "Invalid or expired token"
    // }
    //
    cJSON* responseRoot = cJSON_Parse(result->body.c_str());
    if (responseRoot == nullptr)
    {
        response.isValid = false;
        response.errorMessage = "Failed to parse JSON response";
        return response;
    }

    // -------------------------------------------------------------------------
    // Cas 1 : le backend a validé le token
    // -------------------------------------------------------------------------
    if (result->status == 200)
    {
        // Récupère l'objet "user" dans le JSON de réponse.
        cJSON* userItem = cJSON_GetObjectItemCaseSensitive(responseRoot, "user");

        // Vérifie que "user" est bien un objet JSON.
        if (!cJSON_IsObject(userItem))
        {
            cJSON_Delete(responseRoot);
            response.isValid = false;
            response.errorMessage = "Missing or invalid 'user' object in JSON response";
            return response;
        }

        // Récupère les champs attendus dans l'objet user.
        cJSON* idItem = cJSON_GetObjectItemCaseSensitive(userItem, "id");
        cJSON* usernameItem = cJSON_GetObjectItemCaseSensitive(userItem, "username");

        // Vérifie et copie l'id utilisateur si présent.
        if (cJSON_IsNumber(idItem))
        {
            response.accountIdDatabase = static_cast<uint64_t>(idItem->valuedouble);
        }

        // Vérifie et copie le username si présent.
        if (cJSON_IsString(usernameItem) && usernameItem->valuestring != nullptr)
        {
            response.accountUsernameDatabase = usernameItem->valuestring;
        }

        // Marque le token comme valide.
        response.isValid = true;
        response.errorMessage = "";

        cJSON_Delete(responseRoot);
        return response;
    }

    // -------------------------------------------------------------------------
    // Cas 2 : le backend a refusé le token ou répondu avec une erreur
    // -------------------------------------------------------------------------
    //
    // On tente de récupérer un champ "message" pour le propager à la simulation.
    //
    cJSON* messageItem = cJSON_GetObjectItemCaseSensitive(responseRoot, "message");

    response.isValid = false;

    if (cJSON_IsString(messageItem) && messageItem->valuestring != nullptr)
    {
        response.errorMessage = messageItem->valuestring;
    }
    else
    {
        response.errorMessage = "Auth API returned unexpected HTTP status";
    }

    cJSON_Delete(responseRoot);
    return response;
}