#pragma once

#include <mutex>   // std::mutex
#include <deque>   // std::deque
#include <cstdint> // uint16_t, uint32_t, etc.

#include "auth/types.h"
#include "auth/http_requests.h"

// ======================================================================================
// Messages de la simulation vers le thread HTTP (Simulation -> HTTP)
// ======================================================================================

enum class SimulationToHttpMessageType : uint8_t
{
    AUTH_VALIDATE_TOKEN_REQUEST = 0,
};

struct SimulationToHttpMessage
{
    SimulationToHttpMessageType type;

    // Client concerné
    uint32_t connectionId = 0;

    // Pour le type AUTH_VALIDATE_TOKEN_REQUEST, le token à valider auprès du backend
    AuthTokenVerificationHTTPRequest authTokenVerificationRequest;
};

struct SimulationToHttpQueue
{
    std::mutex mtx;
    std::deque<SimulationToHttpMessage> q;

    void push(const SimulationToHttpMessage& m)
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push_back(m);
    }

    void drain(std::deque<SimulationToHttpMessage>& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        out.swap(q);
    }
};