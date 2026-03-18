#include "services/http/requests/auth_validatetokenrequest.h"

#include "core/context.h"

#include <cJSON.h>

// Build a readable transport-level error for auth token validation requests.
static std::string ServerHttp_BuildValidateTokenTransportErrorMessage(httplib::Error error)
{
    switch (error)
    {
    case httplib::Error::Connection:
        return "HTTP transport error (connection failed)";
    case httplib::Error::ConnectionTimeout:
    case httplib::Error::Timeout:
        return "HTTP transport error (request timed out)";
    case httplib::Error::Read:
    case httplib::Error::Write:
        return "HTTP transport error (socket read/write failure)";
    case httplib::Error::InvalidHeaders:
        return "HTTP transport error (invalid request headers; check auth token format)";
    default:
        return "HTTP transport error (" + httplib::to_string(error) + ")";
    }
}

AuthTokenVerificationHTTPResponse ServerHttp_Auth_ValidateTokenRequest(const AuthTokenVerificationHTTPRequest& request)
{
    // Réponse finale renvoyée au thread HTTP appelant.
    AuthTokenVerificationHTTPResponse response{};

    // Récupère le client HTTP global initialisé au démarrage du serveur.
    httplib::Client& cli = GetHttpClient();

    // Construit le header Authorization standard avec le bearer token.
    const httplib::Headers headers = {
        { "Authorization", "Bearer " + request.authToken }
    };

    // Envoie la requête HTTP au backend d'authentification.
    const auto result = cli.Post(
        "/auth/validate-token",
        headers,
        std::string{},
        "application/json"
    );

    // Si la requête HTTP a échoué au niveau transport.
    if (!result)
    {
        response.isValid = false;
        response.errorMessage = ServerHttp_BuildValidateTokenTransportErrorMessage(result.error());
        return response;
    }

    // Parse le JSON retourné par le backend.
    cJSON* responseRoot = cJSON_Parse(result->body.c_str());
    if (responseRoot == nullptr)
    {
        response.isValid = false;
        response.errorMessage = "Failed to parse JSON response";
        return response;
    }

    // -------------------------------------------------------------------------
    // Cas succès : token valide
    // -------------------------------------------------------------------------
    if (result->status == 200)
    {
        cJSON* userItem = cJSON_GetObjectItemCaseSensitive(responseRoot, "user");
        if (!cJSON_IsObject(userItem))
        {
            cJSON_Delete(responseRoot);
            response.isValid = false;
            response.errorMessage = "Missing or invalid 'user' object in JSON response";
            return response;
        }

        cJSON* idItem = cJSON_GetObjectItemCaseSensitive(userItem, "id");
        cJSON* usernameItem = cJSON_GetObjectItemCaseSensitive(userItem, "username");

        if (!cJSON_IsNumber(idItem))
        {
            cJSON_Delete(responseRoot);
            response.isValid = false;
            response.errorMessage = "Missing or invalid 'user.id' in JSON response";
            return response;
        }

        if (!cJSON_IsString(usernameItem) || usernameItem->valuestring == nullptr)
        {
            cJSON_Delete(responseRoot);
            response.isValid = false;
            response.errorMessage = "Missing or invalid 'user.username' in JSON response";
            return response;
        }

        response.isValid = true;
        response.errorMessage.clear();
        response.accountIdDatabase = static_cast<uint64_t>(idItem->valuedouble);
        response.accountUsernameDatabase = usernameItem->valuestring;

        cJSON_Delete(responseRoot);
        return response;
    }

    // -------------------------------------------------------------------------
    // Cas auth refusée : token invalide / expiré / absent
    // -------------------------------------------------------------------------
    if (result->status == 401)
    {
        cJSON* messageItem = cJSON_GetObjectItemCaseSensitive(responseRoot, "message");

        response.isValid = false;

        if (cJSON_IsString(messageItem) && messageItem->valuestring != nullptr)
        {
            response.errorMessage = messageItem->valuestring;
        }
        else
        {
            response.errorMessage = "Authentication required or invalid token or expired";
        }

        cJSON_Delete(responseRoot);
        return response;
    }

    // -------------------------------------------------------------------------
    // Cas inattendu : erreur backend
    // -------------------------------------------------------------------------
    response.isValid = false;
    response.errorMessage = "Auth API returned unexpected HTTP status: " + std::to_string(result->status);

    cJSON_Delete(responseRoot);
    return response;
}

