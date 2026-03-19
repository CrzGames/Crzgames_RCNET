#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <string_view>   // std::string_view

#include "network/channels/channel.h" // NetworkChannel

struct ServerConfig
{
    // ------------------------------------------------------------------------
    // Configuration du serveur ENet
    // ------------------------------------------------------------------------

    // Port d'écoute du serveur
    static constexpr uint16_t serverPort = 12345;

    // Nombre de channels ENet utilisés
    static constexpr uint8_t channelCount = static_cast<uint8_t>(NetworkChannel::COUNT);

    // Nombre maximum de clients connectés
    static constexpr uint32_t maxClientsConnected = 4000;

    // ------------------------------------------------------------------------
    // Configuration des tick rates du moteur
    // ------------------------------------------------------------------------

    // Fréquence de tick de simulation du serveur en Hz (ex: 128)
    static constexpr uint32_t simulationTickRateHz = 128;

    // Fréquence de tick réseau OUT du serveur en Hz (ex: 64)
    static constexpr uint32_t networkOutgoingTickRateHz = 64;

    // Durée de poll réseau entrant en ms (ex: 1)
    static constexpr uint32_t networkIncomingPollTimeoutMs = 1;

    // --------------------------------------------------------------------------
    // API externe (pour les appels HTTP vers l'API du jeu, ex: pour checker les tokens d'authentification, etc.)
    // --------------------------------------------------------------------------

#if SERVER_ENV_DEV
    static constexpr std::string_view baseUrlApi = "http://localhost:3400";
#elif SERVER_ENV_STAGING
    static constexpr std::string_view baseUrlApi = "https://staging-aetherroyale-backend-service.staging-aetherroyale-backend.svc.cluster.local:3333";
#elif SERVER_ENV_PRODUCTION
    static constexpr std::string_view baseUrlApi = "https://prod-aetherroyale-backend-service.prod-aetherroyale-backend.svc.cluster.local:3333";
#else
#error "Define one of SERVER_ENV_DEV, SERVER_ENV_STAGING or SERVER_ENV_PRODUCTION"
#endif

    // ------------------------------------------------------------------------
    // Configuration NATS
    // ------------------------------------------------------------------------

    // Bypass la vérification des certificats TLS du serveur NATS (uniquement si `natsUseTLS` est à `true`)
    static constexpr bool natsSkipVerifyCertsServer = true;

#if SERVER_ENV_DEV
    static constexpr const char* natsServerURL = "nats://localhost:4222";
    static constexpr bool natsUseTLS = false;
    static constexpr const char* natsPublicKeyNKey = "UCFS4XRTGO7OUO3GCYYXOU4CYQJRPJ47DKJ2VMPLOX2EFEENU2BG44RW";
    static constexpr const char* natsPrivateKeySeedNKey = "SUAG4ONEN4NCQHDTVEVW4TSYTSTOMSUBI4QOUIBSUQXKAD4HJ5PSK6QLYM";
#elif SERVER_ENV_STAGING
    static constexpr const char* natsServerURL = "nats://nats.nats.svc.cluster.local:4222";
    static constexpr bool natsUseTLS = false;
    // En staging, les clés NKey sont fournies via des variables d'environnement pour éviter de les hardcoder dans le code source.
#elif SERVER_ENV_PRODUCTION
    static constexpr const char* natsServerURL = "nats://nats.nats.svc.cluster.local:4222";
    static constexpr bool natsUseTLS = false;
    // En production, les clés NKey sont fournies via des variables d'environnement pour éviter de les hardcoder dans le code source.
#else
#error "Define one of SERVER_ENV_DEV, SERVER_ENV_STAGING or SERVER_ENV_PRODUCTION"
#endif
};