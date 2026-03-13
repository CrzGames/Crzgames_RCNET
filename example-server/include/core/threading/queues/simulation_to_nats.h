#pragma once

#include <condition_variable> // std::condition_variable, std::unique_lock
#include <cstdint>            // uint16_t, uint32_t, etc.
#include <deque>              // std::deque
#include <mutex>              // std::mutex, std::lock_guard
#include <string>             // std::string

// ======================================================================================
// Messages de la simulation vers le thread NATS (Simulation -> NATS)
// ======================================================================================

enum class SimulationToNatsMessageType : uint8_t
{
    // Publie un message sur le sujet NATS : mysubject1.
    PUBLISH_MESSAGE_FOR_SUBJECT_MYSUBJECT1 = 0,
};

struct SimulationToNatsMessage
{
    SimulationToNatsMessageType type;

    // Sujet NATS sur lequel publier le message.
    std::string subject;

    // Pour le type PUBLISH_MESSAGE_FOR_SUBJECT_MYSUBJECT1
    std::string mySubject1Message;
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

    bool waitAndPop(SimulationToNatsMessage& out)
    {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [this] {
            return stopped || !q.empty();
        });

        if (stopped && q.empty())
        {
            return false;
        }

        out = std::move(q.front());
        q.pop_front();
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
