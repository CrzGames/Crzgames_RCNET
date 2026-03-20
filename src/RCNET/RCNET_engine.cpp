#include "RCNET/RCNET.h"

// ============================================================================
// Standard C/C++
// ============================================================================

#include <stdbool.h>  // bool
#include <cstdlib>    // std::size_t, utilitaires généraux C/C++
#include <chrono>     // horloges et durées
#include <thread>     // std::thread, std::this_thread::sleep_for
#include <atomic>     // std::atomic
#include <algorithm>  // std::sort
#include <vector>
#include <mutex>

using namespace std::chrono;

// ============================================================================
// Dépendances OpenSSL
// ============================================================================

#include <openssl/ssl.h> // OPENSSL_init_ssl, SSL_COMP_free_compression_methods
#include <openssl/bio.h> // BIO (inclus ici comme dans ton code d'origine)
#include <openssl/err.h> // ERR_error_string, ERR_get_error

// ============================================================================
// Dépendance libsodium
// ============================================================================

#include <sodium.h> // sodium_init

// ============================================================================
// Dépendances Agones
// ============================================================================
#include <agones/sdk.h>

// ============================================================================
// Dépendances système pour les threads et l'affinité CPU
// ============================================================================
#if defined(__linux__)
    #include <pthread.h>
    #include <sched.h>
    #include <string.h>
    #include <errno.h>
    #include <time.h>
#endif

// ============================================================================
// 1) État global exposé thread-safe
// ============================================================================

// Tick logique courant du serveur.
// Il est incrémenté à chaque tick simulation.
static std::atomic<uint64_t> g_serverSimulationTick{0};

// Temps logique monotone du serveur en nanosecondes.
// Il avance à chaque tick simulation en ajoutant exactement dtNs.
static std::atomic<uint64_t> g_serverTimeNsMonotonic{0};

// Fréquence de simulation effectivement appliquée par le moteur.
static std::atomic<uint32_t> g_simulationTickRateHz{128};

// Durée de poll réseau entrant effectivement utilisée.
static std::atomic<uint32_t> g_networkIncomingPollTimeoutMs{1};

// Fréquence réseau sortante effectivement utilisée.
static std::atomic<uint32_t> g_networkOutgoingTickRateHz{32};

// ============================================================================
// 2) État global interne moteur
// ============================================================================

// Indique si le serveur doit continuer à tourner.
// Cette variable est lue par plusieurs threads.
static std::atomic<bool> serverIsRunning{true};

// Port serveur utilisé pour le bind ENet.
static uint16_t g_serverPort = 0;

// Nombre maximum de clients autorisés par le host ENet.
static uint32_t g_serverMaxClients = 0;

// Nombre de channels ENet utilisés.
static uint32_t g_serverChannelCount = 0;

// Host ENet global.
// IMPORTANT : il n'est manipulé que dans le thread réseau.
static ENetHost* g_enetServerHost = nullptr;

// Client NATS global.
static RCNET_NATSContext g_natsClient = {0};
static bool g_natsEnabled = false;
static const char* g_natsServerURL = nullptr;
static const char* g_natsPublicKeyNKey = nullptr;
static const char* g_natsPrivateKeySeedNKey = nullptr;
static bool g_natsSkipVerifyCertsServer = false;
static bool g_natsUseTLS = false;

// ============================================================================
// 3) Paramètres runtime simulation / réseau
// ============================================================================

// Fréquence simulation en Hz.
// Exemple : 128 signifie 128 ticks de simulation par seconde.
static int simulationTickRateHz = 128;

// Durée d'attente max côté réception réseau, en millisecondes.
static uint32_t networkIncomingPollTimeoutMs = 1;

// Fréquence réseau sortante en Hz.
// Exemple : 32 signifie 32 ticks d'envoi par seconde.
static int networkOutgoingTickRateHz = 32;

// Fréquence réseau sortante effectivement retenue sous forme uint64.
static uint64_t g_netOutTickHz = 32;

// Partie entière de la durée d'un tick réseau sortant en ns.
static uint64_t g_netOutTickBaseNs = 0;

// Reste de division de 1 seconde par la fréquence réseau sortante.
static uint64_t g_netOutTickRem = 0;

// Accumulateur du reste pour distribuer proprement les ns résiduels.
static uint64_t g_netOutTickRemAcc = 0;

// ============================================================================
// 4) Paramètres de temps exact simulation (sans drift)
// ============================================================================

// Fréquence simulation effectivement retenue sous forme uint64.
static uint64_t g_simTickHz = 128;

// Partie entière de la durée d'un tick simulation en ns.
static uint64_t g_simTickBaseNs = 0;

// Reste de division de 1 seconde par la fréquence simulation.
static uint64_t g_simTickRem = 0;

// Accumulateur du reste pour distribuer proprement les ns résiduels.
static uint64_t g_simTickRemAcc = 0;

// ============================================================================
// 5) Compteurs internes de debug / diagnostic
// ============================================================================

// Identifiant interne du tick simulation.
static uint64_t simulationTickId = 0;

// Identifiant interne du tick réseau entrant.
static uint64_t networkIncomingTickId = 0;

// Identifiant interne du tick réseau sortant.
static uint64_t networkOutgoingTickId = 0;

// Identifiant interne du tick HTTP.
static uint64_t httpTickId = 0;

// Identifiant interne du tick NATS.
static uint64_t natsTickId = 0;

// ============================================================================
// 5-bis) Statistiques runtime simulation
// ============================================================================

// Dernier temps réel d'exécution d'un tick simulation.
static std::atomic<uint64_t> g_simLastExecNs{0};

// Temps max d'exécution d'un tick sur la fenêtre courante.
static std::atomic<uint64_t> g_simMaxExecNs{0};

// Dernier retard observé avant exécution d'un tick.
static std::atomic<uint64_t> g_simLastLatenessNs{0};

// Retard max observé sur la fenêtre courante.
static std::atomic<uint64_t> g_simMaxLatenessNs{0};

// Nombre de ticks exécutés en retard sur la fenêtre courante.
static std::atomic<uint64_t> g_simLateTickCount{0};

// Nombre de ticks de catch-up exécutés sur la fenêtre courante.
static std::atomic<uint64_t> g_simCatchUpTickCount{0};
static std::atomic<uint64_t> g_simCatchUpTickCountSinceStartup{0};

// Nombre de drops backlog simulation sur la fenêtre courante.
static std::atomic<uint64_t> g_simBacklogDropCount{0};
static std::atomic<uint64_t> g_simBacklogDropCountSinceStartup{0};

// Fréquence réelle observée sur la dernière fenêtre d'une seconde.
static std::atomic<uint32_t> g_simRealTickRateHz{0};

// Dernier snapshot expose de metrics simulation.
static RCNET_SimulationEtatMetrics g_lastSimulationEtatMetrics = {};

// Mutex de protection du snapshot de metrics simulation.
static std::mutex g_lastSimulationEtatMetricsMutex;

// Indique si au moins un snapshot valide a deja ete publie.
static std::atomic<bool> g_hasLastSimulationEtatMetrics{false};

// ============================================================================
// 6) Paramètres de robustesse des boucles
// ============================================================================

// Nombre maximum de ticks de rattrapage consécutifs.
// Sert à éviter la spirale de la mort si le thread prend trop de retard.
static constexpr uint32_t kMaxCatchUpTicks = 5;

// Marge finale avant deadline OUT pendant laquelle on évite
// de consommer trop de travail côté réseau entrant.
static constexpr uint64_t kFinalOutMarginNs = 200'000ull; // 200 µs

// Budget maximum accordé au traitement réseau entrant
// pendant une itération de la boucle réseau.
static constexpr uint64_t kMaxIncomingWorkBudgetNs = 500'000ull; // 500 µs

// ============================================================================
// 7) Callbacks utilisateur enregistrés
// ============================================================================

// Structure globale qui contient les callbacks utilisateur retenus par le moteur.
static RCNET_Callbacks callbacksServerEngine = {};

// ============================================================================
// 7-bis) Priorités Linux fixes
// ============================================================================

// Priorité Linux RT du thread simulation.
// Valeur volontairement élevée, mais non maximale.
static constexpr int kLinuxSimulationThreadPriority = 80;

// Priorité Linux RT du thread réseau.
// Valeur inférieure à celle du thread simulation.
static constexpr int kLinuxNetworkThreadPriority = 70;

// ============================================================================
static bool rcnet_engine_setCurrentThreadPrioritySimulationLinux(void)
{
#if defined(__linux__)
    const int policy = SCHED_FIFO;

    sched_param param = {};
    const int minPriority = sched_get_priority_min(policy);
    const int maxPriority = sched_get_priority_max(policy);

    if (minPriority == -1 || maxPriority == -1)
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "Impossible de recuperer la plage de priorite Linux pour le thread simulation."
        );
        return false;
    }

    // Clamp de la priorité voulue dans la plage autorisée par Linux.
    param.sched_priority = kLinuxSimulationThreadPriority;

    if (param.sched_priority < minPriority)
        param.sched_priority = minPriority;

    if (param.sched_priority > maxPriority)
        param.sched_priority = maxPriority;

    const int result = pthread_setschedparam(pthread_self(), policy, &param);
    if (result != 0)
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "Impossible de definir la priorite Linux du thread simulation: %s",
            strerror(result)
        );
        return false;
    }

    RCNET_log(
        RCNET_LOG_INFO,
        "Priorite Linux du thread simulation configuree avec succes (policy=SCHED_FIFO, priority=%d).",
        param.sched_priority
    );
    return true;
#else
    return true;
#endif
}

static bool rcnet_engine_setCurrentThreadPriorityNetworkLinux(void)
{
#if defined(__linux__)
    const int policy = SCHED_FIFO;

    sched_param param = {};
    const int minPriority = sched_get_priority_min(policy);
    const int maxPriority = sched_get_priority_max(policy);

    if (minPriority == -1 || maxPriority == -1)
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "Impossible de recuperer la plage de priorite Linux pour le thread reseau."
        );
        return false;
    }

    // Clamp de la priorité voulue dans la plage autorisée par Linux.
    param.sched_priority = kLinuxNetworkThreadPriority;

    if (param.sched_priority < minPriority)
        param.sched_priority = minPriority;

    if (param.sched_priority > maxPriority)
        param.sched_priority = maxPriority;

    const int result = pthread_setschedparam(pthread_self(), policy, &param);
    if (result != 0)
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "Impossible de definir la priorite Linux du thread reseau: %s",
            strerror(result)
        );
        return false;
    }

    RCNET_log(
        RCNET_LOG_INFO,
        "Priorite Linux du thread reseau configuree avec succes (policy=SCHED_FIFO, priority=%d).",
        param.sched_priority
    );
    return true;
#else
    return true;
#endif
}

// ============================================================================
// 8) Helpers temps
// ============================================================================

/**
 * \brief Retourne l'index "nearest-rank" d'un percentile sur un tableau trie.
 *
 * Exemple :
 * - percentile=95 sur 100 elements -> index 94
 * - percentile=99 sur 100 elements -> index 98
 */
static inline size_t rcnet_engine_percentileNearestRankIndex(size_t count, size_t percentile)
{
    if (count == 0)
        return 0;

    // Rang = ceil(percentile * count / 100)
    const size_t rank = (percentile * count + 99) / 100;

    // Conversion rang (1..N) -> index (0..N-1)
    return (rank > 0) ? (rank - 1) : 0;
}

/**
 * \brief Consomme exactement un pas de simulation en ns sans dérive.
 *
 * La logique :
 * - stepNs démarre avec la base entière
 * - on accumule le reste
 * - quand l'accumulateur dépasse la fréquence, on ajoute 1 ns
 *
 * Cela permet de représenter exactement 1 seconde répartie sur N ticks.
 */
static inline uint64_t rcnet_engine_consumeSimulationStepNs(void)
{
    // On part de la partie entière de la durée du tick.
    uint64_t stepNs = g_simTickBaseNs;

    // On accumule le reste de division.
    g_simTickRemAcc += g_simTickRem;

    // Si on a accumulé assez de reste pour "gagner" 1 ns supplémentaire,
    // on le redistribue maintenant.
    if (g_simTickRemAcc >= g_simTickHz)
    {
        // On retire une "unité de fréquence" de l'accumulateur.
        g_simTickRemAcc -= g_simTickHz;

        // On ajoute 1 ns au tick courant.
        stepNs += 1;
    }

    // On retourne la durée exacte du tick courant.
    return stepNs;
}

/**
 * \brief Consomme exactement un pas de tick réseau sortant en ns sans dérive.
 *
 * La logique est strictement la même que pour la simulation :
 * - stepNs démarre avec la base entière
 * - on accumule le reste
 * - quand l'accumulateur dépasse la fréquence, on ajoute 1 ns
 *
 * Cela permet de représenter exactement 1 seconde répartie sur N ticks OUT.
 */
static inline uint64_t rcnet_engine_consumeNetworkOutgoingStepNs(void)
{
    // On part de la partie entière de la durée du tick réseau sortant.
    uint64_t stepNs = g_netOutTickBaseNs;

    // On accumule le reste de division.
    g_netOutTickRemAcc += g_netOutTickRem;

    // Si on a accumulé assez de reste pour "gagner" 1 ns supplémentaire,
    // on le redistribue maintenant.
    if (g_netOutTickRemAcc >= g_netOutTickHz)
    {
        // On retire une "unité de fréquence" de l'accumulateur.
        g_netOutTickRemAcc -= g_netOutTickHz;

        // On ajoute 1 ns au tick courant.
        stepNs += 1;
    }

    // On retourne la durée exacte du tick courant.
    return stepNs;
}

/**
 * \brief Retourne le temps monotone courant en nanosecondes.
 *
 * Utilise steady_clock pour garantir que l'horloge ne recule jamais.
 */
static uint64_t rcnet_engine_getCurrentTimeNs(void)
{
#if defined(__linux__)
    timespec ts{};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        // Fallback de secours si clock_gettime echoue.
        auto now = steady_clock::now();
        return static_cast<uint64_t>(
            duration_cast<nanoseconds>(now.time_since_epoch()).count()
        );
    }

    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ull
         + static_cast<uint64_t>(ts.tv_nsec);
#else
    // Lit l'heure monotone courante.
    auto now = steady_clock::now();

    // Convertit le temps écoulé depuis l'epoch de steady_clock en nanosecondes.
    return static_cast<uint64_t>(duration_cast<nanoseconds>(now.time_since_epoch()).count());
#endif
}

/**
 * \brief Attend jusqu'à une date cible en nanosecondes.
 *
 * Stratégie :
 * - sleep sur le gros du temps restant
 * - spin sur la fin pour limiter l'oversleep
 */
static inline void rcnet_sleep_until_ns(uint64_t targetTimeNs)
{
    // Marge finale dans laquelle on ne dort plus et on finit en spin.
    constexpr uint64_t kSpinMarginNs = 200'000ull; // 200 µs

    while (true)
    {
        const uint64_t now = rcnet_engine_getCurrentTimeNs();

        if (now >= targetTimeNs)
            return;

        const uint64_t remaining = targetTimeNs - now;

#if defined(__linux__)
        // Tant qu'on est en dehors de la marge finale,
        // on utilise un sleep absolu sur CLOCK_MONOTONIC.
        if (remaining > kSpinMarginNs)
        {
            const uint64_t sleepUntilNs = targetTimeNs - kSpinMarginNs;

            timespec ts{};
            ts.tv_sec  = static_cast<time_t>(sleepUntilNs / 1'000'000'000ull);
            ts.tv_nsec = static_cast<long>(sleepUntilNs % 1'000'000'000ull);

            const int rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, nullptr);

            if (rc == 0)
            {
                // On revient dans la boucle pour finir proprement la marge finale.
                continue;
            }

            if (rc == EINTR)
            {
                // Signal recu : on recalcule simplement.
                continue;
            }

            // En cas d'erreur inattendue, fallback sur le comportement portable actuel.
            std::this_thread::sleep_for(std::chrono::nanoseconds(remaining - kSpinMarginNs));
            continue;
        }
        else
        {
            // Marge finale: comportement identique a ton code actuel.
            std::this_thread::yield();
        }
#else
        // Comportement portable conserve tel quel.
        if (remaining > kSpinMarginNs)
        {
            std::this_thread::sleep_for(std::chrono::nanoseconds(remaining - kSpinMarginNs));
        }
        else
        {
            std::this_thread::yield();
        }
#endif
    }
}

// ============================================================================
// 9) Initialisation / nettoyage des dépendances
// ============================================================================

/**
 * \brief Initialise RCENet.
 */
static bool rcnet_engine_initRCENet(void)
{
    // Initialise la librairie ENet/RCENet.
    if (enet_initialize() < 0)
    {
        // Log erreur si l'initialisation échoue.
        RCNET_log(RCNET_LOG_CRITICAL, "Erreur lors de l initialisation de RCEnet.");
        return false;
    }

    // Log succès.
    RCNET_log(RCNET_LOG_INFO, "RCENet initialiser avec succes.");
    return true;
}

/**
 * \brief Nettoie RCENet.
 */
static void rcnet_engine_cleanupRCENet(void)
{
    // Libère l'état global ENet.
    enet_deinitialize();

    // Log succès.
    RCNET_log(RCNET_LOG_INFO, "RCENet nettoyer avec succes.");
}

/**
 * \brief Initialise OpenSSL.
 */
static bool rcnet_engine_initOpenssl(void)
{
    // Initialise OpenSSL
    if (OPENSSL_init_ssl(0, nullptr) != 1)
    {
        unsigned long err = ERR_get_error();
        RCNET_log(RCNET_LOG_ERROR,
                  "Erreur lors de l initialisation d OpenSSL%s%s",
                  err ? " : " : "",
                  err ? ERR_error_string(err, nullptr) : "");
        return false;
    }

    // Log succès.
    RCNET_log(RCNET_LOG_INFO, "OpenSSL initialiser avec succes.");
    return true;
}

/**
 * \brief Initialise libsodium.
 */
static bool rcnet_engine_initLibSodium(void)
{
    // Initialise libsodium.
    if (sodium_init() < 0)
    {
        // Log erreur en cas d'échec.
        RCNET_log(RCNET_LOG_ERROR, "Erreur lors de l initialisation de libsodium.");
        return false;
    }

    // Log succès.
    RCNET_log(RCNET_LOG_INFO, "libsodium initialiser avec succes.");
    return true;
}

// ============================================================================
// 10) Configuration callbacks
// ============================================================================

/**
 * \brief Copie les callbacks utilisateur fournis dans la structure globale.
 *
 * IMPORTANT :
 * - on ne remplace un callback que si le pointeur fourni est non null
 * - cela conserve le comportement exact de ton code
 */
static void rcnet_engine_setCallbacks(RCNET_Callbacks* callbacksUser)
{
    // Si le callback load existe, on l'enregistre.
    if (callbacksUser->rcnet_load)
        callbacksServerEngine.rcnet_load = callbacksUser->rcnet_load;

    // Si le callback unload existe, on l'enregistre.
    if (callbacksUser->rcnet_unload)
        callbacksServerEngine.rcnet_unload = callbacksUser->rcnet_unload;

    // Si le callback simulation existe, on l'enregistre.
    if (callbacksUser->rcnet_simulation_update)
        callbacksServerEngine.rcnet_simulation_update = callbacksUser->rcnet_simulation_update;

    // Si le callback réseau entrant existe, on l'enregistre.
    if (callbacksUser->rcnet_network_incoming_update)
        callbacksServerEngine.rcnet_network_incoming_update = callbacksUser->rcnet_network_incoming_update;

    // Si le callback setup host réseau existe, on l'enregistre.
    if (callbacksUser->rcnet_network_host_setup)
        callbacksServerEngine.rcnet_network_host_setup = callbacksUser->rcnet_network_host_setup;

    // Si le callback réseau sortant existe, on l'enregistre.
    if (callbacksUser->rcnet_network_outgoing_update)
        callbacksServerEngine.rcnet_network_outgoing_update = callbacksUser->rcnet_network_outgoing_update;

    // Si le callback HTTP existe, on l'enregistre.
    if (callbacksUser->rcnet_http_update)
        callbacksServerEngine.rcnet_http_update = callbacksUser->rcnet_http_update;

    // Si le callback NATS existe, on l'enregistre.
    if (callbacksUser->rcnet_nats_update)
        callbacksServerEngine.rcnet_nats_update = callbacksUser->rcnet_nats_update;

    // Si le callback de réveil des threads bloqués existe, on l'enregistre.
    if (callbacksUser->rcnet_wake_blocking_threads)
        callbacksServerEngine.rcnet_wake_blocking_threads = callbacksUser->rcnet_wake_blocking_threads;
}

// ============================================================================
// 11) Initialisation / arrêt moteur
// ============================================================================

/**
 * \brief Initialise le moteur et prépare les durées de ticks.
 */
static bool rcnet_engine_init(void)
{
    // Initialise OpenSSL.
    if (!rcnet_engine_initOpenssl())
        return false;

    // Initialise RCENet.
    if (!rcnet_engine_initRCENet())
        return false;

    // Initialise libsodium.
    if (!rcnet_engine_initLibSodium())
        return false;

    // Calcule la fréquence simulation effectivement utilisée.
    uint64_t Hz = (simulationTickRateHz > 0) ? (uint64_t)simulationTickRateHz : 128ull;

    // Stocke la fréquence simulation.
    g_simTickHz = Hz;

    // Calcule la partie entière de la durée d'un tick simulation.
    g_simTickBaseNs = 1'000'000'000ull / Hz;

    // Calcule le reste de division pour la compensation sans drift.
    g_simTickRem = 1'000'000'000ull % Hz;

    // Reset de l'accumulateur du reste.
    g_simTickRemAcc = 0;

    // Calcule la fréquence réseau sortante effectivement utilisée.
    uint64_t outHz = (networkOutgoingTickRateHz > 0) ? (uint64_t)networkOutgoingTickRateHz : 32ull;

    // Stocke la fréquence réseau sortante.
    g_netOutTickHz = outHz;

    // Calcule la partie entière de la durée d'un tick OUT.
    g_netOutTickBaseNs = 1'000'000'000ull / outHz;

    // Calcule le reste de division pour la compensation sans drift.
    g_netOutTickRem = 1'000'000'000ull % outHz;

    // Reset de l'accumulateur du reste.
    g_netOutTickRemAcc = 0;

    // Expose la fréquence simulation utilisée.
    g_simulationTickRateHz.store((uint32_t)g_simTickHz, std::memory_order_relaxed);

    // Expose la fréquence réseau sortante utilisée.
    g_networkOutgoingTickRateHz.store((uint32_t)outHz, std::memory_order_relaxed);

    // Expose le timeout de poll réseau entrant utilisé.
    g_networkIncomingPollTimeoutMs.store(networkIncomingPollTimeoutMs, std::memory_order_relaxed);

    // Log de la configuration effective du moteur.
    RCNET_log(RCNET_LOG_INFO, "Simulation tick rate: %u Hz", rcnet_engine_getSimulationTickRateHz());
    RCNET_log(RCNET_LOG_INFO, "Network outgoing tick rate: %u Hz", rcnet_engine_getNetworkOutgoingTickRateHz());
    RCNET_log(RCNET_LOG_INFO, "Network incoming poll timeout: %u ms", rcnet_engine_getNetworkIncomingPollTimeoutMs());

    // Init OK.
    return true;
}

/**
 * \brief Nettoie le moteur.
 *
 * NOTE : on conserve volontairement exactement ton ordre :
 * - cleanup RCENet
 */
static void rcnet_engine_quit(void)
{
    // Nettoie RCENet.
    rcnet_engine_cleanupRCENet();
}

// ============================================================================
// 12) Ticks logiques internes
// ============================================================================

/**
 * \brief Exécute un tick de simulation.
 */
static inline void rcnet_engine_simulationUpdate(uint64_t dtNs)
{
    // Incrémente l'identifiant interne du tick simulation.
    simulationTickId++;

    // Publie le tick logique courant de manière thread-safe.
    g_serverSimulationTick.store(simulationTickId, std::memory_order_relaxed);

    // Constante de conversion nanosecondes -> secondes.
    constexpr double kNsToSec = 1.0 / 1'000'000'000.0;

    // Convertit dtNs en secondes double pour le callback utilisateur.
    double dt = (double)dtNs * kNsToSec;

    // Ajoute dtNs au temps logique serveur de manière atomique
    // et récupère l'ancienne valeur.
    uint64_t prev = g_serverTimeNsMonotonic.fetch_add(dtNs, std::memory_order_relaxed);

    // Calcule le temps logique courant après incrément.
    uint64_t serverTimeNs = prev + dtNs;

    // Si le callback utilisateur existe, on l'appelle.
    if (callbacksServerEngine.rcnet_simulation_update)
        callbacksServerEngine.rcnet_simulation_update(simulationTickId, serverTimeNs, dtNs, dt);
}

/**
 * \brief Exécute un tick réseau entrant.
 */
static inline void rcnet_engine_networkIncomingUpdate(ENetHost* host, const ENetEvent* event)
{
    // Incrémente le compteur interne de tick réseau entrant.
    networkIncomingTickId++;

    // Si le callback utilisateur existe, on lui transmet l'événement.
    if (callbacksServerEngine.rcnet_network_incoming_update != nullptr)
    {
        callbacksServerEngine.rcnet_network_incoming_update(host, event);
    }
}

/**
 * \brief Exécute le setup host réseau one-shot.
 */
static inline void rcnet_engine_networkHostSetup(ENetHost* host)
{
    if (callbacksServerEngine.rcnet_network_host_setup != nullptr)
        callbacksServerEngine.rcnet_network_host_setup(host);
}

/**
 * \brief Exécute un tick réseau sortant.
 */
static inline void rcnet_engine_networkOutgoingUpdate(ENetHost* host)
{
    // Incrémente le compteur interne de tick réseau sortant.
    networkOutgoingTickId++;

    // Si le callback utilisateur existe, on l'appelle.
    if (callbacksServerEngine.rcnet_network_outgoing_update != nullptr)
        callbacksServerEngine.rcnet_network_outgoing_update(host);
}

/**
 * \brief Exécute un tick HTTP.
 */
static inline void rcnet_engine_httpUpdate(void)
{
    // Incrémente le compteur interne de tick HTTP.
    httpTickId++;

    // Si le callback utilisateur existe, on l'appelle.
    if (callbacksServerEngine.rcnet_http_update != nullptr)
        callbacksServerEngine.rcnet_http_update();
}

/**
 * \brief Exécute un tick NATS.
 */
static inline void rcnet_engine_natsUpdate(RCNET_NATSContext* natsContext)
{
    // Incrémente le compteur interne de tick NATS.
    natsTickId++;

    // Si le callback utilisateur existe, on l'appelle.
    if (callbacksServerEngine.rcnet_nats_update != nullptr)
        callbacksServerEngine.rcnet_nats_update(natsContext);
}

// ============================================================================
// 13) Getters publics thread-safe
// ============================================================================

uint64_t rcnet_engine_getCurrentServerSimulationTick(void)
{
    // Retourne le tick simulation courant.
    return g_serverSimulationTick.load(std::memory_order_relaxed);
}

uint64_t rcnet_engine_getCurrentServerTimeNsMonotonic(void)
{
    // Retourne le temps logique monotone courant.
    return g_serverTimeNsMonotonic.load(std::memory_order_relaxed);
}

uint32_t rcnet_engine_getSimulationTickRateHz(void)
{
    // Retourne la fréquence de simulation utilisée.
    return g_simulationTickRateHz.load(std::memory_order_relaxed);
}

uint32_t rcnet_engine_getNetworkIncomingPollTimeoutMs(void)
{
    // Retourne la durée de timeout de service réseau entrant utilisée.
    return g_networkIncomingPollTimeoutMs.load(std::memory_order_relaxed);
}

bool rcnet_engine_getLastSimulationEtatMetrics(RCNET_SimulationEtatMetrics* outMetrics)
{
    if (outMetrics == nullptr)
    {
        return false;
    }

    if (!g_hasLastSimulationEtatMetrics.load(std::memory_order_relaxed))
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_lastSimulationEtatMetricsMutex);
    *outMetrics = g_lastSimulationEtatMetrics;
    return true;
}

uint32_t rcnet_engine_getNetworkOutgoingTickRateHz(void)
{
    // Retourne la fréquence réseau sortante utilisée.
    return g_networkOutgoingTickRateHz.load(std::memory_order_relaxed);
}

// ============================================================================
// 14) Helpers publics déplacés du header vers le cpp
// ============================================================================

uint32_t rcnet_engine_durationMsToTicks(uint32_t durationMs)
{
    // Récupère la fréquence simulation actuellement utilisée.
    const uint64_t hz = (uint64_t)rcnet_engine_getSimulationTickRateHz();

    // Convertit des millisecondes en ticks avec arrondi plafond.
    return (uint32_t)(((uint64_t)durationMs * hz + 999ull) / 1000ull);
}

uint32_t rcnet_engine_ticksToDurationMs(uint32_t ticks)
{
    // Récupère la fréquence simulation actuellement utilisée.
    const uint64_t hz = (uint64_t)rcnet_engine_getSimulationTickRateHz();

    // Convertit des ticks en millisecondes avec arrondi inférieur.
    return (uint32_t)(((uint64_t)ticks * 1000ull) / hz);
}

bool rcnet_engine_isNetworkOutgoingProductionTick(uint64_t currentTick, uint32_t targetRateHz)
{
    // Récupérer la fréquence de simulation.
    const uint32_t simulationRateHz = rcnet_engine_getSimulationTickRateHz();

    // Récupérer la fréquence du thread réseau sortant.
    const uint32_t networkOutgoingRateHz = rcnet_engine_getNetworkOutgoingTickRateHz();

    // Si la fréquence cible est invalide, ne jamais déclencher.
    if (targetRateHz == 0)
    {
        return false;
    }

    // Si une des fréquences système est invalide, ne jamais déclencher.
    if (simulationRateHz == 0 || networkOutgoingRateHz == 0)
    {
        return false;
    }

    // Ne jamais produire plus vite que le thread réseau sortant.
    const uint32_t effectiveRateHz =
        (targetRateHz < networkOutgoingRateHz)
            ? targetRateHz
            : networkOutgoingRateHz;

    // Si la fréquence effective atteint ou dépasse la fréquence de simulation,
    // alors on peut produire à chaque tick.
    if (effectiveRateHz >= simulationRateHz)
    {
        return true;
    }

    // Le tick 0 peut être considéré comme un tick de production.
    if (currentTick == 0)
    {
        return true;
    }

    // Utiliser une comparaison par "bucket temporel franchi"
    // pour mieux répartir les fréquences non divisibles exactement.
    const uint64_t previousBucket = ((currentTick - 1) * effectiveRateHz) / simulationRateHz;
    const uint64_t currentBucket  = (currentTick * effectiveRateHz) / simulationRateHz;

    // Déclencher uniquement lorsqu'on change de bucket.
    return currentBucket != previousBucket;
}

// ============================================================================
// 15) Signal d'arrêt public
// ============================================================================

void rcnet_engine_eventQuit(void)
{
    // Demande l'arrêt global de manière thread-safe.
    serverIsRunning.store(false, std::memory_order_relaxed);

    if (callbacksServerEngine.rcnet_wake_blocking_threads != nullptr)
    {
        callbacksServerEngine.rcnet_wake_blocking_threads();
    }
}

// ============================================================================
// 16) Thread HTTP
// ============================================================================

/**
 * \brief Point d'entrée du thread HTTP.
 */
static void rcnet_engine_httpThreadMain(void)
{
    // Log de démarrage du thread HTTP.
    RCNET_log(RCNET_LOG_INFO, "Thread HTTP demarrer.");

    // Tant que le serveur tourne.
    while (serverIsRunning.load(std::memory_order_relaxed))
    {
        // Exécute un tick HTTP.
        rcnet_engine_httpUpdate();
    }

    // Log de fin du thread HTTP.
    RCNET_log(RCNET_LOG_INFO, "Thread HTTP terminer.");
}

// ============================================================================
// 17) Thread NATS
// ============================================================================

/**
 * \brief Point d'entrée du thread NATS.
 */
static void rcnet_engine_natsThreadMain(void)
{
    // Log de démarrage du thread NATS.
    RCNET_log(RCNET_LOG_INFO, "Thread NATS demarrer.");

    bool natsReady = false;

    // Initialise le natsContext NATS si activé.
    if (g_natsEnabled)
    {
        // Note : on passe les paramètres de connexion NATS via des variables globales
        if (!rcnet_nats_initialize(
                &g_natsClient,
                g_natsServerURL,
                g_natsUseTLS,
                g_natsSkipVerifyCertsServer,
                g_natsPublicKeyNKey,
                g_natsPrivateKeySeedNKey))
        {
            RCNET_log(RCNET_LOG_ERROR, "Failed to initialize NATS client.");
            rcnet_engine_eventQuit();
        }
        else
        {
            natsReady = true;
        }
    }

    // Tant que le serveur tourne.
    while (serverIsRunning.load(std::memory_order_relaxed))
    {
        if (natsReady)
        {
            // Exécute un tick NATS.
            rcnet_engine_natsUpdate(&g_natsClient);
        }
    }

    if (natsReady)
    {
        rcnet_nats_cleanup(&g_natsClient);
    }

    // Log de fin du thread NATS.
    RCNET_log(RCNET_LOG_INFO, "Thread NATS terminer.");
}

// ============================================================================
// 17) Thread simulation
// ============================================================================

static void rcnet_engine_simulationThreadMain(void)
{
    rcnet_engine_setCurrentThreadPrioritySimulationLinux();

    RCNET_log(RCNET_LOG_INFO, "Thread simulation demarrer.");

    // ------------------------------------------------------------------------
    // A) Constantes de budget / seuils de diagnostic
    // ------------------------------------------------------------------------

    // Budget theorique d'un tick simulation a la frequence courante.
    const uint64_t simBudgetNs = 1'000'000'000ull / g_simTickHz;

    // Seuil warning "tick lent" sur le temps de traitement pur : 75% du budget.
    const uint64_t warnBudgetNs = (simBudgetNs * 75ull) / 100ull;

    // Fenetre de stats : 1 seconde reelle.
    constexpr uint64_t kStatsWindowNs = 1'000'000'000ull;

    // Historique des temps de traitement de tick sur les 2 dernieres secondes.
    const size_t tickExecHistoryCapacity =
        (size_t)((g_simTickHz > 0) ? (g_simTickHz * 2ull) : 256ull);

    // ------------------------------------------------------------------------
    // B) Initialisation de l'horloge de simulation
    // ------------------------------------------------------------------------

    uint64_t nowNs = rcnet_engine_getCurrentTimeNs();
    uint64_t nextStepNs = rcnet_engine_consumeSimulationStepNs();
    uint64_t nextSimNs = nowNs + nextStepNs;

    // ------------------------------------------------------------------------
    // C) Initialisation des stats de fenetre
    // ------------------------------------------------------------------------

    uint64_t statsWindowStartNs = nowNs;

    // Nombre de ticks simulation reellement executes dans la fenetre courante.
    uint32_t statsExecutedTicks = 0;

    // Somme des temps d'execution reels de ticks sur la fenetre courante.
    uint64_t statsExecSumNs = 0;

    // Somme des retards de reveil sur la fenetre courante
    // uniquement pour les ticks effectivement en retard.
    uint64_t statsLateSumNs = 0;

    // Somme des marges restantes observees sur la fenetre courante.
    uint64_t statsRemainingBudgetSumNs = 0;

    // Temps max observe sur la fenetre courante.
    uint64_t statsMaxExecNsInWindow = 0;

    // Identifiant du tick sur lequel le temps max de la fenetre a ete observe.
    uint64_t statsMaxExecTickIdInWindow = 0;

    // Historique circulaire des temps reels de traitement de tick.
    std::vector<uint64_t> tickExecHistoryNs(tickExecHistoryCapacity, 0);
    size_t tickExecHistoryWriteIndex = 0;
    size_t tickExecHistoryCount = 0;

    // ------------------------------------------------------------------------
    // D) Boucle principale simulation
    // ------------------------------------------------------------------------

    while (serverIsRunning.load(std::memory_order_relaxed))
    {
        nowNs = rcnet_engine_getCurrentTimeNs();

        // Reset du compteur de rattrapage simulation pour cette iteration.
        uint32_t catchUpSim = 0;

        // Tant qu'on est en retard sur la simulation
        // et qu'on n'a pas depasse la limite de catch-up.
        while (nowNs >= nextSimNs && catchUpSim < kMaxCatchUpTicks)
        {
            // Temps reel de depart du tick.
            const uint64_t tickStartNs = rcnet_engine_getCurrentTimeNs();

            // Retard avant execution par rapport a la deadline prevue.
            const uint64_t latenessNs = (tickStartNs > nextSimNs) ? (tickStartNs - nextSimNs) : 0;

            // Execute le tick simulation logique.
            rcnet_engine_simulationUpdate(nextStepNs);

            // L'identifiant du tick simulation vient d'etre incremente
            // dans rcnet_engine_simulationUpdate().
            const uint64_t currentSimulationTickId = simulationTickId;

            // Temps reel de fin du tick.
            const uint64_t tickEndNs = rcnet_engine_getCurrentTimeNs();

            // Temps reel d'execution du tick.
            const uint64_t execNs = tickEndNs - tickStartNs;

            // Temps reel total du tick, depuis la deadline theorique
            // jusqu'a la fin effective du traitement.
            const uint64_t totalRealTickNs = latenessNs + execNs;

            // Marge restante avant de deborder sur le tick suivant.
            const uint64_t remainingBudgetNs =
                (simBudgetNs > totalRealTickNs) ? (simBudgetNs - totalRealTickNs) : 0ull;

            // Publie les dernieres valeurs observees.
            g_simLastExecNs.store(execNs, std::memory_order_relaxed);
            g_simLastLatenessNs.store(latenessNs, std::memory_order_relaxed);

            // Met a jour le max d'execution global de la fenetre.
            {
                uint64_t prevMax = g_simMaxExecNs.load(std::memory_order_relaxed);
                while (execNs > prevMax &&
                       !g_simMaxExecNs.compare_exchange_weak(prevMax, execNs, std::memory_order_relaxed))
                {
                }
            }

            // Met a jour le max de retard global de la fenetre.
            {
                uint64_t prevMax = g_simMaxLatenessNs.load(std::memory_order_relaxed);
                while (latenessNs > prevMax &&
                       !g_simMaxLatenessNs.compare_exchange_weak(prevMax, latenessNs, std::memory_order_relaxed))
                {
                }
            }

            // Met a jour le max d'execution local de la fenetre
            // et memorise le tick sur lequel ce max a ete observe.
            if (execNs > statsMaxExecNsInWindow)
            {
                statsMaxExecNsInWindow = execNs;
                statsMaxExecTickIdInWindow = currentSimulationTickId;
            }

            // Si le tick est lance en retard, on le compte
            // et on ajoute son retard a la somme de fenetre.
            if (latenessNs > 0)
            {
                g_simLateTickCount.fetch_add(1, std::memory_order_relaxed);
                statsLateSumNs += latenessNs;
            }

            // Si on fait du catch-up au-dela du premier tick de cette passe,
            // on compte un tick de rattrapage.
            if (catchUpSim > 0)
            {
                g_simCatchUpTickCount.fetch_add(1, std::memory_order_relaxed);
                g_simCatchUpTickCountSinceStartup.fetch_add(1, std::memory_order_relaxed);
            }

            // Stats de fenetre.
            statsExecutedTicks++;
            statsExecSumNs += execNs;
            statsRemainingBudgetSumNs += remainingBudgetNs;

            // Stocke le temps reel de traitement dans l'historique circulaire.
            tickExecHistoryNs[tickExecHistoryWriteIndex] = execNs;
            tickExecHistoryWriteIndex = (tickExecHistoryWriteIndex + 1) % tickExecHistoryCapacity;

            if (tickExecHistoryCount < tickExecHistoryCapacity)
            {
                tickExecHistoryCount++;
            }

            // Warning tick lent sur le temps de traitement pur.
            if (execNs >= warnBudgetNs && execNs < simBudgetNs)
            {
                RCNET_log(
                    RCNET_LOG_WARN,
                    "SIMULATION_ALERTE: tick lent detecte, temps_reel_de_traitement_du_tick=%.3f ms, retard_de_reveil=%.3f ms, temps_total_reel_du_tick=%.3f ms, budget_maximal_par_tick_avant_de_deborder_sur_le_tick_suivant=%.3f ms, identifiant_tick_simulation=%llu",
                    (double)execNs / 1'000'000.0,
                    (double)latenessNs / 1'000'000.0,
                    (double)totalRealTickNs / 1'000'000.0,
                    (double)simBudgetNs / 1'000'000.0,
                    (unsigned long long)currentSimulationTickId
                );
            }

            // Alerte si le temps reel complet du tick depasse le budget.
            if (totalRealTickNs > simBudgetNs)
            {
                RCNET_log(
                    RCNET_LOG_ERROR,
                    "SIMULATION_ALERTE: tick reel complet hors budget, temps_total_reel_du_tick=%.3f ms, retard_de_reveil=%.3f ms, temps_reel_de_traitement_du_tick=%.3f ms, budget_maximal_par_tick_avant_de_deborder_sur_le_tick_suivant=%.3f ms, identifiant_tick_simulation=%llu",
                    (double)totalRealTickNs / 1'000'000.0,
                    (double)latenessNs / 1'000'000.0,
                    (double)execNs / 1'000'000.0,
                    (double)simBudgetNs / 1'000'000.0,
                    (unsigned long long)currentSimulationTickId
                );
            }

            // Calcule la duree exacte du tick suivant.
            nextStepNs = rcnet_engine_consumeSimulationStepNs();

            // Programme la prochaine echeance absolue.
            nextSimNs += nextStepNs;

            // Incremente le compteur de catch-up.
            catchUpSim++;

            // Relit le temps pour savoir si un autre rattrapage est necessaire.
            nowNs = rcnet_engine_getCurrentTimeNs();
        }

        // Si on est toujours en retard apres le catch-up max,
        // on drop le backlog accumule.
        if (nowNs >= nextSimNs)
        {
            g_simBacklogDropCount.fetch_add(1, std::memory_order_relaxed);
            g_simBacklogDropCountSinceStartup.fetch_add(1, std::memory_order_relaxed);

            RCNET_log(
                RCNET_LOG_WARN,
                "SIMULATION_ALERTE: retard_accumule_trop_important, limite_rattrapage_atteinte=%u, backlog_abandonne, retard_courant=%.3f ms",
                kMaxCatchUpTicks,
                (double)(nowNs - nextSimNs) / 1'000'000.0
            );

            // Recalcule un pas simulation.
            nextStepNs = rcnet_engine_consumeSimulationStepNs();

            // Rebase la prochaine echeance depuis maintenant.
            nextSimNs = nowNs + nextStepNs;
        }

        // --------------------------------------------------------------------
        // E) Publication des stats toutes les 1 seconde
        // --------------------------------------------------------------------

        const uint64_t statsNowNs = rcnet_engine_getCurrentTimeNs();
        const uint64_t statsElapsedNs = statsNowNs - statsWindowStartNs;

        if (statsElapsedNs >= kStatsWindowNs)
        {
            // Hz reel observe sur la fenetre.
            const double realHz =
                (statsElapsedNs > 0)
                    ? ((double)statsExecutedTicks * 1'000'000'000.0 / (double)statsElapsedNs)
                    : 0.0;

            // Temps moyen reel d'execution par tick sur la fenetre.
            const double avgExecMs =
                (statsExecutedTicks > 0)
                    ? ((double)statsExecSumNs / (double)statsExecutedTicks) / 1'000'000.0
                    : 0.0;

            // Snapshot des compteurs atomiques.
            const uint64_t maxLateNs    = g_simMaxLatenessNs.load(std::memory_order_relaxed);
            const uint64_t lateTicks    = g_simLateTickCount.load(std::memory_order_relaxed);
            const uint64_t catchUpTicks = g_simCatchUpTickCount.load(std::memory_order_relaxed);
            const uint64_t backlogDrops = g_simBacklogDropCount.load(std::memory_order_relaxed);
            const uint64_t catchUpTicksSinceStartup =
                g_simCatchUpTickCountSinceStartup.load(std::memory_order_relaxed);
            const uint64_t backlogDropsSinceStartup =
                g_simBacklogDropCountSinceStartup.load(std::memory_order_relaxed);

            // Temps moyen reel de retard de reveil sur la fenetre,
            // uniquement parmi les ticks effectivement en retard.
            const double avgLateMs =
                (lateTicks > 0)
                    ? ((double)statsLateSumNs / (double)lateTicks) / 1'000'000.0
                    : 0.0;

            // Temps moyen total reel par tick:
            // temps de traitement + retard de reveil.
            const double avgTotalRealTickMs =
                (statsExecutedTicks > 0)
                    ? ((double)(statsExecSumNs + statsLateSumNs) / (double)statsExecutedTicks) / 1'000'000.0
                    : 0.0;

            // Marge moyenne restante par tick sur la fenetre.
            const double avgRemainingBudgetMs =
                (statsExecutedTicks > 0)
                    ? ((double)statsRemainingBudgetSumNs / (double)statsExecutedTicks) / 1'000'000.0
                    : 0.0;

            // Calcule p95 / p99 sur l'historique des temps reels de traitement
            // des deux dernieres secondes.
            uint64_t p95ExecNs = 0;
            uint64_t p99ExecNs = 0;

            if (tickExecHistoryCount > 0)
            {
                std::vector<uint64_t> sortedExecHistoryNs(
                    tickExecHistoryNs.begin(),
                    tickExecHistoryNs.begin() + static_cast<std::ptrdiff_t>(tickExecHistoryCount)
                );

                std::sort(sortedExecHistoryNs.begin(), sortedExecHistoryNs.end());

                const size_t p95Index =
                    rcnet_engine_percentileNearestRankIndex(tickExecHistoryCount, 95);

                const size_t p99Index =
                    rcnet_engine_percentileNearestRankIndex(tickExecHistoryCount, 99);

                p95ExecNs = sortedExecHistoryNs[p95Index];
                p99ExecNs = sortedExecHistoryNs[p99Index];
            }

            g_simRealTickRateHz.store((uint32_t)(realHz + 0.5), std::memory_order_relaxed);

            // Met a jour le dernier snapshot expose au reste de l'application.
            {
                std::lock_guard<std::mutex> lock(g_lastSimulationEtatMetricsMutex);

                g_lastSimulationEtatMetrics.frequence_cible_tick_simulation_hz =
                    (uint64_t)g_simTickHz;
                g_lastSimulationEtatMetrics.frequence_reelle_tick_simulation_hz_sur_derniere_seconde =
                    realHz;
                g_lastSimulationEtatMetrics.nombre_ticks_simulation_executes_sur_derniere_seconde =
                    statsExecutedTicks;

                g_lastSimulationEtatMetrics.temps_moyen_par_tick_sur_derniere_seconde_ms =
                    avgExecMs;
                g_lastSimulationEtatMetrics.temps_en_ms_sous_lequel_se_situent_95_pourcent_des_ticks_sur_les_deux_dernieres_secondes =
                    (double)p95ExecNs / 1'000'000.0;
                g_lastSimulationEtatMetrics.temps_en_ms_sous_lequel_se_situent_99_pourcent_des_ticks_sur_les_deux_dernieres_secondes =
                    (double)p99ExecNs / 1'000'000.0;
                g_lastSimulationEtatMetrics.temps_maximum_observe_pour_un_tick_sur_derniere_seconde_ms =
                    (double)statsMaxExecNsInWindow / 1'000'000.0;
                g_lastSimulationEtatMetrics.identifiant_tick_du_temps_maximum_observe_sur_derniere_seconde =
                    statsMaxExecTickIdInWindow;

                g_lastSimulationEtatMetrics.retard_moyen_de_reveil_du_thread_parmi_les_ticks_en_retard_sur_derniere_seconde_ms =
                    avgLateMs;
                g_lastSimulationEtatMetrics.retard_maximum_observe_sur_derniere_seconde_ms =
                    (double)maxLateNs / 1'000'000.0;
                g_lastSimulationEtatMetrics.nombre_de_reveils_du_thread_apres_l_horaire_prevu_sur_derniere_seconde =
                    lateTicks;

                g_lastSimulationEtatMetrics.temps_moyen_total_reel_du_tick_en_comptant_retard_de_reveil_plus_traitement_sur_derniere_seconde_ms =
                    avgTotalRealTickMs;
                g_lastSimulationEtatMetrics.marge_moyenne_restante_avant_de_deborder_sur_le_tick_suivant_sur_derniere_seconde_ms =
                    avgRemainingBudgetMs;
                g_lastSimulationEtatMetrics.budget_maximal_par_tick_avant_de_deborder_sur_le_tick_suivant_ms =
                    (double)simBudgetNs / 1'000'000.0;

                g_lastSimulationEtatMetrics.nombre_ticks_de_rattrapage_executes_sur_derniere_seconde =
                    catchUpTicks;
                g_lastSimulationEtatMetrics.nombre_ticks_de_rattrapage_executes_depuis_le_lancement_du_serveur =
                    catchUpTicksSinceStartup;
                g_lastSimulationEtatMetrics.nombre_abandons_de_backlog_simulation_sur_derniere_seconde =
                    backlogDrops;
                g_lastSimulationEtatMetrics.nombre_abandons_de_backlog_simulation_depuis_le_lancement_du_serveur =
                    backlogDropsSinceStartup;
            }

            g_hasLastSimulationEtatMetrics.store(true, std::memory_order_relaxed);

            /*RCNET_log(
                RCNET_LOG_INFO,
                "SIMULATION_ETAT:\n"
                "  frequence_cible_tick_simulation_hz=%llu\n"
                "  frequence_reelle_tick_simulation_hz_sur_derniere_seconde=%.2f\n"
                "  nombre_ticks_simulation_executes_sur_derniere_seconde=%u\n"
                "  temps_moyen_par_tick_sur_derniere_seconde_ms=%.3f\n"
                "  temps_en_ms_sous_lequel_se_situent_95_pourcent_des_ticks_sur_les_deux_dernieres_secondes=%.3f\n"
                "  temps_en_ms_sous_lequel_se_situent_99_pourcent_des_ticks_sur_les_deux_dernieres_secondes=%.3f\n"
                "  temps_maximum_observe_pour_un_tick_sur_derniere_seconde_ms=%.3f\n"
                "  identifiant_tick_du_temps_maximum_observe_sur_derniere_seconde=%llu\n"
                "  retard_moyen_de_reveil_du_thread_parmi_les_ticks_en_retard_sur_derniere_seconde_ms=%.3f\n"
                "  retard_maximum_observe_sur_derniere_seconde_ms=%.3f\n"
                "  nombre_de_reveils_du_thread_apres_l_horaire_prevu_sur_derniere_seconde=%llu\n"
                "  temps_moyen_total_reel_du_tick_en_comptant_retard_de_reveil_plus_traitement_sur_derniere_seconde_ms=%.3f\n"
                "  marge_moyenne_restante_avant_de_deborder_sur_le_tick_suivant_sur_derniere_seconde_ms=%.3f\n"
                "  budget_maximal_par_tick_avant_de_deborder_sur_le_tick_suivant_ms=%.3f\n"
                "  nombre_ticks_de_rattrapage_executes_sur_derniere_seconde=%llu\n"
                "  nombre_ticks_de_rattrapage_executes_depuis_le_lancement_du_serveur=%llu\n"
                "  nombre_abandons_de_backlog_simulation_sur_derniere_seconde=%llu\n"
                "  nombre_abandons_de_backlog_simulation_depuis_le_lancement_du_serveur=%llu",
                (unsigned long long)g_simTickHz,
                realHz,
                (unsigned)statsExecutedTicks,

                avgExecMs,
                (double)p95ExecNs / 1'000'000.0,
                (double)p99ExecNs / 1'000'000.0,
                (double)statsMaxExecNsInWindow / 1'000'000.0,
                (unsigned long long)statsMaxExecTickIdInWindow,

                avgLateMs,
                (double)maxLateNs / 1'000'000.0,
                (unsigned long long)lateTicks,

                avgTotalRealTickMs,
                avgRemainingBudgetMs,
                (double)simBudgetNs / 1'000'000.0,

                (unsigned long long)catchUpTicks,
                (unsigned long long)catchUpTicksSinceStartup,
                (unsigned long long)backlogDrops,
                (unsigned long long)backlogDropsSinceStartup
            );*/

            // Reset de la fenetre suivante.
            statsWindowStartNs = statsNowNs;
            statsExecutedTicks = 0;
            statsExecSumNs = 0;
            statsLateSumNs = 0;
            statsRemainingBudgetSumNs = 0;
            statsMaxExecNsInWindow = 0;
            statsMaxExecTickIdInWindow = 0;

            g_simMaxExecNs.store(0, std::memory_order_relaxed);
            g_simMaxLatenessNs.store(0, std::memory_order_relaxed);
            g_simLateTickCount.store(0, std::memory_order_relaxed);
            g_simCatchUpTickCount.store(0, std::memory_order_relaxed);
            g_simBacklogDropCount.store(0, std::memory_order_relaxed);
        }

        // Attend jusqu'a la prochaine echeance absolue de simulation.
        rcnet_sleep_until_ns(nextSimNs);
    }

    RCNET_log(RCNET_LOG_INFO, "Thread simulation terminer.");
}

// ============================================================================
// 18) Thread réseau
// ============================================================================

/**
 * \brief Point d'entrée du thread réseau.
 *
 * IMPORTANT :
 * ce thread est le seul autorisé à manipuler ENet.
 */
static void rcnet_engine_networkThreadMain(void)
{
    rcnet_engine_setCurrentThreadPriorityNetworkLinux();

    // Log de démarrage du thread réseau.
    RCNET_log(RCNET_LOG_INFO, "Thread reseau demarrer.");

    // ------------------------------------------------------------------------
    // A) Création du host ENet
    // ------------------------------------------------------------------------

    // Déclare l'adresse ENet de bind.
    ENetAddress address;

    // Construit une adresse "any" IPv6.
    enet_address_build_any(&address, ENET_ADDRESS_TYPE_IPV6);

    // Assigne le port d'écoute.
    address.port = (enet_uint16)g_serverPort;

    // Crée le host ENet serveur.
    g_enetServerHost = enet_host_create(
        ENET_ADDRESS_TYPE_ANY,          // mode dual-stack IPv4/IPv6
        &address,                       // adresse de bind
        (size_t)g_serverMaxClients,     // nombre max de clients
        (size_t)g_serverChannelCount,   // nombre de channels
        0,                              // bande passante entrante illimitée
        0                               // bande passante sortante illimitée
    );

    // Si la création a échoué.
    if (g_enetServerHost == nullptr)
    {
        RCNET_log(RCNET_LOG_CRITICAL,
                  "enet_host_create() a échoué (port=%u, maxClients=%u, channels=%u).",
                  (unsigned)g_serverPort,
                  (unsigned)g_serverMaxClients,
                  (unsigned)g_serverChannelCount);

        // Stop global pour éviter de laisser tourner les autres threads inutilement.
        rcnet_engine_eventQuit();
        return;
    }
    else
    {
        RCNET_log(RCNET_LOG_INFO,
                  "ENet server listening on port %u (dual-stack) with max clients %u and %u channels.",
                  address.port,
                  (unsigned)g_serverMaxClients,
                  (unsigned)g_serverChannelCount);
    }

    // Execute un setup host one-shot immediat apres creation du host, avant le
    // premier enet_host_service().
    //
    // Cela permet aux callbacks utilisateurs d'installer des hooks host-level
    // (ex: enet_host_encrypt / enet_host_compress) des le demarrage reseau.
    rcnet_engine_networkHostSetup(g_enetServerHost);

    // ------------------------------------------------------------------------
    // B) Préparation du timing réseau OUT
    // ------------------------------------------------------------------------

    // Durée exacte du tick réseau sortant courant.
    uint64_t outStepNs = rcnet_engine_consumeNetworkOutgoingStepNs();

    // Prochaine échéance absolue d'envoi.
    uint64_t nextOutNs = 0;

    // Si le tick OUT est actif, on calcule la première deadline.
    if (outStepNs > 0)
    {
        nextOutNs = rcnet_engine_getCurrentTimeNs() + outStepNs;
    }

    // Tant que le serveur tourne.
    while (serverIsRunning.load(std::memory_order_relaxed))
    {
        // --------------------------------------------------------------------
        // 1) Déterminer le timeout du premier enet_host_service()
        // --------------------------------------------------------------------

        // Base : on utilise le timeout de poll réseau entrant configuré.
        uint32_t timeoutMs = g_networkIncomingPollTimeoutMs.load(std::memory_order_relaxed);

        // Si le tick OUT est actif, on borne le timeout
        // pour ne pas rater la prochaine deadline OUT.
        if (outStepNs > 0)
        {
            // Lit le temps courant.
            const uint64_t nowNs = rcnet_engine_getCurrentTimeNs();

            // Calcule le temps restant avant la prochaine deadline OUT.
            const uint64_t outRemainingNs = (nowNs < nextOutNs) ? (nextOutNs - nowNs) : 0;

            // Si on est déjà dans la marge finale, on ne bloque plus.
            if (outRemainingNs <= kFinalOutMarginNs)
            {
                timeoutMs = 0;
            }
            else
            {
                // Retire la marge finale pour obtenir un temps d'attente sûr.
                const uint64_t safeWaitNs = outRemainingNs - kFinalOutMarginNs;

                // Convertit en ms.
                const uint32_t safeWaitMs = (uint32_t)(safeWaitNs / 1'000'000ull);

                // Clamp avec le sleep max configuré côté IN.
                if (safeWaitMs < timeoutMs)
                {
                    timeoutMs = safeWaitMs;
                }

                // Si la conversion tombe à 0 ms, on reste non bloquant.
                if (safeWaitMs == 0)
                {
                    timeoutMs = 0;
                }
            }
        }

        // --------------------------------------------------------------------
        // 2) Premier pump ENet : potentiellement bloquant mais borné
        // --------------------------------------------------------------------

        // Déclare l'événement ENet courant.
        ENetEvent event;

        // Appelle ENet avec le timeout calculé.
        int serviceResult = enet_host_service(
            g_enetServerHost,
            &event,
            (enet_uint32)timeoutMs
        );

        // Si erreur ENet.
        if (serviceResult < 0)
        {
            RCNET_log(RCNET_LOG_ERROR, "enet_host_service() error");
        }
        // Si au moins un événement est disponible.
        else if (serviceResult > 0)
        {
            // ----------------------------------------------------------------
            // 3) Drain réseau entrant avec budget de temps
            // ----------------------------------------------------------------

            // Marque le début du budget de traitement IN.
            const uint64_t incomingBudgetStartNs = rcnet_engine_getCurrentTimeNs();

            // Boucle de drain non bloquante.
            while (true)
            {
                // Traite l'événement réseau courant via le callback utilisateur.
                rcnet_engine_networkIncomingUpdate(g_enetServerHost, &event);

                // Si l'événement contient un packet RECEIVE,
                // on détruit explicitement le packet après traitement.
                if (event.type == ENET_EVENT_TYPE_RECEIVE && event.packet != nullptr)
                {
                    enet_packet_destroy(event.packet);
                    event.packet = nullptr;
                }

                // Lit le temps courant après traitement.
                const uint64_t nowNs = rcnet_engine_getCurrentTimeNs();

                // Si le budget IN est consommé, on stop le drain.
                if ((nowNs - incomingBudgetStartNs) >= kMaxIncomingWorkBudgetNs)
                {
                    break;
                }

                // Si un tick OUT est actif, on évite d'entamer sa marge finale.
                if (outStepNs > 0)
                {
                    const uint64_t outRemainingNs = (nowNs < nextOutNs) ? (nextOutNs - nowNs) : 0;

                    if (outRemainingNs <= kFinalOutMarginNs)
                    {
                        break;
                    }
                }

                // Continue à drainer ENet sans bloquer.
                serviceResult = enet_host_service(g_enetServerHost, &event, 0);

                // S'il n'y a plus d'événement immédiatement disponible, on s'arrête.
                if (serviceResult <= 0)
                {
                    break;
                }
            }
        }

        // --------------------------------------------------------------------
        // 4) Tick réseau sortant cadencé sur horloge absolue
        // --------------------------------------------------------------------

        // Si le tick OUT est actif.
        if (outStepNs > 0)
        {
            // Lit le temps courant.
            uint64_t nowNs = rcnet_engine_getCurrentTimeNs();

            // Reset du compteur de catch-up OUT.
            uint32_t catchUpOut = 0;

            // Tant qu'on est en retard sur OUT
            // et qu'on n'a pas dépassé la limite de catch-up.
            while (nowNs >= nextOutNs && catchUpOut < kMaxCatchUpTicks)
            {
                // Exécute le tick OUT utilisateur.
                rcnet_engine_networkOutgoingUpdate(g_enetServerHost);

                // Calcule la durée exacte du tick OUT suivant.
                outStepNs = rcnet_engine_consumeNetworkOutgoingStepNs();

                // Programme la prochaine deadline absolue.
                nextOutNs += outStepNs;

                // Incrémente le compteur de catch-up.
                catchUpOut++;

                // Relit l'heure pour savoir si on doit encore rattraper.
                nowNs = rcnet_engine_getCurrentTimeNs();
            }

            // Si on est encore trop en retard malgré le catch-up,
            // on drop le backlog accumulé.
            if (nowNs >= nextOutNs)
            {
                RCNET_log(RCNET_LOG_WARN,
                          "Backlog NET OUT trop grand: catch-up atteint (%u). Drop backlog.",
                          kMaxCatchUpTicks);

                // Recalcule un pas réseau sortant exact.
                outStepNs = rcnet_engine_consumeNetworkOutgoingStepNs();

                // Rebase la prochaine deadline à partir de maintenant.
                nextOutNs = nowNs + outStepNs;
            }
        }
    }

    // ------------------------------------------------------------------------
    // C) Nettoyage du host ENet
    // ------------------------------------------------------------------------

    // Si le host existe encore.
    if (g_enetServerHost != nullptr)
    {
        // Vide les buffers ENet avant destruction.
        enet_host_flush(g_enetServerHost);

        // Détruit le host.
        enet_host_destroy(g_enetServerHost);

        // Reset du pointeur global.
        g_enetServerHost = nullptr;
    }

    // Log de fin du thread réseau.
    RCNET_log(RCNET_LOG_INFO, "Thread reseau terminer.");
}

// ============================================================================
// 19) Point d'entrée public moteur
// ============================================================================

bool rcnet_engine_run(RCNET_Callbacks* callbacksUser, const RCNET_ServerConfig* config)
{
    // ------------------------------------------------------------------------
    // A) Validation des paramètres
    // ------------------------------------------------------------------------

    // Si la config est absente, impossible de démarrer.
    if (config == nullptr)
        return false;

    // ------------------------------------------------------------------------
    // B) Application des callbacks utilisateur
    // ------------------------------------------------------------------------

    // Si des callbacks sont fournis, on les enregistre.
    if (callbacksUser != nullptr)
        rcnet_engine_setCallbacks(callbacksUser);

    // ------------------------------------------------------------------------
    // C) Lecture de la configuration runtime
    // ------------------------------------------------------------------------

    // Lit la fréquence simulation depuis la config, avec fallback.
    simulationTickRateHz = (config->simulationTickHz > 0) ? config->simulationTickHz : 128;

    // Lit la fréquence réseau sortante depuis la config, avec fallback.
    networkOutgoingTickRateHz = (config->networkOutgoingTickHz > 0) ? (int)config->networkOutgoingTickHz : 32;

    // Copie le port serveur.
    g_serverPort = config->port;

    // Copie le nombre max de clients.
    g_serverMaxClients = config->maxClients;

    // Copie le nombre de channels.
    g_serverChannelCount = config->channelCount;

    // Copie la durée maximale de poll réseau entrant.
    networkIncomingPollTimeoutMs = config->networkIncomingPollTimeoutMs;

    // Copie la configuration NATS.
    g_natsEnabled =
        (callbacksServerEngine.rcnet_nats_update != nullptr) &&
        (config->natsConfig.natsServerURL != nullptr) &&
        (config->natsConfig.publicKeyNKey != nullptr) &&
        (config->natsConfig.privateKeySeedNKey != nullptr);
    g_natsServerURL = config->natsConfig.natsServerURL;
    g_natsPublicKeyNKey = config->natsConfig.publicKeyNKey;
    g_natsPrivateKeySeedNKey = config->natsConfig.privateKeySeedNKey;
    g_natsSkipVerifyCertsServer = config->natsConfig.skipVerifyCertsServer;
    g_natsUseTLS = config->natsConfig.useTLS;

    // ------------------------------------------------------------------------
    // D) Reset de l'état d'exécution
    // ------------------------------------------------------------------------

    // Remet le flag d'exécution à true.
    serverIsRunning.store(true, std::memory_order_relaxed);

    // Reset l'identifiant simulation interne.
    simulationTickId = 0;

    // Reset l'identifiant réseau entrant interne.
    networkIncomingTickId = 0;

    // Reset l'identifiant réseau sortant interne.
    networkOutgoingTickId = 0;

    // Reset l'identifiant HTTP interne.
    httpTickId = 0;

    // Reset l'identifiant NATS interne.
    natsTickId = 0;

    // Reset le context NATS global.
    g_natsClient.connection = nullptr;
    g_natsClient.subscriptions = nullptr;
    g_natsClient.subscriptionCount = 0;

    // Reset le tick logique exposé.
    g_serverSimulationTick.store(0, std::memory_order_relaxed);

    // Reset le temps logique exposé.
    g_serverTimeNsMonotonic.store(0, std::memory_order_relaxed);
    g_simCatchUpTickCountSinceStartup.store(0, std::memory_order_relaxed);
    g_simBacklogDropCountSinceStartup.store(0, std::memory_order_relaxed);

    // Reset du dernier snapshot de metrics simulation exposé.
    {
        std::lock_guard<std::mutex> lock(g_lastSimulationEtatMetricsMutex);
        g_lastSimulationEtatMetrics = {};
    }
    g_hasLastSimulationEtatMetrics.store(false, std::memory_order_relaxed);

    // ------------------------------------------------------------------------
    // E) Initialisation moteur
    // ------------------------------------------------------------------------
    agones::SDK *sdk = new agones::SDK();

    // Si l'init échoue, on nettoie puis on retourne false.
    if (!rcnet_engine_init())
    {
        rcnet_engine_quit();
        return false;
    }

    // ------------------------------------------------------------------------
    // F) Callback utilisateur de chargement
    // ------------------------------------------------------------------------

    // Si le callback load existe, on l'appelle sur le thread principal.
    if (callbacksServerEngine.rcnet_load != nullptr)
        callbacksServerEngine.rcnet_load();

    // Check si le callback load a demandé un arrêt immédiat.
    if (!serverIsRunning.load(std::memory_order_relaxed))
    {
        if (callbacksServerEngine.rcnet_unload != nullptr)
            callbacksServerEngine.rcnet_unload();

        rcnet_engine_quit();
        return false;
    }

    // ------------------------------------------------------------------------
    // G) Lancement des threads
    // ------------------------------------------------------------------------

    // Lance le thread simulation.
    std::thread simThread(rcnet_engine_simulationThreadMain);

    // Lance le thread réseau.
    std::thread netThread(rcnet_engine_networkThreadMain);

    // Lance le thread HTTP.
    std::thread httpThread;
    bool hasHttpThread = (callbacksServerEngine.rcnet_http_update != nullptr);
    if (hasHttpThread)
    {
        httpThread = std::thread(rcnet_engine_httpThreadMain);
    }

    // Lance le thread NATS si activé.
    std::thread natsThread;
    if (g_natsEnabled)
    {
        natsThread = std::thread(rcnet_engine_natsThreadMain);
    }

    // ------------------------------------------------------------------------
    // H) Attente de fin des threads
    // ------------------------------------------------------------------------

    // Attend la fin du thread simulation.
    simThread.join();

    // Attend la fin du thread réseau.
    netThread.join();

    // Attend la fin du thread HTTP.
    if (hasHttpThread)
    {
        httpThread.join();
    }

    // Attend la fin du thread NATS si activé.
    if (g_natsEnabled)
    {
        natsThread.join();
    }

    // ------------------------------------------------------------------------
    // I) Callback utilisateur de déchargement
    // ------------------------------------------------------------------------

    // Si le callback unload existe, on l'appelle sur le thread principal.
    if (callbacksServerEngine.rcnet_unload != nullptr)
        callbacksServerEngine.rcnet_unload();

    // ------------------------------------------------------------------------
    // J) Nettoyage final moteur
    // ------------------------------------------------------------------------

    // Nettoie les dépendances.
    rcnet_engine_quit();

    // Indique que l'exécution s'est terminée normalement.
    return true;
}
