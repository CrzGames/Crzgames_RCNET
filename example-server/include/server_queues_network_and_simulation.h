#pragma once

#include <mutex>   // std::mutex pour protéger l'accès à la file d'attente
#include <deque>   // std::deque pour la file d'attente des messages du réseau vers la simulation
#include <cstdint> // uint16_t, uint32_t, etc.

#include "server_network_clientinput.h"

enum class NetworkMessageType : uint8_t 
{ 
    CONNECT = 0,
    DISCONNECT = 1,
    INPUT = 2,
    HANDSHAKE = 3,
};

struct NetworkToSimulationMessage
{
    // Type de message (connect, disconnect, input, etc.)
    NetworkMessageType type;

    // connectionId pour identifier la connexion réseau (ex: pour les connect/disconnect)
    uint32_t connectionId;

    // Un identifiant de session ou de compte pour savoir à qui s'adresse le message (utile pour les inputs)
    // 0 = pas encore identifié (ex: avant handshake)
    uint64_t accountIdDatabase;

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