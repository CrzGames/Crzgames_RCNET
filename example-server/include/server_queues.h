#pragma once

#include <mutex>   // std::mutex
#include <deque>   // std::deque
#include <cstdint> // uint16_t, uint32_t, etc.
#include <vector>  // std::vector
#include <string>  // std::string

#include "server_network_packets_client_unreliable.h"
#include "server_network_packets_client_reliable.h"
#include "server_network_packets_server_unreliable.h"
#include "server_network_packets_server_reliable.h"
#include "server_auth.h"

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

    // payload brut à envoyer (contenant le packet sérialisé correspondant au type de message)
    std::vector<uint8_t> serializedPacket;
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
    std::deque<SimulationToHttpMessage> q;

    void push(const SimulationToHttpMessage& m)
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push_back(m);
    }

    void drain(std::deque<SimulationToHttpMessage>& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        out.swap(q);
    }
};

// ======================================================================================
// Messages du thread HTTP vers la simulation (HTTP -> Simulation)
// ======================================================================================

enum class HttpToSimulationMessageType : uint8_t
{
    AUTH_VALIDATE_TOKEN_RESPONSE = 0,
};

struct HttpToSimulationMessage
{
    HttpToSimulationMessageType type;

    // Client concerné
    uint32_t connectionId = 0;

    // Pour le type AUTH_VALIDATE_TOKEN_RESPONSE, la réponse du backend d'authentification après vérification du token
    AuthTokenVerificationHTTPResponse authTokenVerificationResponse;
};

struct HttpToSimulationQueue
{
    std::mutex mtx;
    std::deque<HttpToSimulationMessage> q;

    void push(const HttpToSimulationMessage& m)
    {
        std::lock_guard<std::mutex> lock(mtx);
        q.push_back(m);
    }

    void drain(std::deque<HttpToSimulationMessage>& out)
    {
        std::lock_guard<std::mutex> lock(mtx);
        out.swap(q);
    }
};