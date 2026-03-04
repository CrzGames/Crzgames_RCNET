#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <deque>         // std::deque pour la queue d'inputs non encore consommés

#include "server_network_clientinput.h" // ClientInputCommand


// ============================================================================
// Données autoritaires spécifiques à une entité contrôlée par un client.
// ============================================================================
//
// Cette structure indique quelle entité du monde est contrôlée par le client.
// Exemple : un joueur peut contrôler un personnage dans le monde.
// Si le joueur meurt ou est en spectateur, controlledEntityId peut être 0.
//
struct PlayerControl
{
    // Id de l’entité contrôlée par le client dans le monde du jeu
    // 0 = aucune entité (ex: mort, spectateur, pas encore spawn)
    uint32_t controlledEntityId = 0;
};

// ============================================================================
// Données autoritaires spécifiques à un client connecté.
// ============================================================================
//
// Une ClientSession représente l'état complet du joueur côté serveur.
//
// Elle contient :
// - l'identité du joueur
// - les inputs envoyés par le client
// - l'état réseau de synchronisation
// - quelle entité du monde il contrôle
//
struct ClientSession
{
    // =======================================================================
    // Identification du client
    // =======================================================================

    // Indique si le client a réussi le handshake et est authentifié.
    bool isAuthenticated = false;

    // Identifiant unique du compte joueur dans la base de données.
    // Il est généralement obtenu après le handshake / authentification.
    uint64_t accountIdDatabase = 0;

    // Identifiant unique de la connexion réseau active.
    // Généré par le serveur lors du CONNECT.
    // Il change si le joueur se reconnecte.
    uint32_t connectionId = 0;


    // =======================================================================
    // INPUTS (client -> serveur)
    // =======================================================================

    // Dernier input reçu depuis le réseau.
    // Sert principalement à appliquer un état si aucun nouvel input n'arrive.
    ClientInputCommand latestReceivedInputCommand{};

    // Dernier inputSequenceNumber que le serveur a déjà appliqué
    // dans la simulation.
    //
    // Cela permet de :
    // - ignorer les duplications réseau
    // - garantir que les inputs sont traités dans l'ordre
    uint32_t serverLastProcessedInputSequenceNumber = 0;

    // File d'attente des inputs reçus mais pas encore traités
    // par la simulation serveur.
    //
    // Les inputs arrivent depuis le thread réseau puis sont
    // consommés dans le thread de simulation.
    std::deque<ClientInputCommand> pendingInputCommandsQueue;


    // =======================================================================
    // Gameplay
    // =======================================================================

    // Quelle entité du monde ce client contrôle actuellement.
    PlayerControl control;


    // =======================================================================
    // Synchronisation réseau (serveur <-> client)
    // =======================================================================
    //
    // Ces champs servent à suivre ce que le client a reçu
    // et ce que le serveur lui a envoyé.
    //
    // Ils sont utilisés pour :
    // - delta compression des snapshots
    // - prediction client
    // - reconciliation
    // - monitoring réseau


    // Prochain snapshotId que le serveur générera pour ce client.
    //
    // Chaque snapshot envoyé au client possède un identifiant
    // strictement croissant (snapshotId).
    //
    // Exemple :
    // snapshotId = 1
    // snapshotId = 2
    // snapshotId = 3
    //
    // Cela permet au client de détecter les pertes et d'envoyer un ACK.
    uint32_t serverNextSnapshotId = 1;


    // Dernier snapshot envoyé par le serveur à ce client.
    //
    // Ce snapshot peut ne pas encore être confirmé par le client.
    //
    // Exemple :
    // serveur envoie snapshotId = 120
    // serverLastSentSnapshotId = 120
    uint32_t serverLastSentSnapshotId = 0;


    // Dernier snapshot confirmé par le client (ACK).
    //
    // Le client envoie périodiquement un message ACK indiquant
    // le dernier snapshot correctement reçu.
    //
    // Exemple :
    // client ACK snapshotId = 118
    // clientLastAckedSnapshotId = 118
    //
    // Cela permet au serveur :
    // - de calculer un delta snapshot
    // - de libérer les anciens snapshots en mémoire
    uint32_t clientLastAckedSnapshotId = 0;


    // Dernier inputSequenceNumber que le serveur confirme au client.
    //
    // Cette valeur est généralement envoyée dans les snapshots.
    //
    // Elle permet au client de savoir quels inputs ont été
    // réellement appliqués par le serveur.
    //
    // Le client peut alors supprimer ces inputs de son buffer
    // de prediction et ne garder que les inputs non confirmés.
    uint32_t serverLastAckedInputSequenceNumberToClient = 0;
};