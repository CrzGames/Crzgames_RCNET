#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.
#include <deque>   // std::deque
#include <mutex>   // std::mutex, std::lock_guard
#include <string>  // std::string

// ======================================================================================
// Messages du thread NATS vers la simulation (NATS -> Simulation)
// ======================================================================================

enum class NatsToSimulationMessageType : uint8_t
{
    // Recoit un message depuis le sujet NATS : mysubject1.
    MESSAGE_RECEIVED_FOR_SUBJECT_MYSUBJECT1 = 0,
};

struct NatsToSimulationMessage
{
    // Type de message.
    NatsToSimulationMessageType type;

    // Sujet NATS d'origine du message.
    std::string subject;

    // Pour le type MESSAGE_RECEIVED_FOR_SUBJECT_MYSUBJECT1
    std::string mySubject1Message;
};

struct NatsToSimulationQueue
{
    std::mutex mtx;
    std::deque<NatsToSimulationMessage> q;

    void push(const NatsToSimulationMessage& m)
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push_back(m);
    }

    void drain(std::deque<NatsToSimulationMessage>& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        out.swap(q);
    }
};
