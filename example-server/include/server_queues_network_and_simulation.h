#pragma once

#include <mutex>   // std::mutex pour protéger l'accès à la file d'attente
#include <deque>   // std::deque pour la file d'attente des messages du réseau vers la simulation
#include <cstdint> // uint16_t, uint32_t, etc.
#include <vector>  // std::vector pour le payload des messages de la simulation vers le réseau

#include "server_network_packets_client_unreliable.h"
#include "server_network_packets_client_reliable.h"
#include "server_network_packets_server_unreliable.h"
#include "server_network_packets_server_reliable.h"

enum class NetworkINToSimulationMessageType : uint8_t 
{ 
    CLIENT_EVENT_CONNECT = 0,
    CLIENT_EVENT_DISCONNECT = 1, 
    CLIENT_INPUT_PACKET_UNRELIABLE = 2,
    CLIENT_HANDSHAKE_PACKET_RELIABLE = 3,
    CLIENT_READY_FOR_MATCH_PACKET_RELIABLE = 4,
};

struct NetworkINToSimulationMessage
{
    // Type de message (connect, disconnect, input, etc.)
    NetworkINToSimulationMessageType type;

    // Identifier la connexion réseau (connectionId) à partir de event->peer->data
    uint32_t connectionId;

    // Packet d'input reçu du client (uniquement pour les messages de type CLIENT_INPUT_PACKET_UNRELIABLE)
    ClientInputPacketUnreliable inputPacket;

    // Packet de handshake reçu du client (uniquement pour les messages de type CLIENT_HANDSHAKE_PACKET_RELIABLE)
    ClientHandshakePacketReliable handshakePacket;
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
    SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE = 0,
    SERVER_MATCH_INIT_PACKET_RELIABLE = 1,
    SERVER_WORLD_STATIC_STATE_INIT_PACKET_RELIABLE = 2,
    SERVER_MATCH_START_PACKET_RELIABLE = 3,
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