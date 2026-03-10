#ifndef RCNET_ENGINE_H
#define RCNET_ENGINE_H

// ============================================================================
// Standard C/C++
// ============================================================================

#include <stdbool.h> // bool
#include <stdint.h>  // uint16_t, uint32_t, uint64_t

// ============================================================================
// Dépendance RCENet (fork d'ENet)
// ============================================================================

#include <rcenet/RCENET_enet.h>

// ============================================================================
// Dépendance
// ============================================================================
#include <RCNET/RCNET_nats.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration serveur
// ============================================================================

/**
 * \brief Configuration pour la connexion NATS.
 */
typedef struct RCNET_NATSConfig
{
    const char* natsServerURL;
    bool useTLS;
    bool skipVerifyCertsServer;
    const char* publicKeyNKey;
    const char* privateKeySeedNKey;
} RCNET_NATSConfig;

/**
 * \brief Configuration runtime du serveur RCNET.
 *
 * Cette structure centralise les paramètres principaux du moteur :
 * - port                   : port UDP d'écoute (0 à 65535)
 * - maxClients             : nombre maximum de clients (4096 max pour ENet)
 * - channelCount           : nombre de channels ENet (0 à 255 max)
 * - networkIncomingPollTimeoutMs : durée max de poll réseau entrant en ms
 * - networkOutgoingTickHz  : fréquence des envois réseau sortants
 * - simulationTickHz       : fréquence de simulation serveur
 */
typedef struct RCNET_ServerConfig
{
    uint16_t port;
    uint32_t maxClients;
    uint32_t channelCount;
    uint32_t networkIncomingPollTimeoutMs;
    uint32_t networkOutgoingTickHz;
    uint32_t simulationTickHz;
    RCNET_NATSConfig natsConfig;
} RCNET_ServerConfig;

// ============================================================================
// Callbacks utilisateur
// ============================================================================

/**
 * \brief Ensemble des callbacks exposés par le moteur RCNET.
 *
 * Répartition des callbacks :
 * - rcnet_load                     : initialisation utilisateur
 * - rcnet_unload                   : nettoyage utilisateur
 * - rcnet_simulation_update        : logique de simulation
 * - rcnet_network_incoming_update  : traitement des événements réseau entrants
 * - rcnet_network_outgoing_update  : envoi des données réseau sortantes
 * - rcnet_http_update              : logique de traitement HTTP (ex: pour l'API du jeu, etc.)
 * - rcnet_nats_update              : logique de traitement NATS (ex: pour la communication inter-serveurs, etc.)
 * - rcnet_wake_blocking_threads    : callback pour réveiller les threads bloqués (ex: en cas de shutdown)
 *
 * IMPORTANT :
 * - rcnet_simulation_update() tourne dans le thread simulation
 * - rcnet_network_incoming_update() tourne dans le thread réseau
 * - rcnet_network_outgoing_update() tourne dans le thread réseau
 * - rcnet_http_update() tourne dans le thread HTTP
 * - rcnet_nats_update() tourne dans le thread NATS
 */
typedef struct RCNET_Callbacks
{
    void (*rcnet_load)(void);
    void (*rcnet_unload)(void);
    void (*rcnet_simulation_update)(uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt);
    void (*rcnet_network_incoming_update)(ENetHost* host, const ENetEvent* event);
    void (*rcnet_network_outgoing_update)(ENetHost* host);
    void (*rcnet_http_update)(void);
    void (*rcnet_nats_update)(RCNET_NATSContext* natsContext);
    void (*rcnet_wake_blocking_threads)(void);
} RCNET_Callbacks;

// ============================================================================
// API publique moteur
// ============================================================================

/**
 * \brief Lance le moteur RCNET en mode multi-thread.
 *
 * Threads démarrés :
 * - 1 thread simulation
 * - 1 thread réseau
 * - 1 thread HTTP
 * - 1 thread NATS (uniquement si la config NATS est activée)
 *
 * \param callbacks Pointeur vers les callbacks utilisateur.
 * \param config    Pointeur vers la configuration serveur.
 *
 * \return true si le moteur a démarré correctement, false sinon.
 */
bool rcnet_engine_run(RCNET_Callbacks* callbacks, const RCNET_ServerConfig* config);

/**
 * \brief Demande l’arrêt du moteur de manière thread-safe.
 */
void rcnet_engine_eventQuit(void);

// ============================================================================
// Getters thread-safe
// ============================================================================

/**
 * \brief Retourne le tick logique courant de simulation serveur.
 */
uint64_t rcnet_engine_getCurrentServerSimulationTick(void);

/**
 * \brief Retourne le temps logique monotone du serveur en nanosecondes.
 */
uint64_t rcnet_engine_getCurrentServerTimeNsMonotonic(void);

/**
 * \brief Retourne la fréquence de simulation effectivement utilisée.
 */
uint32_t rcnet_engine_getSimulationTickRateHz(void);

/**
 * \brief Retourne la fréquence réseau sortante effectivement utilisée.
 */
uint32_t rcnet_engine_getNetworkOutgoingTickRateHz(void);

/**
 * \brief Temps maximum de blocage du premier enet_host_service() côté réseau entrant.
 *
 * IMPORTANT :
 * cette valeur n'est pas un "tick fixe" ni un "sleep forcé".
 * Elle représente la durée maximale pendant laquelle le thread réseau
 * peut bloquer sur ENet avant de reprendre la main.
 *
 * Cette attente peut être réduite dynamiquement si une deadline réseau
 * sortante approche.
 */
uint32_t rcnet_engine_getNetworkIncomingPollTimeoutMs(void);

// ============================================================================
// Helpers utilitaires
// ============================================================================

/**
 * \brief Convertit une durée en millisecondes vers des ticks de simulation.
 *
 * Exemple : 1500 ms à 60 Hz = 90 ticks
 */
uint32_t rcnet_engine_durationMsToTicks(uint32_t durationMs);

/**
 * \brief Convertit des ticks de simulation vers une durée en millisecondes.
 *
 * Exemple : 90 ticks à 60 Hz = 1500 ms
 */
uint32_t rcnet_engine_ticksToDurationMs(uint32_t ticks);

/**
 * \brief Indique si le tick courant de la simulation doit produire un flux destiné au réseau sortant.
 *
 * Cette fonction sert à cadencer la production des messages que la simulation
 * pousse dans le thread réseau sortant.
 *
 * La fréquence réellement utilisée ne dépasse jamais la fréquence du thread
 * réseau sortant. Autrement dit, si `targetRateHz` est supérieur à la fréquence
 * de `network out`, la cadence est automatiquement plafonnée à celle de
 * `network out`.
 *
 * Exemple avec :
 * - simulation = 128 Hz
 * - network out = 32 Hz
 *
 * Alors :
 * - `targetRateHz = 32` déclenche à 32 Hz ;
 * - `targetRateHz = 3` déclenche à environ 3 Hz ;
 * - `targetRateHz = 60` reste plafonné à 32 Hz.
 *
 * \param currentTick Tick courant de simulation.
 * \param targetRateHz Fréquence cible souhaitée pour ce flux sortant.
 *
 * \return `true` si la simulation doit produire ce flux à ce tick,
 *         `false` sinon.
 */
bool rcnet_engine_isNetworkOutgoingProductionTick(uint64_t currentTick, uint32_t targetRateHz);

#ifdef __cplusplus
}
#endif

#endif // RCNET_ENGINE_H