#pragma once

#include <mutex>   // std::mutex
#include <deque>   // std::deque
#include <cstdint> // uint16_t, uint32_t, etc.

#include "auth/types.h"
#include "services/http/types/auth/responses.h"

// ======================================================================================
// Messages du thread HTTP vers la simulation (HTTP -> Simulation)
// ======================================================================================

enum class HttpToSimulationMessageType : uint8_t
{
    AUTH_VALIDATE_TOKEN_RESPONSE = 0,
};

struct HttpToSimulationMessage
{
    HttpToSimulationMessageType type;

    // Client concerné
    uint32_t connectionId = 0;

    // Pour le type AUTH_VALIDATE_TOKEN_RESPONSE, la réponse du backend d'authentification après vérification du token
    AuthTokenVerificationHTTPResponse authTokenVerificationResponse;
};

struct HttpToSimulationQueue
{
    std::mutex mtx;
    std::deque<HttpToSimulationMessage> q;

    void push(const HttpToSimulationMessage& m)
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push_back(m);
    }

    void drain(std::deque<HttpToSimulationMessage>& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        out.swap(q);
    }
};