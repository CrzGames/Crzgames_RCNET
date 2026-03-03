#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.

// ============================================================================
// Configuration réseau ENet et paramètres généraux du serveur
// ============================================================================

// Port d’écoute du serveur (Les port UDP ont une plage de 0 à 65535, évite les ports < 1024 qui sont réservés)
static constexpr uint16_t serverPort = 12345;

// Nombre max de clients acceptés par le serveur (ENet supporte 4096 clients max)
static constexpr size_t serverMaxConnectedClients = 64;

// Fréquence de la simulation serveur (logique, physique, etc.)
static constexpr int serverSimulationTickRateHz = 128;

// Fréquence des updates réseau (envoi de snapshots, etc.)
static constexpr int serverNetworkTickRateHz = 32;

// ENet channels (à utiliser lors de l’envoi de paquets pour différencier les types de données, ex: inputs vs snapshots vs events importants)
enum class NetworkChannel : uint8_t
{
    // Inputs fréquents (unreliable sequenced)
    InputUnreliableSequenced = 0,

    // Snapshots fréquents (unreliable sequenced)
    SnapshotUnreliableSequenced = 1,

    // Events importants (reliable)
    ReliableEvents = 2,
};

// ============================================================================
// API utilisée par server.cpp
// ============================================================================

// Démarre le serveur (initialisation ENet, bind, listen)
bool example_server_network_start(void);

// Arrête le serveur (cleanup ENet, disconnect clients, etc.)
void example_server_network_stop(void);

// Reçoit des paquets (inputs, connect/disconnect) et remplit les queues d’inputs
void example_server_network_pump_incoming_events(void);

// Envoie un snapshot à tous les clients connectés
void example_server_network_broadcast_world_snapshot(uint32_t serverTick, uint32_t serverTimeMs);