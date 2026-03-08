#pragma once

#include <mutex>   // std::mutex
#include <deque>   // std::deque
#include <cstdint> // uint16_t, uint32_t, etc.

#include "network/packets/client/unreliable.h"
#include "network/packets/client/reliable.h"

// ======================================================================================
// Queues de messages entre le réseau et la simulation (Network IN -> Simulation)
// ======================================================================================
enum class NetworkINToSimulationMessageType : uint8_t 
{ 
    CLIENT_SECURE_SESSION_HELLO_PACKET_RELIABLE = 0,
    CLIENT_AUTH_PACKET_RELIABLE = 1,
    CLIENT_EVENT_CONNECT = 2,
    CLIENT_EVENT_DISCONNECT = 3, 
    CLIENT_INPUT_PACKET_UNRELIABLE = 4,
    CLIENT_READY_FOR_MATCH_PACKET_RELIABLE = 5,
};

struct NetworkINToSimulationMessage
{
    // Type de message (connect, disconnect, input, etc.)
    NetworkINToSimulationMessageType type;

    // Identifier la connexion réseau (connectionId) à partir de event->peer->data
    uint32_t connectionId = 0;

    // type = CLIENT_INPUT_PACKET_UNRELIABLE
    ClientInputPacketUnreliable inputPacket;

    // type = CLIENT_SECURE_SESSION_HELLO_PACKET_RELIABLE
    ClientSecureSessionHelloPacketReliable secureSessionHelloPacket;

    // type = CLIENT_AUTH_PACKET_RELIABLE
    ClientAuthPacketReliable authPacket;

    // type = CLIENT_READY_FOR_MATCH_PACKET_RELIABLE
    ClientReadyForMatchPacketReliable readyForMatchPacket;
};

struct NetworkINToSimulationQueue
{
    std::mutex mtx;
    std::deque<NetworkINToSimulationMessage> q;

    void push(const NetworkINToSimulationMessage& m)
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push_back(m);
    }

    // drain en une fois (moins de lock)
    void drain(std::deque<NetworkINToSimulationMessage>& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        out.swap(q);
    }
};