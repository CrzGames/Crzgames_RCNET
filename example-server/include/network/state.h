#pragma once

#include <cstdint>       // uint16_t, uint32_t, etc.
#include <mutex>         // std::mutex
#include <unordered_map> // std::unordered_map
#include <string>        // std::string

#include <rcenet/RCENET_enet.h> // ENetPeer

#include "simulation/session/client.h" // ClientSession
#include "crypto/kx.h"         // ServerCryptoKxState

struct NetworkState
{
    // ------------------------------------------------------------------------
    // Connections - ATTENTION: Thread RÉSEAU UNIQUEMENT
    // ------------------------------------------------------------------------
    
    // Incrémenté à chaque nouvelle connexion pour lui donner un ID unique (différent de l'accountIdDatabase)
    uint32_t nextConnectionId = 1; // commence à 1 pour éviter les confusions avec une valeur "0" non initialisée

    // --- mapping de connectionId vers ENetPeer* pour envoyer des messages à un client spécifique ---
    std::unordered_map<uint32_t, ENetPeer*> connectionIdToEnetPeer; // key = connectionId, value = ENetPeer*


    // ------------------------------------------------------------------------
    // Sessions - ATTENTION: Thread SIMULATION et RÉSEAU (accès protégé par mutex)
    // ------------------------------------------------------------------------

    // Mapping de connectionId vers ClientSession (sessions actives pour les clients connectés)
    std::unordered_map<uint32_t, ClientSession> sessions; // key = connectionId, value = ClientSession

    // Mutex de protection pour tous les acces a sessions
    mutable std::mutex sessionsMutex;


    // ------------------------------------------------------------------------
    // Crypto - ATTENTION: Thread Simulation UNIQUEMENT
    // ------------------------------------------------------------------------

    // Etat crypto global du serveur pour le protocole de session sécurisée (clé publique/privée, etc.)
    ServerCryptoKxState cryptoKxState;
};
