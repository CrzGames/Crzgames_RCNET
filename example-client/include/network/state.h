#pragma once

#include <rcenet/RCENET_enet.h> // ENetPeer

#include "crypto/kx.h"          // ClientCryptoKxState

struct NetworkState
{
    // ------------------------------------------------------------------------
    // Connections - ATTENTION: thread reseau UNIQUEMENT
    // ------------------------------------------------------------------------

    ENetPeer* peerServer;   // ENetPeer* du serveur (cible reseau ENet)
    bool encryptionEnabled; // false tant que la secure-session n'est pas validee (ACK cote reseau)

    // ------------------------------------------------------------------------
    // Crypto - ATTENTION: thread simulation UNIQUEMENT
    // ------------------------------------------------------------------------

    // Etat KX (X25519/libsodium crypto_kx):
    // - contient la cle KX client (publique/privee) utilisee pour derivation rx/tx
    // - generee au boot du client
    ClientCryptoKxState cryptoKxState;
};
