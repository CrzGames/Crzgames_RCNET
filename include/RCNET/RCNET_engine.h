#ifndef RCNET_ENGINE_H
#define RCNET_ENGINE_H

// Standard C/C++ Libraries
#include <stdbool.h> // bool

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Callbacks pour l'API du moteur RCNET.
 *
 * On sépare la simulation (tick fixe) et le réseau (tick fixe aussi mais différent).
 *
 * - simulation_update(dt) : logique serveur (état, gameplay, collisions simples, etc.)
 * - network_update()      : envoi snapshots/deltas, flush réseau, etc.
 *
 * \since RCNET 1.0.0 (à adapter selon ta version)
 */
typedef struct RCNET_Callbacks {
    // Appelé avant la boucle (init monde, ressources, etc.)
    void (*rcnet_load)(void);

    // Appelé après la boucle (free mémoire, fermeture, etc.)
    void (*rcnet_unload)(void);

    /**
     * \brief Tick simulation (logique serveur) à fréquence simTickRate.
     * \param dt Pas de temps FIXE (ex: 1/60, 1/30).
     */
    void (*rcnet_simulation_update)(double dt);

    /**
     * \brief Tick réseau à fréquence netTickRate.
     * Typiquement: envoyer snapshots/deltas, traiter flush, etc.
     */
    void (*rcnet_network_update)(void);
} RCNET_Callbacks;

/**
 * \brief Démarre le moteur RCNET.
 *
 * \param callbacks      Pointeur callbacks.
 * \param simTickRateHz  Fréquence simulation (ex 60, 30, 20).
 * \param netTickRateHz  Fréquence réseau (ex 20, 10, 30).
 *
 * \return true si OK, false sinon.
 */
bool rcnet_engine_run(RCNET_Callbacks* callbacks, int simTickRateHz, int netTickRateHz);

/**
 * \brief Stop le serveur (thread-safe).
 */
void rcnet_engine_eventQuit(void);

#ifdef __cplusplus
}
#endif

#endif // RCNET_ENGINE_H