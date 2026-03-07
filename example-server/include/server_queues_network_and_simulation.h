#pragma once

#include <mutex>   // std::mutex pour protéger l'accès à la file d'attente
#include <deque>   // std::deque pour la file d'attente des messages du réseau vers la simulation
#include <cstdint> // uint16_t, uint32_t, etc.
#include <vector>  // std::vector pour le payload des messages de la simulation vers le réseau

#include "server_network_packets_client_unreliable.h"
#include "server_network_packets_client_reliable.h"
#include "server_network_packets_server_unreliable.h"
#include "server_network_packets_server_reliable.h"

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
    uint32_t connectionId;

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


// ======================================================================================
// Messages de la simulation vers le réseau (Simulation -> Network OUT)
// ======================================================================================
enum class SimulationToNetworkOUTMessageType : uint8_t
{
    SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE = 0,
    SERVER_AUTH_RESPONSE_PACKET_RELIABLE = 1,
    SERVER_MATCH_INIT_PACKET_RELIABLE = 2,
    SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE = 3,
    SERVER_WORLD_STATIC_STATE_INIT_PACKET_RELIABLE = 4,
    SERVER_MATCH_START_PACKET_RELIABLE = 5,
    // plus tard: SNAPSHOT_DELTA, EVENT, etc.
};

struct SimulationToNetworkOUTMessage
{
    // Type de message (snapshot full, delta, event, etc.)
    SimulationToNetworkOUTMessageType type;

    // à qui envoyer
    uint32_t connectionId = 0;

    // payload brut
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