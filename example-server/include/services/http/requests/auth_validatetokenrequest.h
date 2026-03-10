#pragma once

#include "services/http/types/auth/requests.h"
#include "services/http/types/auth/responses.h"

AuthTokenVerificationHTTPResponse ServerHttp_Auth_ValidateTokenRequest(const AuthTokenVerificationHTTPRequest& request);