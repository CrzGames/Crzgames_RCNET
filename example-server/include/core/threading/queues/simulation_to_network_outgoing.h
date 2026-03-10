#pragma once

#include <mutex>   // std::mutex
#include <deque>   // std::deque
#include <cstdint> // uint16_t, uint32_t, etc.
#include <vector>  // std::vector

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
    SERVER_CLOCK_SYNC_PACKET_UNRELIABLE = 6,
};

struct SimulationToNetworkOUTMessage
{
    // Type de message (snapshot full, delta, event, etc.)
    SimulationToNetworkOUTMessageType type;

    // à qui envoyer
    uint32_t connectionId = 0;

    // Si true, le serveur doit fermer la connexion du client 
    // après l'envoi de ce message dès que le client aura accusé de réception du packet correspondant.
    bool disconnectAfterAck = false;

    // Si true, le serveur active le chiffrement réseau pour ce client
    // dès que ce message a été accusé de réception.
    bool enableEncryptionAfterAck = false;

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
