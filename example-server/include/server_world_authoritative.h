#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <deque>         // std::deque pour la queue d’inputs par client
#include <unordered_map> // std::unordered_map pour stocker les clients connectés

#include <rcenet/RCENET_enet.h>

#include "server_clientinput_authoritative.h"

// ============================================================================
// Données autoritaires spécifiques à une entité contrôlée par un client (ex: quel PlayerType il contrôle, etc.)
// ============================================================================
struct PlayerControl
{
    // Id de l’entité contrôlée par le client (0 si aucune, ex: en cas de mort)
    uint32_t controlledEntityId = 0;
};

// ============================================================================
// Données autoritaires spécifiques à un client connecté (ex: quelle entité il contrôle, etc.)
// ============================================================================
struct ClientSession
{
    // Pointeur ENet vers le client connecté
    ENetPeer* enetPeer = nullptr;

    // L'id du compte du client en base de données
    uint64_t accountIdDatabase = 0; 

    // Dernier input reçu, appliqué chaque tick dans la simulation serveur
    ClientInputCommand latestReceivedInputCommand{};

    // Dernière séquence traitée côté serveur pour ce client (pour éviter de traiter plusieurs fois le même input en cas de lag)
    uint32_t lastProcessedInputSequenceNumber = 0;

    // Queue des inputs reçus mais pas encore “consommés” par la simulation serveur
    std::deque<ClientInputCommand> pendingInputCommandsQueue;

    // Données autoritaires spécifiques au client (ex: quelle entité le client contrôle, etc.)
    PlayerControl control;
};

// ============================================================================
// API world serveur (appelée depuis server.cpp)
// ============================================================================
void example_server_world_reset(void);

// Connect/disconnect
uint32_t example_server_world_on_client_connected(ENetPeer* peerPointer);
void example_server_world_on_client_disconnected(ENetPeer* peerPointer);

// Réception d’un input et push dans la queue d’inputs du client correspondant
void example_server_world_push_input_command(ENetPeer* peerPointer, const ClientInputCommand& inputCommand);

// Simulation 120Hz (ou autre tick rate défini dans main.cpp)
void example_server_world_simulate_fixed_step(double fixedDeltaSeconds, uint32_t currentServerTick);

// Accès snapshot
const std::unordered_map<uint32_t, ClientSession>& example_server_world_get_connected_clients(void);

// Outils temps
uint32_t example_server_world_get_server_time_ms_monotonic(void);