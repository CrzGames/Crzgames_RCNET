#pragma once

#include <array>       // std::array
#include <mutex>       // std::mutex
#include <string_view> // std::string_view

#include <rcenet/RCENET_enet.h> // ENetPeer
#include <sodium.h>             // crypto_kx_SESSIONKEYBYTES

#include "crypto/kx.h"          // ClientCryptoKxState
#include "network/protocol/secure_session.h" // CLIENT_SECURE_SESSION_CLIENT_NONCE_BYTES

struct NetworkState
{
    // ------------------------------------------------------------------------
    // Connections - ATTENTION: thread reseau UNIQUEMENT
    // ------------------------------------------------------------------------

    // ENetPeer* du serveur, initialement nullptr, valide apres connexion reussie.
    ENetPeer* peerServer = nullptr;


    // ------------------------------------------------------------------------
    // Crypto - protegee par mutex (thread reseau + simulation)
    // ------------------------------------------------------------------------

    // Etat KX (X25519/libsodium crypto_kx):
    // - contient la cle KX client (publique/privee) utilisee pour derivation rx/tx
    // - generee au boot du client
    ClientCryptoKxState cryptoKxState{};

    // Vrai une fois la secure-session validee cote client
    // (hello response avec status SUCCESS, nonce verifie,
    // signature verifiee, et cles de session derivees).
    bool secureSessionEstablished = false;

    // Vrai des que le client recoit une reponse d'authentification du backend web.
    // Ce flag indique que l'etape d'authentification a ete traitee.
    bool authValidated = false;

    // Vrai uniquement si le serveur du jeu a valide le token d'authentification
    // (AuthResponse status == SUCCESS).
    bool authTokenValidated = false;

    // Vrai quand le chiffrement transport ENet est actif pour le peer serveur
    // (peerServer). Dans le flux actuel ce flag passe a true juste apres
    // secureSessionEstablished.
    bool encryptionEnabled = false;

    // Vrai apres envoi du packet CLIENT_SECURE_SESSION_HELLO_PACKET_RELIABLE
    // et avant reception du packet
    // SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE.
    bool hasPendingSecureSessionHello = false;

    // Nonce envoye dans le dernier hello secure-session.
    std::array<uint8_t, CLIENT_SECURE_SESSION_CLIENT_NONCE_BYTES> pendingClientNonce{};

    // Cles derivees cote client:
    // - clientTxKey: utilisee pour chiffrer les paquets sortants client->serveur
    // - clientRxKey: utilisee pour dechiffrer les paquets entrants serveur->client
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> clientTxKey{};
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> clientRxKey{};

    // Mutex de protection des champs crypto/session utilises
    // depuis plusieurs threads (reseau + simulation).
    std::mutex cryptoMutex;


    // --------------------------------------------------------------------------
    // API externe (backend web d'authentification, pour signup/signin)
    // --------------------------------------------------------------------------

#if GAME_ENV_DEV
    static constexpr std::string_view baseUrlApi = "http://localhost:3400";
#elif GAME_ENV_STAGING
    static constexpr std::string_view baseUrlApi = "https://staging.api.aetherroyale.crzgames.com";
#elif GAME_ENV_PRODUCTION
    static constexpr std::string_view baseUrlApi = "https://api.aetherroyale.crzgames.com";
#else
#error "Define one of GAME_ENV_DEV, GAME_ENV_STAGING or GAME_ENV_PRODUCTION"
#endif
};
