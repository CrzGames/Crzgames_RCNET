#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <string>        // std::string

#include "network/channels/channel.h" // NetworkChannel

struct ServerConfig
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

    // Durée de poll réseau entrant en ms (ex: 1)
    static constexpr uint32_t networkIncomingPollTimeoutMs = 1;

    // Durée de sommeil entre chaque tick du thread HTTP en ms (ex: 1)
    static constexpr uint32_t httpThreadSleepMs = 1;

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
};