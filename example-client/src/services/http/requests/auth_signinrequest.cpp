#include "services/http/requests/auth_signinrequest.h"

#include "core/context.h"

#include <cJSON.h>

AuthSigninHTTPResponse ClientHttp_Auth_SigninRequest(const AuthSigninHTTPRequest& request)
{
    // Réponse finale renvoyée au thread HTTP appelant.
    AuthSigninHTTPResponse response{};

    // ... Ici, on ferait la logique réelle de signin, par exemple en envoyant une requête

    return response;
}
