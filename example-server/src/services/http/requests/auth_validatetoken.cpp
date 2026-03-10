#include "services/http/requests/auth_validatetoken.h"

#include "core/context.h"

#include <cJSON.h>

AuthTokenVerificationHTTPResponse ServerHttp_Auth_ValidateTokenRequest(const AuthTokenVerificationHTTPRequest& request)
{
    // Réponse finale renvoyée au thread HTTP appelant.
    AuthTokenVerificationHTTPResponse response{};

    // Récupère le client HTTP global.
    httplib::Client& cli = GetHttpClient();

    // =========================================================================
    // 1) Construire le JSON de requête
    // =========================================================================
    cJSON* requestRoot = cJSON_CreateObject();
    if (requestRoot == nullptr)
    {
        response.isValid = false;
        response.errorMessage = "Failed to create JSON request object";
        return response;
    }

    if (cJSON_AddStringToObject(requestRoot, "authToken", request.authToken.c_str()) == nullptr)
    {
        cJSON_Delete(requestRoot);
        response.isValid = false;
        response.errorMessage = "Failed to add authToken to JSON request";
        return response;
    }

    char* requestBodyRaw = cJSON_PrintUnformatted(requestRoot);
    cJSON_Delete(requestRoot);

    if (requestBodyRaw == nullptr)
    {
        response.isValid = false;
        response.errorMessage = "Failed to serialize JSON request";
        return response;
    }

    std::string requestBody = requestBodyRaw;
    cJSON_free(requestBodyRaw);

    // =========================================================================
    // 2) Envoyer la requête HTTP au backend d'authentification
    // =========================================================================
    auto result = cli.Post(
        "/auth/validate-token",
        requestBody,
        "application/json"
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

    // =========================================================================
    // 3) Parser le JSON de réponse
    // =========================================================================
    cJSON* responseRoot = cJSON_Parse(result->body.c_str());
    if (responseRoot == nullptr)
    {
        response.isValid = false;
        response.errorMessage = "Failed to parse JSON response";
        return response;
    }

    // Champs attendus :
    // {
    //   "isValid": true,
    //   "errorMessage": "",
    //   "accountIdDatabase": 123,
    //   "accountUsernameDatabase": "Corentin"
    // }

    cJSON* isValidItem = cJSON_GetObjectItemCaseSensitive(responseRoot, "isValid");
    cJSON* errorMessageItem = cJSON_GetObjectItemCaseSensitive(responseRoot, "errorMessage");
    cJSON* accountIdItem = cJSON_GetObjectItemCaseSensitive(responseRoot, "accountIdDatabase");
    cJSON* accountUsernameItem = cJSON_GetObjectItemCaseSensitive(responseRoot, "accountUsernameDatabase");

    if (cJSON_IsBool(isValidItem))
    {
        response.isValid = cJSON_IsTrue(isValidItem);
    }

    if (cJSON_IsString(errorMessageItem) && errorMessageItem->valuestring != nullptr)
    {
        response.errorMessage = errorMessageItem->valuestring;
    }

    if (cJSON_IsNumber(accountIdItem))
    {
        response.accountIdDatabase = static_cast<uint64_t>(accountIdItem->valuedouble);
    }

    if (cJSON_IsString(accountUsernameItem) && accountUsernameItem->valuestring != nullptr)
    {
        response.accountUsernameDatabase = accountUsernameItem->valuestring;
    }

    cJSON_Delete(responseRoot);
    return response;
}