#include "services/http/requests/auth_signinrequest.h"

#include "core/context.h"

#include <cJSON.h>

AuthSignInHTTPResponse ClientHttp_Auth_SignInRequest(const AuthSignInHTTPRequest& request)
{
    // Réponse finale renvoyée au thread HTTP appelant.
    AuthSignInHTTPResponse response{};

    // ... Ici, on ferait la logique réelle de signin, par exemple en envoyant une requête

    return response;
}
