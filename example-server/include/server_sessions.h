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
// Données autoritaires spécifiques à un client connecté.
// ============================================================================
struct ClientSession
{
    // =======================================================================
    // Identification du client
    // =======================================================================

    // L'id du compte du client en base de données
    uint64_t accountIdDatabase = 0; 

    // connectionId pour identifier la connexion réseau (ex: pour les connect/disconnect)
    uint32_t connectionId = 0;

    // ============================================================================
    // INPUTS
    // ============================================================================

    // Dernier input reçu de ce client
    ClientInputCommand latestReceivedInputCommand{};

    // Dernier input appliqué dans la simulation serveur (pour savoir si on en a de nouveaux à appliquer)
    uint32_t lastProcessedInputSequenceNumber = 0;

    // Queue d'inputs reçus du client mais pas encore appliqués dans la simulation
    std::deque<ClientInputCommand> pendingInputCommandsQueue;


    // ============================================================================
    // Gameplay
    // ============================================================================

    // L'entité actuellement contrôlée par le client (0 si aucune, ex: en cas de mort)
    PlayerControl control;


    // ============================================================================
    // Réseau
    // ============================================================================
    uint32_t lastAckedSnapshotId = 0;   // dernier snapshot ACK par le client
    uint32_t lastSentSnapshotId  = 0;   // dernier snapshot envoyé
    uint32_t lastAckedInputSeq   = 0;   // si tu ACK les inputs
};