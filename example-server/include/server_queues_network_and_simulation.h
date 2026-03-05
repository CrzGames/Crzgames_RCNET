#pragma once

#include <mutex>   // std::mutex pour protéger l'accès à la file d'attente
#include <deque>   // std::deque pour la file d'attente des messages du réseau vers la simulation
#include <cstdint> // uint16_t, uint32_t, etc.
#include <vector>  // std::vector pour le payload des messages de la simulation vers le réseau

#include "server_network_input_packets.h"

enum class NetworkINToSimulationMessageType : uint8_t 
{ 
    CONNECT = 0,
    DISCONNECT = 1, 
    PACKET_INPUT = 2,
    PACKET_HANDSHAKE = 3,
    PACKET_EVENT_IMPORTANT = 4,
};

struct NetworkINToSimulationMessage
{
    // Type de message (connect, disconnect, input, etc.)
    NetworkINToSimulationMessageType type;

    // Identifier la connexion réseau (connectionId) à partir de event->peer->data
    uint32_t connectionId;

    // L'input du client (valide seulement si type == PACKET_INPUT)
    ClientInputCommand input{};
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



enum class SimulationToNetworkOUTMessageType : uint8_t
{
    SNAPSHOT_FULL = 0,
    // plus tard: SNAPSHOT_DELTA, EVENT, etc.
};

struct SimulationToNetworkOUTMessage
{
    // Type de message (snapshot full, delta, event, etc.)
    SimulationToNetworkOUTMessageType type;

    // à qui envoyer
    uint32_t connectionId = 0;

    // payload brut (full snapshot pour commencer)
    std::vector<uint8_t> payload;
};

struct SimulationToNetworkOUTQueue
{
    std::mutex mtx;
    std::deque<SimulationToNetworkOUTMessage> q;

    void push(const SimulationToNetworkOUTMessage& m)
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push_back(m);
    }

    void drain(std::deque<SimulationToNetworkOUTMessage>& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        out.swap(q);
    }
};