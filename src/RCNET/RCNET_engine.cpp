#include "RCNET/RCNET.h"

// ================================
// Standard C/C++ Libraries
// ================================
#include <stdbool.h>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <atomic>

using namespace std::chrono;

// ================================
// Dependencies Libraries OpenSSL
// ================================
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

// ================================
// Dependencies Libraries libsodium
// ================================
#include <sodium.h>

// ================================
// Dependencies Libraries RCENet
// ================================
#include <rcenet/RCENET_enet.h>

// ======================================================
// 1) Etat global serveur (thread-safe)
// ======================================================
// On utilise atomic<bool> car rcnet_engine_eventQuit() peut être appelée
// depuis un autre thread.
static std::atomic<bool> serverIsRunning{true};

// ======================================================
// 2) Paramètres Simulation Tick
// ======================================================

// Fréquence simulation (Hz). Exemple: 60 => 60 ticks/s
static int simTickRateHz = 60;

// Durée d'un tick simulation en nanosecondes: 1e9 / simTickRateHz
static uint64_t simTickDurationNs = 0;

// dt fixe en secondes: 1.0 / simTickRateHz (fourni au callback simulation)
static double simFixedDt = 0.0;

// ======================================================
// 3) Paramètres Network Tick
// ======================================================

// Fréquence réseau (Hz). Exemple: 20 => 20 envois/s
static int netTickRateHz = 20;

// Durée d'un tick réseau en nanosecondes: 1e9 / netTickRateHz
static uint64_t netTickDurationNs = 0;

// ======================================================
// 4) Identifiants de ticks (utile pour logs / prediction / acks)
// ======================================================

// Identifiants de ticks (utile pour logs / prediction / acks)
static uint64_t simTickId = 0;
static uint64_t netTickId = 0;

// ======================================================
// 5) Callbacks API Server Engine
// ======================================================
RCNET_Callbacks callbacksServerEngine = {
    NULL, // rcnet_load
    NULL, // rcnet_unload
    NULL, // rcnet_simulation_update
    NULL, // rcnet_network_update
};

// ======================================================
// 6) RCENet init/cleanup
// ======================================================

static bool rcnet_engine_initRCENet(void)
{
    if (enet_initialize() < 0)
    {
        RCNET_log(RCNET_LOG_CRITICAL, "Erreur lors de l'initialisation de RCEnet.");
        return false;
    }
    RCNET_log(RCNET_LOG_INFO, "RCENet initialiser avec succes.");
    return true;
}

static void rcnet_engine_cleanupRCENet(void)
{
    enet_deinitialize();
    RCNET_log(RCNET_LOG_INFO, "RCENet nettoyer avec succes.");
}

// ======================================================
// 7) OpenSSL init/cleanup
// ======================================================

static bool rcnet_engine_initOpenssl(void)
{
    if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL) == 0)
    {
        RCNET_log(RCNET_LOG_ERROR, "Erreur lors de l'initialisation d'OpenSSL : %s",
                  ERR_error_string(ERR_get_error(), NULL));
        return false;
    }
    RCNET_log(RCNET_LOG_INFO, "OpenSSL initialisé avec succès.");
    return true;
}

static void rcnet_engine_cleanupOpenssl(void)
{
    ERR_free_strings();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    SSL_COMP_free_compression_methods();
    RCNET_log(RCNET_LOG_INFO, "OpenSSL nettoyé avec succès.");
}

// ======================================================
// 8) libsodium init
// ======================================================

static bool rcnet_engine_initLibSodium(void)
{
    if (sodium_init() < 0)
    {
        RCNET_log(RCNET_LOG_ERROR, "Erreur lors de l'initialisation de libsodium.");
        return false;
    }
    RCNET_log(RCNET_LOG_INFO, "libsodium initialisé avec succès.");
    return true;
}

// ======================================================
// 9) Timing helpers
// ======================================================

// Retourne un temps monotone en ns (steady_clock ne recule jamais)
static uint64_t rcnet_engine_getCurrentTimeNs(void)
{
    auto now = steady_clock::now();
    return static_cast<uint64_t>(duration_cast<nanoseconds>(now.time_since_epoch()).count());
}

// Sleep "semi précis" :
// - on dort une grosse partie du temps
// - on finit en petit spin pour éviter l'oversleep
static inline void rcnet_sleep_until_ns(uint64_t targetTimeNs)
{
    // marge finale (spin) en ns. Plus tu augmentes, plus tu consommes CPU.
    constexpr uint64_t kSpinMarginNs = 200'000; // 200 µs

    while (true)
    {
        uint64_t now = rcnet_engine_getCurrentTimeNs();
        if (now >= targetTimeNs)
            return;

        uint64_t remaining = targetTimeNs - now;

        if (remaining > kSpinMarginNs)
        {
            // Sleep sur le gros du temps restant.
            std::this_thread::sleep_for(std::chrono::nanoseconds(remaining - kSpinMarginNs));
        }
        else
        {
            // Spin court (boucle vide) pour terminer précisément.
            // Alternative: std::this_thread::yield();
        }
    }
}

// ======================================================
// 10) Paramètres robustesse boucle
// ======================================================

// Limite de rattrapage (anti "spirale de la mort").
// Si tu mets 0 => pas de rattrapage (pas recommandé).
static constexpr uint32_t kMaxCatchUpTicks = 5;

// Clamp du frame time.
// Exemple: pause debugger de 10 secondes => sinon backlog énorme.
// Ici on clamp à 250ms.
static constexpr uint64_t kMaxFrameClampNs = 250'000'000ull;

// ======================================================
// 11) Set callbacks
// ======================================================

static void rcnet_engine_setCallbacks(RCNET_Callbacks* callbacksUser)
{
    // Charge / décharge
    if (callbacksUser->rcnet_load   != NULL) callbacksServerEngine.rcnet_load   = callbacksUser->rcnet_load;
    if (callbacksUser->rcnet_unload != NULL) callbacksServerEngine.rcnet_unload = callbacksUser->rcnet_unload;

    // Tick simulation
    if (callbacksUser->rcnet_simulation_update != NULL)
        callbacksServerEngine.rcnet_simulation_update = callbacksUser->rcnet_simulation_update;

    // Tick réseau
    if (callbacksUser->rcnet_network_update != NULL)
        callbacksServerEngine.rcnet_network_update = callbacksUser->rcnet_network_update;
}

// ======================================================
// 12) Init moteur + calcule durées de ticks réseau/simulation
// ======================================================

static bool rcnet_engine_init(void)
{
    // 1) Dépendances
    if (!rcnet_engine_initOpenssl())   return false;
    if (!rcnet_engine_initRCENet())   return false;
    if (!rcnet_engine_initLibSodium()) return false;

    // 2) Calcul tick simulation
    simTickDurationNs = static_cast<uint64_t>(1'000'000'000ull) / static_cast<uint64_t>(simTickRateHz);
    simFixedDt        = 1.0 / static_cast<double>(simTickRateHz);

    // 3) Calcul tick réseau
    netTickDurationNs = static_cast<uint64_t>(1'000'000'000ull) / static_cast<uint64_t>(netTickRateHz);

    return true;
}

static void rcnet_engine_quit(void)
{
    rcnet_engine_cleanupOpenssl();
    rcnet_engine_cleanupRCENet();
}

// ======================================================
// 13) Ticks séparés
// ======================================================

// 13.A) Tick simulation (logique serveur)
static inline void rcnet_engine_simulationTick(void)
{
    // Incrémente tickId simulation
    simTickId++;

    // Appel callback utilisateur (si défini)
    if (callbacksServerEngine.rcnet_simulation_update != NULL)
    {
        callbacksServerEngine.rcnet_simulation_update(simFixedDt);
    }
}

// 13.B) Tick réseau (envoi de packets udp, flush, etc.)
static inline void rcnet_engine_networkTick(void)
{
    // Incrémente tickId réseau
    netTickId++;

    // Appel callback utilisateur (si défini)
    if (callbacksServerEngine.rcnet_network_update != NULL)
    {
        callbacksServerEngine.rcnet_network_update();
    }
}

// ======================================================
// 14) Stop event (thread-safe)
// ======================================================
void rcnet_engine_eventQuit(void)
{
    serverIsRunning.store(false, std::memory_order_relaxed);
}

// ======================================================
// 15) Run avec boucle principale + timing séparé simulation/réseau
// ======================================================
bool rcnet_engine_run(RCNET_Callbacks* callbacksUser, int simTickRateArg, int netTickRateArg)
{
    // -----------------------
    // A) Appliquer callbacks
    // -----------------------
    if (callbacksUser != NULL)
        rcnet_engine_setCallbacks(callbacksUser);

    // -----------------------
    // B) Lire/valider les Hz
    // -----------------------

    // Simulation: si <= 0, fallback 60
    simTickRateHz = (simTickRateArg > 0) ? simTickRateArg : 60;

    // Réseau: si <= 0, fallback 20
    netTickRateHz = (netTickRateArg > 0) ? netTickRateArg : 20;

    // Sécurité: éviter division par zéro
    if (simTickRateHz <= 0) simTickRateHz = 60;
    if (netTickRateHz <= 0) netTickRateHz = 20;

    // -----------------------
    // C) Init moteur
    // -----------------------
    if (!rcnet_engine_init())
    {
        rcnet_engine_quit();
        return false;
    }

    // -----------------------
    // D) Callback load
    // -----------------------
    if (callbacksServerEngine.rcnet_load != NULL)
        callbacksServerEngine.rcnet_load();

    // -----------------------
    // E) Init boucle timing
    // -----------------------

    // lastTimeNs = dernier timestamp (réel)
    uint64_t lastTimeNs = rcnet_engine_getCurrentTimeNs();

    // accumulateur simulation: quantité de "temps" à simuler
    uint64_t accSimNs = 0;

    // accumulateur réseau: quantité de "temps" à traiter réseau
    uint64_t accNetNs = 0;

    // -----------------------
    // F) Boucle principale
    // -----------------------
    while (serverIsRunning.load(std::memory_order_relaxed))
    {
        // 1) Mesure temps réel
        uint64_t nowNs = rcnet_engine_getCurrentTimeNs();

        // frameNs = temps réel écoulé depuis dernière itération
        uint64_t frameNs = nowNs - lastTimeNs;
        lastTimeNs = nowNs;

        // 2) Clamp (évite backlog énorme)
        if (frameNs > kMaxFrameClampNs)
            frameNs = kMaxFrameClampNs;

        // 4) On accumule ce temps pour simulation ET réseau
        accSimNs += frameNs;
        accNetNs += frameNs;

        // -----------------------------------------
        // 5) Ticks simulation : rattrapage limité
        // -----------------------------------------
        uint32_t catchUpSim = 0;
        while (accSimNs >= simTickDurationNs && catchUpSim < kMaxCatchUpTicks)
        {
            rcnet_engine_simulationTick();
            accSimNs -= simTickDurationNs;
            catchUpSim++;
        }

        // Si backlog simulation encore trop grand => drop contrôlé
        if (accSimNs >= simTickDurationNs)
        {
            RCNET_log(RCNET_LOG_WARN,
                      "Backlog SIM trop grand: catch-up atteint (%u). Drop backlog.",
                      kMaxCatchUpTicks);

            // On garde au max 1 tick de backlog (ou 0 si tu préfères)
            accSimNs = simTickDurationNs;
        }

        // -----------------------------------------
        // 6) Ticks réseau : rattrapage limité
        // -----------------------------------------
        uint32_t catchUpNet = 0;
        while (accNetNs >= netTickDurationNs && catchUpNet < kMaxCatchUpTicks)
        {
            rcnet_engine_networkTick();
            accNetNs -= netTickDurationNs;
            catchUpNet++;
        }

        // Si backlog réseau encore trop grand => drop contrôlé
        if (accNetNs >= netTickDurationNs)
        {
            RCNET_log(RCNET_LOG_WARN,
                      "Backlog NET trop grand: catch-up atteint (%u). Drop backlog.",
                      kMaxCatchUpTicks);

            accNetNs = netTickDurationNs;
        }

        // -----------------------------------------
        // 7) Sleep jusqu'au prochain "événement"
        // -----------------------------------------
        // On calcule combien de temps avant le prochain tick SIM et le prochain tick NET,
        // puis on dort jusqu'au plus proche des deux (minimum).
        uint64_t simRemainingNs = (accSimNs < simTickDurationNs) ? (simTickDurationNs - accSimNs) : 0;
        uint64_t netRemainingNs = (accNetNs < netTickDurationNs) ? (netTickDurationNs - accNetNs) : 0;

        // Le prochain événement = min(simRemaining, netRemaining) si > 0
        uint64_t sleepNs = 0;

        if (simRemainingNs == 0 && netRemainingNs == 0)
        {
            // On est pile sur un tick => pas de sleep
            sleepNs = 0;
        }
        else if (simRemainingNs == 0)
        {
            sleepNs = netRemainingNs;
        }
        else if (netRemainingNs == 0)
        {
            sleepNs = simRemainingNs;
        }
        else
        {
            sleepNs = (simRemainingNs < netRemainingNs) ? simRemainingNs : netRemainingNs;
        }

        // Dors si on a du temps
        if (sleepNs > 0)
        {
            uint64_t targetWakeNs = rcnet_engine_getCurrentTimeNs() + sleepNs;
            rcnet_sleep_until_ns(targetWakeNs);
        }
    }

    // -----------------------
    // G) Callback unload
    // -----------------------
    if (callbacksServerEngine.rcnet_unload != NULL)
        callbacksServerEngine.rcnet_unload();

    // -----------------------
    // H) Quit / cleanup
    // -----------------------
    rcnet_engine_quit();

    return true;
}