#include "services/http/requests/auth_signuprequest.h"

#include "core/context.h"

#include <RC2D/RC2D.h>
#include <curl/curl.h>
#include <cJSON.h>

#include <limits>  // std::numeric_limits
#include <string>  // std::string

// Callback libcurl:
// - appelee par libcurl pour chaque chunk recu
// - copie le chunk dans la string de sortie
static size_t ClientHttp_AuthSignUp_WriteResponseBodyCallback(
    void* contents,
    size_t size,
    size_t nmemb,
    void* userData)
{
    // Securite: sans buffer source ou destination, impossible de copier.
    if (contents == nullptr || userData == nullptr)
    {
        // 0 indique a libcurl qu'aucun octet n'a ete consomme.
        return 0;
    }

    // Taille reelle du chunk recu (en bytes).
    const size_t byteCount = size * nmemb;

    // S'il n'y a rien a copier, on sort proprement.
    if (byteCount == 0)
    {
        return 0;
    }

    // Reinterprete le pointeur utilisateur en std::string*.
    std::string* outBody = static_cast<std::string*>(userData);

    // Concatene le chunk recu a la fin du body HTTP accumule.
    outBody->append(static_cast<const char*>(contents), byteCount);

    // Retourne le nombre d'octets consommes pour confirmer a libcurl.
    return byteCount;
}

// Extraction "best effort" d'un message lisible depuis la reponse JSON backend.
static void ClientHttp_AuthSignUp_ExtractMessageFromJson(cJSON* jsonRoot, std::string& outMessage)
{
    // Sans objet JSON, rien a extraire.
    if (jsonRoot == nullptr)
    {
        return;
    }

    // 1) Cas nominal: champ "message".
    const cJSON* messageItem = cJSON_GetObjectItemCaseSensitive(jsonRoot, "message");
    if (cJSON_IsString(messageItem) && messageItem->valuestring != nullptr)
    {
        // Copie le message dans la sortie.
        outMessage = messageItem->valuestring;
        // Message trouve, fin de la fonction.
        return;
    }

    // 2) Fallback: champ "error".
    const cJSON* errorItem = cJSON_GetObjectItemCaseSensitive(jsonRoot, "error");
    if (cJSON_IsString(errorItem) && errorItem->valuestring != nullptr)
    {
        // Copie l'erreur dans la sortie.
        outMessage = errorItem->valuestring;
        // Message trouve, fin de la fonction.
        return;
    }

    // 3) Fallback: tableau "errors".
    const cJSON* errorsItem = cJSON_GetObjectItemCaseSensitive(jsonRoot, "errors");
    if (cJSON_IsArray(errorsItem) && cJSON_GetArraySize(errorsItem) > 0)
    {
        // Recupere la premiere entree d'erreur.
        const cJSON* firstError = cJSON_GetArrayItem(errorsItem, 0);

        // Si la premiere entree est une string, on la prend directement.
        if (cJSON_IsString(firstError) && firstError->valuestring != nullptr)
        {
            outMessage = firstError->valuestring;
            return;
        }

        // Si la premiere entree est un objet, on tente "message".
        if (cJSON_IsObject(firstError))
        {
            const cJSON* firstErrorMessage = cJSON_GetObjectItemCaseSensitive(firstError, "message");
            if (cJSON_IsString(firstErrorMessage) && firstErrorMessage->valuestring != nullptr)
            {
                outMessage = firstErrorMessage->valuestring;
                return;
            }
        }
    }
}

AuthSignUpHTTPResponse ClientHttp_Auth_SignUpRequest(const AuthSignUpHTTPRequest& request)
{
    // Reponse renvoyee au thread HTTP appelant.
    AuthSignUpHTTPResponse response{};

    // Valeur defensive par defaut: echec tant que non prouve.
    response.success = false;

    // Recupere l'etat reseau global (contient baseUrlApi selon environnement).
    const NetworkState& networkState = GetNetworkState();

    // Construit l'URL complete du endpoint signup.
    const std::string url = std::string(networkState.baseUrlApi) + "/auth/sign-up";

    // Cree un objet JSON vide pour le payload POST.
    cJSON* jsonBody = cJSON_CreateObject();

    // Si allocation JSON echoue, retourner une erreur claire.
    if (jsonBody == nullptr)
    {
        response.message = "Failed to allocate signup JSON body.";
        return response;
    }

    // Ajoute le username dans le payload JSON.
    cJSON_AddStringToObject(jsonBody, "username", request.username.c_str());

    // Ajoute l'email dans le payload JSON.
    cJSON_AddStringToObject(jsonBody, "email", request.email.c_str());

    // Ajoute le mot de passe dans le payload JSON.
    cJSON_AddStringToObject(jsonBody, "password", request.password.c_str());

    // Serialize l'objet JSON en texte compact.
    char* jsonBodyText = cJSON_PrintUnformatted(jsonBody);

    // Libere l'objet JSON cJSON (le texte serialize est deja alloue a part).
    cJSON_Delete(jsonBody);

    // Si la serialisation JSON echoue, retourner une erreur claire.
    if (jsonBodyText == nullptr)
    {
        response.message = "Failed to serialize signup JSON body.";
        return response;
    }

    // Copie le JSON serialize dans un std::string C++.
    std::string requestBody = jsonBodyText;

    // Libere le buffer C alloue par cJSON_PrintUnformatted.
    cJSON_free(jsonBodyText);

    // Verifie que la taille est representable en long pour CURLOPT_POSTFIELDSIZE.
    if (requestBody.size() > static_cast<size_t>((std::numeric_limits<long>::max)()))
    {
        response.message = "Signup request body is too large.";
        return response;
    }

    // Initialise un handle libcurl pour cette requete.
    CURL* curl = curl_easy_init();

    // Si init libcurl echoue, retourner une erreur claire.
    if (curl == nullptr)
    {
        response.message = "Failed to initialize CURL for signup request.";
        return response;
    }

    // Buffer qui accumule le body de reponse HTTP.
    std::string responseBody;

    // Liste de headers HTTP a envoyer.
    curl_slist* headers = nullptr;

    // Header JSON pour indiquer le type de body envoye.
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Header JSON pour indiquer le type de body attendu en retour.
    headers = curl_slist_append(headers, "Accept: application/json");

    // Configure l'URL cible.
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // Force la methode HTTP POST.
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    // Applique les headers HTTP.
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Fournit le body POST (JSON texte).
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());

    // Fournit la taille explicite du body POST.
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(requestBody.size()));

    // Enregistre le callback de reception du body HTTP.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ClientHttp_AuthSignUp_WriteResponseBodyCallback);

    // Passe la destination de sortie au callback.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    // Evite les signaux POSIX (utile en contexte multithread).
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    // Timeout de connexion (ms).
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 5000L);

    // Timeout global de requete (ms).
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000L);

    // Log de debug/trace pour savoir quel endpoint est appele.
    RC2D_log(
        RC2D_LOG_INFO,
        "[CLIENT] [HTTP] [AUTH_SIGNUP] POST %s",
        url.c_str());

    // Execute la requete HTTP.
    const CURLcode curlCode = curl_easy_perform(curl);

    // Variable pour stocker le code HTTP (200, 409, etc.).
    long httpStatusCode = 0;

    // Recupere le code status HTTP renvoye par le serveur.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatusCode);

    // Libere la liste des headers.
    curl_slist_free_all(headers);

    // Libere le handle CURL.
    curl_easy_cleanup(curl);

    // Echec niveau transport (DNS/TLS/socket/timeout...).
    if (curlCode != CURLE_OK)
    {
        // Reponse metier: echec.
        response.success = false;

        // Message detaille base sur le code curl.
        response.message = std::string("Signup request failed: ") + curl_easy_strerror(curlCode);

        // Log technique pour diagnostic.
        RC2D_log(
            RC2D_LOG_ERROR,
            "[CLIENT] [HTTP] [AUTH_SIGNUP] CURL error=%d message=%s",
            static_cast<int>(curlCode),
            response.message.c_str());

        // Retour immediat en cas d'erreur transport.
        return response;
    }

    // Indique si le status HTTP est dans la plage 2xx.
    const bool httpSuccess = (httpStatusCode >= 200 && httpStatusCode < 300);

    // Pointeur JSON pour parser la reponse, null par defaut.
    cJSON* responseJson = nullptr;

    // Parse JSON uniquement si le body n'est pas vide.
    if (!responseBody.empty())
    {
        responseJson = cJSON_Parse(responseBody.c_str());
    }

    // Si parse JSON reussi, essaie de lire success/message depuis le JSON.
    if (responseJson != nullptr)
    {
        // Cherche un champ bool "success".
        const cJSON* successItem = cJSON_GetObjectItemCaseSensitive(responseJson, "success");

        // Si le champ success existe et est bien un bool, on l'utilise.
        if (cJSON_IsBool(successItem))
        {
            response.success = cJSON_IsTrue(successItem);
        }
        else
        {
            // Sinon fallback: derive success depuis code HTTP.
            response.success = httpSuccess;
        }

        // Essaie d'extraire un message lisible depuis plusieurs formats backend.
        ClientHttp_AuthSignUp_ExtractMessageFromJson(responseJson, response.message);

        // Libere l'arbre JSON parse.
        cJSON_Delete(responseJson);
    }
    else
    {
        // Si pas de JSON valide, fallback sur succes HTTP brut.
        response.success = httpSuccess;
    }

    // Si aucun message n'a ete recupere, fabrique un message par defaut.
    if (response.message.empty())
    {
        // Message par defaut quand tout est OK.
        if (response.success)
        {
            response.message = "Signup succeeded.";
        }
        else
        {
            // Message par defaut quand echec HTTP/metier.
            response.message = std::string("Signup failed (HTTP ") + std::to_string(httpStatusCode) + ").";
        }
    }

    // Log final de resultat metier.
    RC2D_log(
        response.success ? RC2D_LOG_INFO : RC2D_LOG_WARN,
        "[CLIENT] [HTTP] [AUTH_SIGNUP] HTTP status=%ld success=%u message=%s",
        httpStatusCode,
        response.success ? 1u : 0u,
        response.message.c_str());

    // Retourne la reponse finale au dispatcher HTTP.
    return response;
}
