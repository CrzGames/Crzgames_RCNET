#pragma once

// ======================================================================================
// ServerReliablePacketType
//
// Type de packet envoyé sur le channel reliable serveur -> client.
//
// Tous les packets reliable doivent commencer par ServerReliablePacketHeader
// pour permettre au client de dispatcher correctement.
// ======================================================================================
enum class ServerReliablePacketType : uint8_t
{
    MATCH_INIT = 0,
    WORLD_STATIC_STATE_INIT = 1,
    MATCH_END = 2,
};

#pragma pack(push, 1)

struct ServerReliablePacketHeader
{
    ServerReliablePacketType type;
};

struct MatchInitPacket
{
    // Header commun à tous les packets reliable serveur -> client
    ServerReliablePacketHeader header;

    // Tick logique de simulation serveur auquel ce packet a été construit.
    uint64_t serverTick;

    // Tickrate simulation serveur
    uint32_t serverTickRateHz;

    // Tick de simulation auquel le match commence officiellement.
    uint64_t matchStartTick;

    // Durée du compte à rebours avant le début du match en ticks de simulation (ex: 128 ticks = 1s si tick rate = 128Hz).
    uint32_t countdownTicks;

    // Temps monotone du serveur en nanosecondes depuis le démarrage du moteur.
    uint64_t serverTimeNs;
};

#pragma pack(pop)