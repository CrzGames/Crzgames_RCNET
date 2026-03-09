#include "RCNET/RCNET.h"

// ============================================================================
// Standard C/C++
// ============================================================================

#include <stdbool.h>  // bool
#include <cstdlib>    // std::size_t, utilitaires généraux C/C++
#include <chrono>     // horloges et durées
#include <thread>     // std::thread, std::this_thread::sleep_for
#include <atomic>     // std::atomic

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

// Durée de sommeil du thread HTTP entre deux itérations.
static std::atomic<uint32_t> g_httpThreadSleepMs{1};

// Durée de sommeil du thread NATS entre deux itérations.
static std::atomic<uint32_t> g_natsThreadSleepMs{1};

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
static RCNET_NATSClient g_natsClient = {0};
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

// Durée de sommeil du thread HTTP entre deux itérations.
static uint32_t httpThreadSleepMs = 1;

// Durée de sommeil du thread NATS entre deux itérations.
static uint32_t natsThreadSleepMs = 1;

// Fréquence réseau sortante en Hz.
// Exemple : 32 signifie 32 ticks d'envoi par seconde.
static int networkOutgoingTickRateHz = 32;

// Durée d'un tick réseau sortant en nanosecondes.
// Calculée à partir de networkOutgoingTickRateHz.
static uint64_t networkOutgoingTickDurationNs = 0;

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
static RCNET_Callbacks callbacksServerEngine = {
    nullptr, // rcnet_load
    nullptr, // rcnet_unload
    nullptr, // rcnet_simulation_update
    nullptr, // rcnet_network_incoming_update
    nullptr, // rcnet_network_outgoing_update
    nullptr,  // rcnet_http_update
    nullptr  // rcnet_nats_update
};

// ============================================================================
// 8) Helpers temps
// ============================================================================

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
 * \brief Retourne le temps monotone courant en nanosecondes.
 *
 * Utilise steady_clock pour garantir que l'horloge ne recule jamais.
 */
static uint64_t rcnet_engine_getCurrentTimeNs(void)
{
    // Lit l'heure monotone courante.
    auto now = steady_clock::now();

    // Convertit le temps écoulé depuis l'epoch de steady_clock en nanosecondes.
    return static_cast<uint64_t>(duration_cast<nanoseconds>(now.time_since_epoch()).count());
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

    // Boucle jusqu'à atteindre ou dépasser l'instant cible.
    while (true)
    {
        // Récupère l'heure actuelle.
        uint64_t now = rcnet_engine_getCurrentTimeNs();

        // Si on a atteint la cible, on sort.
        if (now >= targetTimeNs)
            return;

        // Calcule le temps restant.
        uint64_t remaining = targetTimeNs - now;

        // S'il reste assez longtemps, on dort sur la plus grosse partie.
        if (remaining > kSpinMarginNs)
        {
            std::this_thread::sleep_for(std::chrono::nanoseconds(remaining - kSpinMarginNs));
        }
        else
        {
            // Sinon on laisse volontairement la boucle tourner
            // pour terminer plus précisément.
            // Alternative possible : std::this_thread::yield();
        }
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
        RCNET_log(RCNET_LOG_CRITICAL, "Erreur lors de l'initialisation de RCEnet.");
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
                  "Erreur lors de l'initialisation d'OpenSSL%s%s",
                  err ? " : " : "",
                  err ? ERR_error_string(err, nullptr) : "");
        return false;
    }

    // Log succès.
    RCNET_log(RCNET_LOG_INFO, "OpenSSL initialisé avec succès.");
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
        RCNET_log(RCNET_LOG_ERROR, "Erreur lors de l'initialisation de libsodium.");
        return false;
    }

    // Log succès.
    RCNET_log(RCNET_LOG_INFO, "libsodium initialisé avec succès.");
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

    // Si le callback réseau sortant existe, on l'enregistre.
    if (callbacksUser->rcnet_network_outgoing_update)
        callbacksServerEngine.rcnet_network_outgoing_update = callbacksUser->rcnet_network_outgoing_update;

    // Si le callback HTTP existe, on l'enregistre.
    if (callbacksUser->rcnet_http_update)
        callbacksServerEngine.rcnet_http_update = callbacksUser->rcnet_http_update;

    // Si le callback NATS existe, on l'enregistre.
    if (callbacksUser->rcnet_nats_update)
        callbacksServerEngine.rcnet_nats_update = callbacksUser->rcnet_nats_update;
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

    // Calcule la durée d'un tick OUT en nanosecondes.
    networkOutgoingTickDurationNs = 1'000'000'000ull / outHz;

    // Expose la fréquence simulation utilisée.
    g_simulationTickRateHz.store((uint32_t)g_simTickHz, std::memory_order_relaxed);

    // Expose la fréquence réseau sortante utilisée.
    g_networkOutgoingTickRateHz.store((uint32_t)outHz, std::memory_order_relaxed);

    // Expose le timeout de poll réseau entrant utilisé.
    g_networkIncomingPollTimeoutMs.store(networkIncomingPollTimeoutMs, std::memory_order_relaxed);

    // Expose la durée de sommeil du thread HTTP entre deux itérations.
    g_httpThreadSleepMs.store(httpThreadSleepMs, std::memory_order_relaxed);

    // Expose la durée de sommeil du thread NATS entre deux itérations.
    g_natsThreadSleepMs.store(natsThreadSleepMs, std::memory_order_relaxed);

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
static inline void rcnet_engine_natsUpdate(RCNET_NATSClient* client)
{
    // Incrémente le compteur interne de tick NATS.
    natsTickId++;

    // Si le callback utilisateur existe, on l'appelle.
    if (callbacksServerEngine.rcnet_nats_update != nullptr)
        callbacksServerEngine.rcnet_nats_update(client);
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

uint32_t rcnet_engine_getNetworkOutgoingTickRateHz(void)
{
    // Retourne la fréquence réseau sortante utilisée.
    return g_networkOutgoingTickRateHz.load(std::memory_order_relaxed);
}

uint32_t rcnet_engine_getHttpThreadSleepMs(void)
{
    // Retourne la durée de sommeil du thread HTTP entre deux itérations.
    return g_httpThreadSleepMs.load(std::memory_order_relaxed);
}

uint32_t rcnet_engine_getNatsThreadSleepMs(void)
{
    // Retourne la durée de sommeil du thread NATS entre deux itérations.
    return g_natsThreadSleepMs.load(std::memory_order_relaxed);
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
}

// ============================================================================
// 16) Thread HTTP
// ============================================================================

/**
 * \brief Point d'entrée du thread HTTP.
 */
static void rcnet_engine_httpThreadMain(void)
{
    // Tant que le serveur tourne.
    while (serverIsRunning.load(std::memory_order_relaxed))
    {
        // Exécute un tick HTTP.
        rcnet_engine_httpUpdate();

        // Petite pause pour éviter de monopoliser le CPU.
        std::this_thread::sleep_for(
            std::chrono::milliseconds(g_httpThreadSleepMs.load(std::memory_order_relaxed))
        );
    }

    // Log de fin du thread HTTP.
    RCNET_log(RCNET_LOG_INFO, "Thread HTTP terminé.");
}

// ============================================================================
// 17) Thread NATS
// ============================================================================

/**
 * \brief Point d'entrée du thread NATS.
 */
static void rcnet_engine_natsThreadMain(void)
{
    bool natsReady = false;

    // Initialise le client NATS si activé.
    if (g_natsEnabled)
    {
        // Note : on passe les paramètres de connexion NATS via des variables globales
        if (rcnet_nats_initialize(
                &g_natsClient,
                g_natsServerURL,
                g_natsUseTLS,
                g_natsSkipVerifyCertsServer,
                g_natsPublicKeyNKey,
                g_natsPrivateKeySeedNKey) != 0)
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

        // Petite pause pour éviter de monopoliser le CPU.
        std::this_thread::sleep_for(
            std::chrono::milliseconds(g_natsThreadSleepMs.load(std::memory_order_relaxed))
        );
    }

    if (natsReady)
    {
        rcnet_nats_cleanup(&g_natsClient);
    }

    // Log de fin du thread NATS.
    RCNET_log(RCNET_LOG_INFO, "Thread NATS terminé.");
}

// ============================================================================
// 17) Thread simulation
// ============================================================================

/**
 * \brief Point d'entrée du thread simulation.
 *
 * Cette boucle exécute une simulation à tick fixe basée sur des échéances absolues.
 */
static void rcnet_engine_simulationThreadMain(void)
{
    // Lit le temps courant au démarrage du thread.
    uint64_t nowNs = rcnet_engine_getCurrentTimeNs();

    // Calcule la durée exacte du premier tick simulation.
    uint64_t nextStepNs = rcnet_engine_consumeSimulationStepNs();

    // Calcule la première échéance absolue de simulation.
    uint64_t nextSimNs = nowNs + nextStepNs;

    // Tant que le serveur tourne.
    while (serverIsRunning.load(std::memory_order_relaxed))
    {
        // Relit le temps courant.
        nowNs = rcnet_engine_getCurrentTimeNs();

        // Reset du compteur de rattrapage simulation pour cette itération.
        uint32_t catchUpSim = 0;

        // Tant qu'on est en retard sur la simulation
        // et qu'on n'a pas dépassé la limite de catch-up.
        while (nowNs >= nextSimNs && catchUpSim < kMaxCatchUpTicks)
        {
            // Exécute le tick simulation courant.
            rcnet_engine_simulationUpdate(nextStepNs);

            // Calcule la durée exacte du tick suivant.
            nextStepNs = rcnet_engine_consumeSimulationStepNs();

            // Programme la prochaine échéance absolue.
            nextSimNs += nextStepNs;

            // Incrémente le compteur de catch-up.
            catchUpSim++;

            // Relit le temps pour savoir si un autre rattrapage est nécessaire.
            nowNs = rcnet_engine_getCurrentTimeNs();
        }

        // Si on est toujours en retard après le catch-up max,
        // on drop le backlog accumulé.
        if (nowNs >= nextSimNs)
        {
            RCNET_log(RCNET_LOG_WARN,
                      "Backlog SIM trop grand: catch-up atteint (%u). Drop backlog.",
                      kMaxCatchUpTicks);

            // Recalcule un pas simulation.
            nextStepNs = rcnet_engine_consumeSimulationStepNs();

            // Rebase la prochaine échéance depuis maintenant.
            nextSimNs = nowNs + nextStepNs;
        }

        // Attend jusqu'à la prochaine échéance absolue de simulation.
        rcnet_sleep_until_ns(nextSimNs);
    }
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

    // ------------------------------------------------------------------------
    // B) Préparation du timing réseau OUT
    // ------------------------------------------------------------------------

    // Durée d'un tick réseau sortant.
    const uint64_t outPeriodNs = networkOutgoingTickDurationNs;

    // Prochaine échéance absolue d'envoi.
    uint64_t nextOutNs = 0;

    // Si le tick OUT est actif, on calcule la première deadline.
    if (outPeriodNs > 0)
    {
        nextOutNs = rcnet_engine_getCurrentTimeNs() + outPeriodNs;
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
        if (outPeriodNs > 0)
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
                if (event.type == ENET_EVENT_TYPE_RECEIVE)
                {
                    enet_packet_destroy(event.packet);
                }

                // Lit le temps courant après traitement.
                const uint64_t nowNs = rcnet_engine_getCurrentTimeNs();

                // Si le budget IN est consommé, on stop le drain.
                if ((nowNs - incomingBudgetStartNs) >= kMaxIncomingWorkBudgetNs)
                {
                    break;
                }

                // Si un tick OUT est actif, on évite d'entamer sa marge finale.
                if (outPeriodNs > 0)
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
        if (outPeriodNs > 0)
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

                // Programme la prochaine deadline absolue.
                nextOutNs += outPeriodNs;

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

                // Rebase la prochaine deadline à partir de maintenant.
                nextOutNs = nowNs + outPeriodNs;
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
    RCNET_log(RCNET_LOG_INFO, "ENet server host détruit (thread réseau terminé).");
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

    // Copie la durée de sommeil du thread HTTP entre deux itérations.
    httpThreadSleepMs = config->httpThreadSleepMs;

    // Copie la durée de sommeil du thread NATS entre deux itérations.
    natsThreadSleepMs = config->natsThreadSleepMs;

    // Copie la configuration NATS.
    g_natsEnabled = config->natsConfig.enabled && config->natsConfig.natsServerURL != nullptr && config->natsConfig.publicKeyNKey != nullptr && config->natsConfig.privateKeySeedNKey != nullptr;
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

    // 
    g_natsClient.connection = nullptr;
    g_natsClient.subscriptions = nullptr;
    g_natsClient.subscriptionCount = 0;
    g_natsClient.jetStreamContext = nullptr;

    // Reset le tick logique exposé.
    g_serverSimulationTick.store(0, std::memory_order_relaxed);

    // Reset le temps logique exposé.
    g_serverTimeNsMonotonic.store(0, std::memory_order_relaxed);

    // ------------------------------------------------------------------------
    // E) Initialisation moteur
    // ------------------------------------------------------------------------

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

    // ------------------------------------------------------------------------
    // G) Lancement des threads
    // ------------------------------------------------------------------------

    // Lance le thread simulation.
    std::thread simThread(rcnet_engine_simulationThreadMain);

    // Lance le thread réseau.
    std::thread netThread(rcnet_engine_networkThreadMain);

    // Lance le thread HTTP.
    std::thread httpThread(rcnet_engine_httpThreadMain);

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
    httpThread.join();

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