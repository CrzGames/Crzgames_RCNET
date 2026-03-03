#ifndef RCNET_ENGINE_H
#define RCNET_ENGINE_H

// Standard C/C++ Libraries
#include <stdbool.h> // bool
#include <stdint.h> // uint64_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Callbacks pour l'API du moteur RCNET.
 *
 * On sépare :
 * - Simulation (tick fixe) : logique du monde.
 * - Réseau IN (tick fixe)  : réception des paquets entrants.
 * - Réseau OUT (tick fixe) : envoi des paquets sortants.
 *
 * Objectif :
 * - Réduire la latence de traitement des inputs (IN plus fréquent),
 * - Tout en gardant une cadence d’envoi contrôlée (OUT plus faible).
 *
 * \since RCNET 1.0.0
 */
typedef struct RCNET_Callbacks {
    // Appelé avant la boucle (init monde, ressources, etc.)
    void (*rcnet_load)(void);

    // Appelé après la boucle (free mémoire, fermeture, etc.)
    void (*rcnet_unload)(void);

    /**
     * \brief Tick simulation.
     * \param dt Pas de temps FIXE (ex: 1/60, 1/30).
     */
    void (*rcnet_simulation_update)(double dt);

    /**
     * \brief Tick réseau IN.
     * Traite les paquets entrants (inputs, connect/disconnect, etc.).
     */
    void (*rcnet_network_incoming_update)(void);

    /**
     * \brief Tick réseau OUT.
     * Envoie les paquets sortants (snapshots, events, etc.).
     */
    void (*rcnet_network_outgoing_update)(void);
} RCNET_Callbacks;

/**
 * \brief Démarre le moteur RCNET.
 *
 * \param callbacks                 Callbacks utilisateur.
 * \param simulationTickRateHz      Fréquence simulation (ex 128, 60, etc.).
 * \param networkIncomingTickRateHz Fréquence réseau IN (ex 256 ou 128, pour réduire la latence d’input).
 * \param networkOutgoingTickRateHz Fréquence réseau OUT (ex 32, pour snapshots).
 *
 * \return true si OK, false sinon.
 */
bool rcnet_engine_run(RCNET_Callbacks* callbacks,
                      int simulationTickRateHz,
                      int networkIncomingTickRateHz,
                      int networkOutgoingTickRateHz);

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
 * - Précis / sans dérive : on accumule des nanosecondes (pas de float, pas de cast ms).
 * - Thread-safe : peut être lu depuis d'autres threads sans verrou.
 * - Temps LOGIQUE : ne reflète pas forcément le temps réel si la machine lag
 *   (ex: backlog droppé => la simulation saute du "temps réel").
 *
 * Usage typique :
 * - Timers gameplay : buffs, cooldowns, durée de vie d'un projectile, timeouts logiques
 * - Mesures internes cohérentes avec la simulation (déterminisme)
 */
uint64_t rcnet_engine_getCurrentServerTimeNsMonotonic(void);

/**
 * \brief Getters de configuration (Hz) utilisés par le moteur.
 */
uint32_t rcnet_engine_getSimulationTickRateHz(void);
uint32_t rcnet_engine_getNetworkIncomingTickRateHz(void);
uint32_t rcnet_engine_getNetworkOutgoingTickRateHz(void);

/**
 * \brief Stop le serveur (thread-safe).
 */
void rcnet_engine_eventQuit(void);

#ifdef __cplusplus
}
#endif

#endif // RCNET_ENGINE_H