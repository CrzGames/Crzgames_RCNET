#ifndef RCNET_ENGINE_H
#define RCNET_ENGINE_H

// Standard C/C++ Libraries
#include <stdbool.h> // bool
#include <stdint.h>  // uint64_t

// ================================
// Dependencies Libraries RCENet (fork d'ENet)
// ================================
#include <rcenet/RCENET_enet.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Configuration serveur RCNET.
 *
 * Cette structure centralise tous les paramètres runtime
 * du moteur réseau et simulation.
 *
 * - port                   : Port UDP d'écoute serveur.
 * - maxClients             : Nombre max de clients connectés.
 * - channelCount           : Nombre de channels ENet.
 * - networkIncomingSleepMs : Durée de sleep (en ms) entre chaque tick de réception réseau.
 * - networkOutgoingTickHz  : Fréquence d’envoi des paquets sortants (snapshots, events, etc.).
 * - simulationTickHz       : Fréquence simulation serveur.
 */
typedef struct RCNET_ServerConfig
{
    uint16_t port;                   // ex: 12345 (plage des ports UDP : 0-65535, éviter <1024 réservés)
    uint32_t maxClients;             // ex: 64 clients max (ENet supporte jusqu'à 4096 clients max)
    uint32_t channelCount;           // ex: 3 channels (inputs unrealiable, snapshots unreliable, events importants reliable)
    uint32_t networkIncomingSleepMs; // ex: 1 ms
    uint32_t networkOutgoingTickHz;  // ex: 32 Hz
    uint32_t simulationTickHz;       // ex: 128 Hz
} RCNET_ServerConfig;

/**
 * \brief Callbacks pour l'API du moteur RCNET.
 *
 * On sépare :
 * - rcnet_load / rcnet_unload : pour l'initialisation et cleanup global du serveur (ex: world, ressources, etc.)
 * - rcnet_simulation_update : logique du monde, appliquée à chaque tick de simulation.
 * - rcnet_network_incoming_update : réception des paquets entrants.
 * - rcnet_network_outgoing_update : envoi des paquets sortants.
 *
 * IMPORTANT :
 * - rcnet_simulation_update() est exécutée dans le thread simulation.
 * - rcnet_network_incoming_update / rcnet_network_outgoing_update sont exécutés dans le thread réseau.
 */
typedef struct RCNET_Callbacks {
    void (*rcnet_load)(void);
    void (*rcnet_unload)(void);
    void (*rcnet_simulation_update)(uint64_t currentTick);
    void (*rcnet_network_incoming_update)(ENetHost* host, const ENetEvent* event);
    void (*rcnet_network_outgoing_update)(void);
} RCNET_Callbacks;

/**
 * \brief Démarre le moteur RCNET en mode multi-thread.
 *
 * - 1 thread simulation (tick fixe, logique du monde, etc.)
 * - 1 thread réseau (réception de paquets, envoi de snapshots, etc.)
 *
 * \param callbacks Callbacks utilisateur.
 * \param config    Configuration serveur complète.
 *
 * \return true si OK, false sinon.
 */
bool rcnet_engine_run(RCNET_Callbacks* callbacks, const RCNET_ServerConfig* config);

/**
 * \brief Retourne le tick courant de simulation du serveur (tick logique).
 *
 * Ce compteur est incrémenté de +1 à CHAQUE tick de simulation (rcnet_simulation_update),
 * donc il représente la "timeline gameplay" officielle du serveur.
 *
 * Propriétés :
 * - Monotone : ne recule jamais.
 * - Thread-safe : peut être lu depuis d'autres threads sans verrou.
 * - Temps LOGIQUE : il avance uniquement quand la simulation exécute un tick.
 *
 * Usage typique :
 * - Stamper un snapshot : "snapshot construit à simTick = X"
 * - ACK / reconciliation : "le serveur a validé jusqu'au tick X"
 * - Debug : comparer la progression du serveur vs client
 */
uint64_t rcnet_engine_getCurrentServerSimulationTick(void);

/**
 * \brief Retourne le temps monotone LOGIQUE du serveur en nanosecondes depuis le démarrage du moteur.
 *
 * Important : ce temps est dérivé du tick de simulation (durée fixe par tick).
 * Concrètement, à chaque tick simulation, on ajoute exactement simTickDurationNs.
 *
 * Propriétés :
 * - Monotone : ne recule jamais.
 * - Précis / sans dérive : on accumule des nanosecondes.
 * - Thread-safe : peut être lu depuis d'autres threads sans verrou.
 * - Temps LOGIQUE : ne reflète pas forcément le temps réel si la machine laggue, mais avance de manière stable avec la simulation.
 */
uint64_t rcnet_engine_getCurrentServerTimeNsMonotonic(void);

/**
 * \brief Retourne la fréquence de tick de simulation du serveur en Hz (ex: 128).
 */
uint32_t rcnet_engine_getSimulationTickRateHz(void);

/**
 * \brief Retourne la fréquence de tick réseau OUT du serveur en Hz (ex: 32).
 */
uint32_t rcnet_engine_getNetworkOutgoingTickRateHz(void);

/**
 * \brief Retourne la durée de sommeil entre chaque tick réseau IN en ms (ex: 1).
 */
uint32_t rcnet_engine_getNetworkIncomingSleepMs(void);

/**
 * \brief Convertit une durée en millisecondes vers des ticks de simulation.
 *
 * Exemple : 1500 ms à 60 Hz = 90 ticks
 */
inline uint32_t DurationMsToTicks(uint32_t durationMs)
{
    const uint64_t hz = (uint64_t)rcnet_engine_getSimulationTickRateHz();
    return (uint32_t)(((uint64_t)durationMs * hz + 999ull) / 1000ull); // ceil
}

/**
 * \brief Convertit une durée en ticks de simulation vers des millisecondes.
 *
 * Exemple : 90 ticks à 60 Hz = 1500 ms
 */
inline uint32_t TicksToDurationMs(uint32_t ticks)
{
    const uint64_t hz = (uint64_t)rcnet_engine_getSimulationTickRateHz();
    return (uint32_t)(((uint64_t)ticks * 1000ull) / hz); // floor
}

/**
 * \brief Stop le serveur (thread-safe).
 */
void rcnet_engine_eventQuit(void);

#ifdef __cplusplus
}
#endif

#endif // RCNET_ENGINE_H