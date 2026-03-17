#include "services/http/requests/auth_signuprequest.h"

#include "core/context.h"

#include <cJSON.h>

AuthSignupHTTPResponse ClientHttp_Auth_SignupRequest(const AuthSignupHTTPRequest& request)
{
    // Réponse finale renvoyée au thread HTTP appelant.
    AuthSignupHTTPResponse response{};

    // ... Ici, on ferait la logique réelle de signup, par exemple en envoyant une requête

    return response;
}
