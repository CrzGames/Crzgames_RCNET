#pragma once

#include <array>   // std::array
#include <cstdint> // uint16_t, uint32_t, etc.
#include <deque>   // std::deque
#include <string>  // std::string

#include <sodium.h> // crypto_kx_PUBLICKEYBYTES, crypto_kx_SESSIONKEYBYTES

#include "auth/types.h"                        // AuthStatus
#include "network/packets/client/unreliable.h" // ClientInputPacketUnreliable

// ============================================================================
// Donnees autoritaires specifiques a une entite controlee par un client.
// ============================================================================
struct PlayerControl
{
    // Id de l'entite controlee dans le monde.
    // 0 = aucune entite (mort, spectateur, pas encore spawn, etc.).
    uint32_t controlledEntityId = 0;
};

// ============================================================================
// Donnees autoritaires specifiques a un client connecte.
// ============================================================================
struct ClientSession
{
    // =======================================================================
    // Identification du client
    // =======================================================================

    // Cle publique envoyee par le client pendant secure session hello.
    std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> clientPublicKey{};

    // Cles derivees cote serveur:
    // - serverRxKey: cle utilisee pour dechiffrer ce que le client envoie.
    // - serverTxKey: cle utilisee pour chiffrer ce que le serveur envoie.
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> serverRxKey{};
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> serverTxKey{};

    // Etat d'authentification.
    AuthStatus authStatus = AuthStatus::None;

    // Message d'erreur backend en cas de AuthStatus::Invalid.
    std::string authErrorMessage = "";

    // True apres l'evenement ENET_EVENT_TYPE_CONNECT.
    bool isTransportConnected = false;

    // True quand l'etape secure-session est validee cote serveur
    // (cles de session derivees et reponse SUCCESS preparee).
    //
    // Ce flag est utilise par les guards de channels (AUTH/GAME)
    // pour autoriser le flux applicatif apres secure-session.
    //
    // Important: ce flag ne signifie pas que le chiffrement reseau est deja actif.
    // L'activation effective du chiffrement est portee par isPacketEncryptionEnabled.
    bool isSecureSessionEstablished = false;

    // Devient vrai uniquement apres ACK de
    // SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE.
    // Tant que ce flag est false, les callbacks ENet encrypt/decrypt restent en passthrough.
    bool isPacketEncryptionEnabled = false;

    // True quand le serveur recoit CLIENT_READY_FOR_MATCH.
    bool isReadyForMatch = false;

    // Identifiant du compte joueur en base.
    uint64_t accountIdDatabase = 0;

    // Username du compte joueur en base.
    std::string accountUsernameDatabase = "";

    // Identifiant unique de la connexion active.
    uint32_t connectionId = 0;


    // =======================================================================
    // INPUTS (client -> serveur)
    // =======================================================================

    // Dernier input recu depuis le reseau.
    ClientInputPacketUnreliable latestReceivedInputPacket{};

    // Dernier inputSequenceNumber deja applique en simulation.
    uint32_t serverLastProcessedInputSequenceNumber = 0;

    // Inputs recus mais pas encore traites par la simulation.
    std::deque<ClientInputPacketUnreliable> pendingInputPacketsQueue;


    // =======================================================================
    // Gameplay
    // =======================================================================

    // Entite du monde actuellement controlee par ce client.
    PlayerControl control;


    // =======================================================================
    // Synchronisation reseau (serveur <-> client)
    // =======================================================================

    // Prochain snapshotId genere par le serveur pour ce client.
    uint32_t serverNextSnapshotId = 1;

    // Dernier snapshot effectivement envoye a ce client.
    uint32_t serverLastSentSnapshotId = 0;

    // Dernier snapshot confirme (ACK) par ce client.
    uint32_t clientLastAckedSnapshotId = 0;
};
