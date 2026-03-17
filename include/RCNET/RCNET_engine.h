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
 * - rcnet_network_host_setup       : setup host réseau one-shot (encrypt/compress...)
 * - rcnet_network_outgoing_update  : envoi des données réseau sortantes
 * - rcnet_http_update              : logique de traitement HTTP (ex: pour l'API du jeu, etc.)
 * - rcnet_nats_update              : logique de traitement NATS (ex: pour la communication inter-serveurs, etc.)
 * - rcnet_wake_blocking_threads    : callback pour réveiller les threads bloqués (ex: en cas de shutdown)
 *
 * IMPORTANT :
 * - rcnet_simulation_update() tourne dans le thread simulation
 * - rcnet_network_incoming_update() tourne dans le thread réseau
 * - rcnet_network_host_setup() tourne dans le thread réseau, une seule fois
 *   juste après enet_host_create()
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
    void (*rcnet_network_host_setup)(ENetHost* host);
    void (*rcnet_network_outgoing_update)(ENetHost* host);
    void (*rcnet_http_update)(void);
    void (*rcnet_nats_update)(RCNET_NATSContext* natsContext);
    void (*rcnet_wake_blocking_threads)(void);
} RCNET_Callbacks;

// ============================================================================
// Snapshot metrics simulation (publiees chaque seconde)
// ============================================================================

/**
 * \brief Snapshot des metrics de simulation calculees sur la derniere fenetre.
 *
 * Cette structure est remplie par le thread simulation une fois par fenetre
 * de stats (actuellement 1 seconde), puis exposee en lecture via
 * rcnet_engine_getLastSimulationEtatMetrics().
 *
 * Important:
 * - Toutes les valeurs "sur_derniere_seconde" concernent la fenetre de stats
 *   precedente complete.
 * - Les valeurs en "_ms" sont exprimees en millisecondes.
 * - Les compteurs sont remis a zero a chaque nouvelle fenetre.
 */
typedef struct RCNET_SimulationEtatMetrics
{
    // Frequence cible configuree pour la simulation (ticks logiques par seconde).
    uint64_t frequence_cible_tick_simulation_hz;

    // Frequence reellement observee sur la derniere fenetre.
    // Peut differer de la cible en cas de charge ou de rattrapage.
    double frequence_reelle_tick_simulation_hz_sur_derniere_seconde;

    // Nombre de ticks simulation effectivement executes pendant la fenetre.
    uint32_t nombre_ticks_simulation_executes_sur_derniere_seconde;

    // Temps moyen de traitement d'un tick (execution du callback simulation),
    // sans inclure le retard de reveil.
    double temps_moyen_par_tick_sur_derniere_seconde_ms;

    // Percentile 95 des temps de traitement de tick calcule sur l'historique
    // glissant des deux dernieres secondes.
    double temps_en_ms_sous_lequel_se_situent_95_pourcent_des_ticks_sur_les_deux_dernieres_secondes;

    // Percentile 99 des temps de traitement de tick calcule sur l'historique
    // glissant des deux dernieres secondes.
    double temps_en_ms_sous_lequel_se_situent_99_pourcent_des_ticks_sur_les_deux_dernieres_secondes;

    // Plus grand temps de traitement observe sur un tick pendant la fenetre.
    double temps_maximum_observe_pour_un_tick_sur_derniere_seconde_ms;

    // Identifiant du tick qui a produit le maximum de traitement ci-dessus.
    uint64_t identifiant_tick_du_temps_maximum_observe_sur_derniere_seconde;

    // Retard moyen de reveil du thread simulation, calcule uniquement sur
    // les ticks qui etaient effectivement en retard.
    double retard_moyen_de_reveil_du_thread_parmi_les_ticks_en_retard_sur_derniere_seconde_ms;

    // Plus grand retard de reveil observe pendant la fenetre.
    double retard_maximum_observe_sur_derniere_seconde_ms;

    // Nombre de ticks dont le reveil thread a eu lieu apres l'horaire prevu.
    uint64_t nombre_de_reveils_du_thread_apres_l_horaire_prevu_sur_derniere_seconde;

    // Temps total reel moyen d'un tick:
    // (retard de reveil + temps de traitement), moyenne sur la fenetre.
    double temps_moyen_total_reel_du_tick_en_comptant_retard_de_reveil_plus_traitement_sur_derniere_seconde_ms;

    // Marge moyenne restante avant de deborder sur le tick suivant:
    // (budget de tick - temps total reel du tick), borne inferieure a 0.
    double marge_moyenne_restante_avant_de_deborder_sur_le_tick_suivant_sur_derniere_seconde_ms;

    // Budget maximal theorique d'un tick avant debordement sur le tick suivant.
    // Exemple: a 128 Hz, cette valeur vaut environ 7.8125 ms.
    double budget_maximal_par_tick_avant_de_deborder_sur_le_tick_suivant_ms;

    // Nombre de ticks de rattrapage executes pendant la fenetre.
    uint64_t nombre_ticks_de_rattrapage_executes_sur_derniere_seconde;

    // Nombre total de ticks de rattrapage executes depuis le lancement
    // courant du serveur (compteur cumulatif, jamais remis a zero en fenetre).
    uint64_t nombre_ticks_de_rattrapage_executes_depuis_le_lancement_du_serveur;

    // Nombre de fois ou un backlog a ete abandonne pendant la fenetre
    // pour eviter une derive de la simulation.
    uint64_t nombre_abandons_de_backlog_simulation_sur_derniere_seconde;

    // Nombre total d'abandons de backlog depuis le lancement courant
    // du serveur (compteur cumulatif, jamais remis a zero en fenetre).
    uint64_t nombre_abandons_de_backlog_simulation_depuis_le_lancement_du_serveur;
} RCNET_SimulationEtatMetrics;

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

/**
 * \brief Retourne le dernier snapshot de metrics simulation publie par le moteur.
 *
 * \param outMetrics Pointeur de sortie vers la structure a remplir.
 * \return true si un snapshot est disponible, false sinon.
 */
bool rcnet_engine_getLastSimulationEtatMetrics(RCNET_SimulationEtatMetrics* outMetrics);

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
