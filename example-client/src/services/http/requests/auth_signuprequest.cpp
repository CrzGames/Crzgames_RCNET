#include "services/http/requests/auth_signuprequest.h"

#include "core/context.h"

#include <cJSON.h>

AuthSignUpHTTPResponse ClientHttp_Auth_SignUpRequest(const AuthSignUpHTTPRequest& request)
{
    // Réponse finale renvoyée au thread HTTP appelant.
    AuthSignUpHTTPResponse response{};

    // ... Ici, on ferait la logique réelle de signup, par exemple en envoyant une requête

    return response;
}
