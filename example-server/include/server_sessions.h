#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <deque>         // std::deque pour la queue d'inputs non encore consommés

#include "server_network_clientinput.h" // ClientInputCommand

// ============================================================================
// Données autoritaires spécifiques à une entité contrôlée par un client.
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
    // L'id du compte du client en base de données
    uint64_t accountIdDatabase = 0; 

    // Dernier input reçu, appliqué chaque tick dans la simulation serveur
    ClientInputCommand latestReceivedInputCommand{};

    // Dernière séquence traitée côté serveur pour ce client (pour éviter de traiter plusieurs fois le même input en cas de lag)
    uint32_t lastProcessedInputSequenceNumber = 0;

    // Queue des inputs reçus mais pas encore “consommés” par la simulation serveur
    std::deque<ClientInputCommand> pendingInputCommandsQueue;

    // Données autoritaires spécifiques au client (ex: quelle entité le client contrôle.)
    PlayerControl control;
};