#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <deque>         // std::deque
#include <array>         // std::array

#include <sodium.h> // crypto_kx_PUBLICKEYBYTES, crypto_kx_SESSIONKEYBYTES

#include "server_network_packets_client_unreliable.h"
#include "server_auth.h"

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

    // Clé publique du client pour établir une session sécurisée via libsodium.
    // Elle est reçue dans le packet CLIENT_SECURE_SESSION_HELLO_PACKET_RELIABLE.
    // Elle est utilisée pour générer les clés de session (serverRxKey, serverTxKey) via crypto_kx_server_session_keys de libsodium.
    std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> clientPublicKey{};
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> serverRxKey{};
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> serverTxKey{};

    // Set à true après authentification validé par le serveur au près du backend / api d'authentification.
    AuthStatus authStatus = AuthStatus::None;

    // Set à true à la connexion lors de l'événement ENET_EVENT_TYPE_CONNECT, 
    bool isTransportConnected = false;

    // Set à true lorsque serveur à reçu un packet reliable de type CLIENT_SECURE_SESSION_HELLO_PACKET_RELIABLE et 
    // que le serveur à envoyer un packet reliable de type SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE pour établir la session sécurisée avec le client.
    bool isSecureSessionEstablished = false;

    // Devient vrai lorsque le serveur reçoit un packet reliable
    // de type CLIENT_READY_FOR_MATCH pour cette session.
    // À partir de ce moment, le serveur peut considérer que le client
    // a terminé son initialisation locale et est prêt à recevoir
    // les informations de démarrage effectif du match.
    bool isReadyForMatch = false;

    // Identifiant unique du compte joueur dans la base de données.
    // Il est généralement obtenu après le check du token d'authentification auprès du backend d'authentification.
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
    ClientInputPacketUnreliable latestReceivedInputPacket{};

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
    std::deque<ClientInputPacketUnreliable> pendingInputPacketsQueue;


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
};