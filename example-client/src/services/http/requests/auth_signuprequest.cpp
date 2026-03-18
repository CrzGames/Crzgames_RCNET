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

// Lit un champ string obligatoire dans un objet JSON.
static bool ClientHttp_AuthSignUp_ReadRequiredString(
    const cJSON* jsonObject,
    const char* key,
    std::string& outValue)
{
    // Sans objet source ni nom de champ, lecture impossible.
    if (jsonObject == nullptr || key == nullptr)
    {
        return false;
    }

    // Chercher le champ cible dans l'objet.
    const cJSON* valueItem = cJSON_GetObjectItemCaseSensitive(jsonObject, key);
    if (!cJSON_IsString(valueItem) || valueItem->valuestring == nullptr)
    {
        // Echec si le champ est absent ou n'est pas une string valide.
        return false;
    }

    // Copier la valeur string en sortie.
    outValue = valueItem->valuestring;
    return true;
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

    // Memorise le status HTTP dans la reponse metier.
    response.httpStatusCode = httpStatusCode;

    // Parse JSON uniquement si le body n'est pas vide.
    cJSON* responseJson = nullptr;
    if (!responseBody.empty())
    {
        responseJson = cJSON_Parse(responseBody.c_str());
    }

    // ---------------------------------------------------------------------
    // Cas succes attendu: HTTP 201
    // Payload:
    // { "message": "Account created successfully" }
    // ---------------------------------------------------------------------
    if (httpStatusCode == 201)
    {
        response.success = true;

        if (responseJson != nullptr)
        {
            ClientHttp_AuthSignUp_ReadRequiredString(responseJson, "message", response.message);
            cJSON_Delete(responseJson);
        }

        if (response.message.empty())
        {
            response.message = "Account created successfully";
        }

        RC2D_log(
            RC2D_LOG_INFO,
            "[CLIENT] [HTTP] [AUTH_SIGNUP] HTTP status=%ld success=%u message=%s",
            response.httpStatusCode,
            response.success ? 1u : 0u,
            response.message.c_str());
        return response;
    }

    // ---------------------------------------------------------------------
    // Cas echec (ex: HTTP 500)
    // Payload attendu:
    // {
    //   "code": "E_INTERNAL_SERVER_ERROR",
    //   "message": "Failed to create account"
    // }
    // ---------------------------------------------------------------------
    response.success = false;

    if (responseJson != nullptr)
    {
        ClientHttp_AuthSignUp_ReadRequiredString(responseJson, "code", response.code);
        ClientHttp_AuthSignUp_ReadRequiredString(responseJson, "message", response.message);
        cJSON_Delete(responseJson);
    }

    if (response.message.empty())
    {
        response.message = std::string("Signup failed (HTTP ") + std::to_string(httpStatusCode) + ").";
    }

    // Log final de resultat metier.
    RC2D_log(
        RC2D_LOG_WARN,
        "[CLIENT] [HTTP] [AUTH_SIGNUP] HTTP status=%ld success=%u code=%s message=%s",
        response.httpStatusCode,
        response.success ? 1u : 0u,
        response.code.empty() ? "<none>" : response.code.c_str(),
        response.message.c_str());

    // Retourne la reponse finale au dispatcher HTTP.
    return response;
}
