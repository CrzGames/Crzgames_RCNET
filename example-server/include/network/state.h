#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <mutex>         // std::mutex
#include <string>        // std::string
#include <unordered_map> // std::unordered_map

#include <rcenet/RCENET_enet.h> // ENetPeer

#include "crypto/kx.h"          // ServerCryptoKxState
#include "crypto/signing.h"     // ServerCryptoSigningState
#include "simulation/session/client.h" // ClientSession

struct NetworkState
{
    // Identifiant unique du serveur.
    // Recuperer via le SDK Agones lors de l'initialisation du serveur.
    uint64_t serverId;

    // ------------------------------------------------------------------------
    // Connections - ATTENTION: thread reseau UNIQUEMENT
    // ------------------------------------------------------------------------

    // Incremente a chaque nouvelle connexion pour produire un ID local unique
    // (different de l'accountIdDatabase).
    uint32_t nextConnectionId = 1; // commence a 1 pour eviter la confusion avec la valeur 0

    // Mapping: connectionId -> ENetPeer* (cible reseau ENet).
    std::unordered_map<uint32_t, ENetPeer*> connectionIdToEnetPeer; // key = connectionId, value = ENetPeer*

    // Mapping: connectionId -> chiffrement actif.
    // false tant que la secure-session n'est pas validee (ACK cote reseau).
    std::unordered_map<uint32_t, bool> connectionIdToEncryptionEnabled; // key = connectionId, value = encryption enabled

    // ------------------------------------------------------------------------
    // Sessions - ATTENTION: thread simulation + thread reseau
    // (acces proteges par sessionsMutex)
    // ------------------------------------------------------------------------

    // Mapping: connectionId -> ClientSession (etat runtime de chaque client).
    std::unordered_map<uint32_t, ClientSession> sessions; // key = connectionId, value = ClientSession

    // Mutex unique de protection du conteneur sessions.
    mutable std::mutex sessionsMutex;

    // ------------------------------------------------------------------------
    // Crypto - ATTENTION: thread simulation UNIQUEMENT
    // ------------------------------------------------------------------------

    // Etat KX (X25519/libsodium crypto_kx):
    // - contient la cle KX serveur (publique/privee) utilisee pour derivation rx/tx
    // - generee au boot du serveur
    ServerCryptoKxState cryptoKxState;

    // Etat de signature Ed25519:
    // - cle d'identite serveur (publique/privee)
    // - sert a signer l'attestation secure-session envoyee au client
    // - le client doit hardcoder (pinner) la cle publique correspondante
    //   pour verifier qu'il parle au bon serveur
    ServerCryptoSigningState cryptoSigningState;
};
