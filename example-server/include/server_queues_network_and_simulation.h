#pragma once

#include <mutex>   // std::mutex pour protéger l'accès à la file d'attente
#include <deque>   // std::deque pour la file d'attente des messages du réseau vers la simulation
#include <cstdint> // uint16_t, uint32_t, etc.
#include <vector>  // std::vector pour le payload des messages de la simulation vers le réseau

#include "server_network_input_packets.h"

enum class NetworkToSimulationMessageType : uint8_t 
{ 
    CONNECT = 0,
    DISCONNECT = 1, 
    INPUT = 2,
    HANDSHAKE = 3,
};

struct NetworkToSimulationMessage
{
    // Type de message (connect, disconnect, input, etc.)
    NetworkToSimulationMessageType type;

    // connectionId pour identifier la connexion réseau
    uint32_t connectionId;

    // L'input du client (valide seulement si type == INPUT)
    ClientInputCommand input{};
};

struct NetworkToSimulationQueue
{
    std::mutex mtx;
    std::deque<NetworkToSimulationMessage> q;

    void push(const NetworkToSimulationMessage& m)
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push_back(m);
    }

    // drain en une fois (moins de lock)
    void drain(std::deque<NetworkToSimulationMessage>& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        out.swap(q);
    }
};



enum class SimulationToNetworkMessageType : uint8_t
{
    SNAPSHOT_FULL = 0,
    // plus tard: SNAPSHOT_DELTA, EVENT, etc.
};

struct SimulationToNetworkMessage
{
    SimulationToNetworkMessageType type;

    // à qui envoyer
    uint32_t connectionId = 0;

    // id snapshot (utile maintenant et indispensable pour ACK/delta plus tard)
    uint32_t snapshotId = 0;

    // payload brut (full snapshot pour commencer)
    std::vector<uint8_t> payload;
};

struct SimulationToNetworkQueue
{
    std::mutex mtx;
    std::deque<SimulationToNetworkMessage> q;

    void push(const SimulationToNetworkMessage& m)
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push_back(m);
    }

    void drain(std::deque<SimulationToNetworkMessage>& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        out.swap(q);
    }
};