#ifndef RCNET_ENGINE_H
#define RCNET_ENGINE_H

// Standard C/C++ Libraries
#include <stdbool.h> // Required for : bool

/**
 * \brief Callbacks pour l'API du moteur RCNET.
 * 
 * Ces callbacks permettent à l'utilisateur de définir des fonctions
 * pour charger, décharger et mettre à jour le moteur de jeu.
 * 
 * \since Cette structure est disponible depuis RCNET 1.0.0.
 */
typedef struct RCNET_Callbacks {
    /**
     * \brief Fonction appelée pour décharger le moteur de jeu.
     * 
     * Cette fonction est appelée lorsque le moteur de jeu doit être arrêté.
     */
    void (*rcnet_unload)(void);

    /**
     * \brief Fonction appelée pour charger le moteur de jeu.
     * 
     * Cette fonction est appelée au démarrage du moteur de jeu.
     */
    void (*rcnet_load)(void);

    /**
     * \brief Fonction appelée pour mettre à jour le moteur de jeu.
     * 
     * Cette fonction est appelée à chaque tick du moteur de jeu.
     */
    void (*rcnet_update)(double dt);
} RCNET_Callbacks;

/**
 * \brief Fonction pour initialiser le moteur de jeu RCNET.
 * 
 * \return {bool} - true si l'initialisation réussit, false sinon
 * 
 * \threadsafety Cette fonction peut être appelée depuis n'importe quel thread.
 * 
 * \since Cette fonction est disponible depuis RCNET 1.0.0.
 */
bool rcnet_engine_run(RCNET_Callbacks* callbacks, int tickRate);

/**
 * \brief Fonction pour arrêter le moteur de jeu RCNET.
 * 
 * Cette fonction arrête le moteur de jeu en cours d'exécution.
 * 
 * \threadsafety Cette fonction peut être appelée depuis n'importe quel thread.
 * 
 * \since Cette fonction est disponible depuis RCNET 1.0.0.
 */
void rcnet_engine_eventQuit(void);

#endif // RCNET_ENGINE_H
