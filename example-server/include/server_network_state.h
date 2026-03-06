#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <unordered_map> // std::unordered_map pour stocker les entités, sessions, etc.

#include <rcenet/RCENET_enet.h>

#include "server_sessions.h"

// channel 0 =  handshake / auth / encrypt (reliable)
// channel 1 = inputs (unreliable)
// channel 2 = snapshots (unreliable)
// channel 3 = events importants (reliable)
enum EnetChannel : uint8_t
{
    ENET_CHANNEL_HANDSHAKE_RELIABLE = 0,
    ENET_CHANNEL_INPUT_UNRELIABLE,
    ENET_CHANNEL_SNAPSHOT_UNRELIABLE,
    ENET_CHANNEL_EVENT_IMPORTANT_RELIABLE,

    ENET_CHANNEL_COUNT
};

struct NetworkState
{
    // ------------------------------------------------------------------------
    // Configuration du serveur
    // ------------------------------------------------------------------------

    // Port d'écoute du serveur
    const uint16_t serverPort = 12345;

    // Nombre de channels ENet (ex: 0=handshakes reliable, 1=inputs unrealiable, 2=snapshots unreliable, 3=events importants reliable)
    const uint8_t channelCount = ENET_CHANNEL_COUNT;

    // Nombre maximum de clients connectés
    const uint32_t maxClientsConnected = 2;

    // Fréquence de tick de simulation du serveur en Hz (ex: 128)
    uint32_t simulationTickRateHz = 128;

    // Fréquence de tick réseau OUT du serveur en Hz (ex: 32)
    uint32_t networkOutgoingTickRateHz = 32;

    // Durée de sommeil entre chaque tick réseau IN en ms (ex: 1)
    uint32_t networkIncomingSleepMs = 1;


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
};