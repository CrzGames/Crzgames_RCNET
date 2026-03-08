#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <unordered_map> // std::unordered_map
#include <string>        // std::string

#include <rcenet/RCENET_enet.h> // ENetPeer

#include "network/client_session.h" // ClientSession
#include "network/channels/channel.h" // NetworkChannel
#include "crypto/crypto_kx.h"         // ServerCryptoKxState

struct NetworkState
{
    // ------------------------------------------------------------------------
    // Configuration du serveur
    // ------------------------------------------------------------------------

    // Port d'écoute du serveur
    static constexpr uint16_t serverPort = 12345;

    // Nombre de channels ENet utilisés
    static constexpr uint8_t channelCount = static_cast<uint8_t>(NetworkChannel::COUNT);

    // Nombre maximum de clients connectés
    static constexpr uint32_t maxClientsConnected = 2;

    // Fréquence de tick de simulation du serveur en Hz (ex: 128)
    static constexpr uint32_t simulationTickRateHz = 128;

    // Fréquence de tick réseau OUT du serveur en Hz (ex: 32)
    static constexpr uint32_t networkOutgoingTickRateHz = 32;

    // Durée de sommeil entre chaque tick réseau IN en ms (ex: 1)
    static constexpr uint32_t networkIncomingSleepMs = 1;


    // --------------------------------------------------------------------------
    // API externe (pour les appels HTTP vers l'API du jeu, ex: pour checker les tokens d'authentification, etc.)
    // --------------------------------------------------------------------------

#if SERVER_ENV_DEV
    static constexpr std::string baseUrlApi = "http://localhost:3400";
#elif SERVER_ENV_STAGING
    static constexpr std::string baseUrlApi = "https://staging.api.aetherroyale.crzgames.com";
#elif SERVER_ENV_PRODUCTION
    static constexpr std::string baseUrlApi = "https://api.aetherroyale.crzgames.com";
#endif


    // ------------------------------------------------------------------------
    // Connections - ATTENTION: Thread RÉSEAU UNIQUEMENT
    // ------------------------------------------------------------------------
    
    // Incrémenté à chaque nouvelle connexion pour lui donner un ID unique (différent de l'accountIdDatabase)
    uint32_t nextConnectionId = 1; // commence à 1 pour éviter les confusions avec une valeur "0" non initialisée

    // --- mapping de connectionId vers ENetPeer* pour envoyer des messages à un client spécifique ---
    std::unordered_map<uint32_t, ENetPeer*> connectionIdToEnetPeer; // key = connectionId, value = ENetPeer*


    // ------------------------------------------------------------------------
    // Sessions - ATTENTION: Thread SIMULATION UNIQUEMENT
    // ------------------------------------------------------------------------

    // Mapping de connectionId vers ClientSession (sessions actives pour les clients connectés)
    std::unordered_map<uint32_t, ClientSession> sessions; // key = connectionId, value = ClientSession

    // Données de chiffrement pour l'établissement de session sécurisée via libsodium (ex: échange de clés, etc.)
    ServerCryptoKxState cryptoKxState;
};