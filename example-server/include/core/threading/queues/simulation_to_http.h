#pragma once

#include <mutex>   // std::mutex
#include <deque>   // std::deque
#include <cstdint> // uint16_t, uint32_t, etc.
#include <condition_variable> // std::condition_variable, std::unique_lock

#include "auth/types.h"
#include "services/http/types/auth/requests.h"

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
    std::condition_variable cv;
    std::deque<SimulationToHttpMessage> q;
    bool stopped = false;

    void push(const SimulationToHttpMessage& m)
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            q.push_back(m);
        }
        cv.notify_one();
    }

    bool waitAndDrain(std::deque<SimulationToHttpMessage>& out)
    {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [this] {
            return stopped || !q.empty();
        });

        if (stopped && q.empty())
        {
            return false;
        }

        // Echange atomique sous lock: le thread HTTP traite ensuite hors lock.
        out.swap(q);
        return true;
    }

    void stop(void)
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            stopped = true;
        }
        cv.notify_all();
    }

    void reset(void)
    {
        std::lock_guard<std::mutex> lock(mtx);
        stopped = false;
        q.clear();
    }
};