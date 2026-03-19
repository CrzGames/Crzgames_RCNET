#pragma once

#include <condition_variable> // std::condition_variable, std::unique_lock
#include <cstdint>            // uint16_t, uint32_t, etc.
#include <deque>              // std::deque
#include <mutex>              // std::mutex, std::lock_guard
#include <string>             // std::string

#include "services/nats/types/gameserver_simulation_etat_metrics_payload.h" // GameserverSimulationEtatMetricsPayload

// ======================================================================================
// Messages de la simulation vers le thread NATS (Simulation -> NATS)
// ======================================================================================

enum class SimulationToNatsMessageType : uint8_t
{
    // Publie un message sur le sujet NATS : mysubject1.
    PUBLISH_MESSAGE_FOR_SUBJECT_MYSUBJECT1 = 0,

    // Publie un message sur le sujet NATS des metrics simulation du game server.
    PUBLISH_MESSAGE_FOR_SUBJECT_GAMESERVER_SIMULATION_ETAT_METRICS = 1,
};

struct SimulationToNatsMessage
{
    SimulationToNatsMessageType type;

    // Sujet NATS sur lequel publier le message.
    std::string subject;

    // Pour le type PUBLISH_MESSAGE_FOR_SUBJECT_MYSUBJECT1
    std::string mySubject1Message;

    // Pour le type PUBLISH_MESSAGE_FOR_SUBJECT_GAMESERVER_SIMULATION_ETAT_METRICS
    GameserverSimulationEtatMetricsPayload gameserverSimulationEtatMetricsPayload{};
};

struct SimulationToNatsQueue
{
    std::mutex mtx;
    std::condition_variable cv;
    std::deque<SimulationToNatsMessage> q;
    bool stopped = false;

    void push(const SimulationToNatsMessage& m)
    {
        {
            std::lock_guard<std::mutex> lock(mtx);
            q.push_back(m);
        }
        cv.notify_one();
    }

    bool waitAndDrain(std::deque<SimulationToNatsMessage>& out)
    {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [this] {
            return stopped || !q.empty();
        });

        if (stopped && q.empty())
        {
            return false;
        }

        // Echange atomique sous lock: le thread NATS traite ensuite hors lock.
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
