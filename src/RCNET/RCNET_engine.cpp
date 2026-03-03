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
// 0) Compteurs exposés (thread-safe)
// ======================================================

// Tick simulation "serveur"
static std::atomic<uint64_t> g_serverSimulationTick{0};

// Temps monotone serveur en ns depuis démarrage moteur (logique, pas wall-clock)
static std::atomic<uint64_t> g_serverTimeNsMonotonic{0};

// Hz effectivement utilisés
static std::atomic<uint32_t> g_simulationTickRateHz{60};
static std::atomic<uint32_t> g_networkIncomingTickRateHz{128};
static std::atomic<uint32_t> g_networkOutgoingTickRateHz{32};

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
static int simulationTickRateHz = 60;

// Durée d'un tick simulation en nanosecondes: 1e9 / simulationTickRateHz
static uint64_t simulationTickDurationNs = 0;

// dt fixe en secondes: 1.0 / simulationTickRateHz (fourni au callback simulation)
static double simulationFixedDt = 0.0;

// ======================================================
// 3) Paramètres Network Tick (IN / OUT)
// ======================================================

// Fréquence réseau (Hz). Exemple: 128 => 128 envois/s
static int networkIncomingTickRateHz = 128;
// Durée d'un tick réseau en nanosecondes: 1e9 / networkIncomingTickRateHz
static uint64_t networkIncomingTickDurationNs = 0;

// Fréquence réseau (Hz). Exemple: 32 => 32 envois/s
static int networkOutgoingTickRateHz = 32;
// Durée d'un tick réseau en nanosecondes: 1e9 / networkOutgoingTickRateHz
static uint64_t networkOutgoingTickDurationNs = 0;

// ======================================================
// 4) Identifiants de ticks internes (debug)
// ======================================================
static uint64_t simulationTickId = 0;
static uint64_t networkIncomingTickId = 0;
static uint64_t networkOutgoingTickId = 0;

// ======================================================
// 5) Callbacks API Server Engine
// ======================================================
RCNET_Callbacks callbacksServerEngine = {
    NULL, // rcnet_load
    NULL, // rcnet_unload
    NULL, // rcnet_simulation_update
    NULL, // rcnet_network_incoming_update
    NULL, // rcnet_network_outgoing_update
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
    if (callbacksUser->rcnet_load)   
        callbacksServerEngine.rcnet_load = callbacksUser->rcnet_load;

    if (callbacksUser->rcnet_unload) 
        callbacksServerEngine.rcnet_unload = callbacksUser->rcnet_unload;

    if (callbacksUser->rcnet_simulation_update)
        callbacksServerEngine.rcnet_simulation_update = callbacksUser->rcnet_simulation_update;

    if (callbacksUser->rcnet_network_incoming_update)
        callbacksServerEngine.rcnet_network_incoming_update = callbacksUser->rcnet_network_incoming_update;

    if (callbacksUser->rcnet_network_outgoing_update)
        callbacksServerEngine.rcnet_network_outgoing_update = callbacksUser->rcnet_network_outgoing_update;
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
    simulationTickDurationNs = static_cast<uint64_t>(1'000'000'000ull) / static_cast<uint64_t>(simulationTickRateHz);
    simulationFixedDt        = 1.0 / static_cast<double>(simulationTickRateHz);

    // 3) Calcul tick réseau IN/OUT
    networkIncomingTickDurationNs = static_cast<uint64_t>(1'000'000'000ull) / static_cast<uint64_t>(networkIncomingTickRateHz);
    networkOutgoingTickDurationNs = static_cast<uint64_t>(1'000'000'000ull) / static_cast<uint64_t>(networkOutgoingTickRateHz);

    // Expose Hz utilisés
    g_simulationTickRateHz.store((uint32_t)simulationTickRateHz, std::memory_order_relaxed);
    g_networkIncomingTickRateHz.store((uint32_t)networkIncomingTickRateHz, std::memory_order_relaxed);
    g_networkOutgoingTickRateHz.store((uint32_t)networkOutgoingTickRateHz, std::memory_order_relaxed);

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
    // Incrémente le tick courant du serveur de simulation
    simulationTickId++;

    // Expose le tick courant (thread-safe)
    g_serverSimulationTick.store(simulationTickId, std::memory_order_relaxed);
    
    // Expose le temps monotone logique du serveur en ns (thread-safe)
    g_serverTimeNsMonotonic.fetch_add(simulationTickDurationNs, std::memory_order_relaxed);

    // Appel callback utilisateur (si défini)
    if (callbacksServerEngine.rcnet_simulation_update != NULL)
    {
        callbacksServerEngine.rcnet_simulation_update(simulationFixedDt);
    }
}

// 13.B) Tick réseau IN (réception de paquets, etc.)
static inline void rcnet_engine_networkIncomingTick(void)
{
    // Incrémente networkIncomingTickId pour le réseau IN
    networkIncomingTickId++;

    // Appel callback utilisateur (si défini)
    if (callbacksServerEngine.rcnet_network_incoming_update != NULL)
    {
        callbacksServerEngine.rcnet_network_incoming_update();
    }
}

// 13.C) Tick réseau OUT (envoi de snapshots, etc.)
static inline void rcnet_engine_networkOutgoingTick(void)
{   
    // Incrémente networkOutgoingTickId pour le réseau OUT
    networkOutgoingTickId++;

    // Appel callback utilisateur (si défini)
    if (callbacksServerEngine.rcnet_network_outgoing_update != NULL)
        callbacksServerEngine.rcnet_network_outgoing_update();
}

// ======================================================
// 14) Getters thread-safe
// ======================================================
uint64_t rcnet_engine_getCurrentServerSimulationTick(void)
{
    return g_serverSimulationTick.load(std::memory_order_relaxed);
}

uint64_t rcnet_engine_getCurrentServerTimeNsMonotonic(void)
{
    return g_serverTimeNsMonotonic.load(std::memory_order_relaxed);
}

uint32_t rcnet_engine_getSimulationTickRateHz(void)
{
    return g_simulationTickRateHz.load(std::memory_order_relaxed);
}

uint32_t rcnet_engine_getNetworkIncomingTickRateHz(void)
{
    return g_networkIncomingTickRateHz.load(std::memory_order_relaxed);
}

uint32_t rcnet_engine_getNetworkOutgoingTickRateHz(void)
{
    return g_networkOutgoingTickRateHz.load(std::memory_order_relaxed);
}

// ======================================================
// 14) Stop event (thread-safe)
// ======================================================
void rcnet_engine_eventQuit(void)
{
    serverIsRunning.store(false, std::memory_order_relaxed);
}

// ======================================================
// 15) Run boucle principale
// ======================================================
bool rcnet_engine_run(RCNET_Callbacks* callbacksUser,
                      int simTickRateArg,
                      int networkIncomingTickRateArg,
                      int networkOutgoingTickRateArg)
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
    simulationTickRateHz = (simTickRateArg > 0) ? simTickRateArg : 60;

    // Réseau IN: si <= 0, fallback 128
    networkIncomingTickRateHz = (networkIncomingTickRateArg > 0) ? networkIncomingTickRateArg : 128;

    // Réseau OUT: si <= 0, fallback 32
    networkOutgoingTickRateHz = (networkOutgoingTickRateArg > 0) ? networkOutgoingTickRateArg : 32;

    // Par sécurité, on recalcule les durées de ticks réseau/simulation même si les arguments sont invalides (ex: -1)
    if (simulationTickRateHz <= 0) simulationTickRateHz = 60;
    if (networkIncomingTickRateHz <= 0) networkIncomingTickRateHz = 128;
    if (networkOutgoingTickRateHz <= 0) networkOutgoingTickRateHz = 32;

    // Reset état run (si jamais rcnet_engine_run est relancé)
    serverIsRunning.store(true, std::memory_order_relaxed);
    simulationTickId = 0;
    networkIncomingTickId = 0;
    networkOutgoingTickId = 0;
    g_serverSimulationTick.store(0, std::memory_order_relaxed);
    g_serverTimeNsMonotonic.store(0, std::memory_order_relaxed);

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

    // accumulateur réseau: quantité de "temps" à traiter pour le réseau
    uint64_t accNetInNs = 0;
    uint64_t accNetOutNs = 0;

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
        accNetInNs += frameNs;
        accNetOutNs += frameNs;

        // -----------------------------------------
        // 5) Ticks simulation : rattrapage limité
        // -----------------------------------------
        uint32_t catchUpSim = 0;
        while (accSimNs >= simulationTickDurationNs && catchUpSim < kMaxCatchUpTicks)
        {
            rcnet_engine_simulationTick();
            accSimNs -= simulationTickDurationNs;
            catchUpSim++;
        }
        // Si backlog simulation encore trop grand => drop contrôlé
        if (accSimNs >= simulationTickDurationNs)
        {
            RCNET_log(RCNET_LOG_WARN,
                      "Backlog SIM trop grand: catch-up atteint (%u). Drop backlog.",
                      kMaxCatchUpTicks);

            // On garde au max 1 tick de backlog (ou 0 si tu préfères)
            accSimNs = simulationTickDurationNs;
        }

        // -----------------------------------------
        // 6) Ticks réseau (IN): rattrapage limité
        // -----------------------------------------
        uint32_t catchUpIn = 0;
        while (accNetInNs >= networkIncomingTickDurationNs && catchUpIn < kMaxCatchUpTicks)
        {
            rcnet_engine_networkIncomingTick();
            accNetInNs -= networkIncomingTickDurationNs;
            catchUpIn++;
        }
        // Si backlog réseau encore trop grand => drop contrôlé
        if (accNetInNs >= networkIncomingTickDurationNs)
        {
            RCNET_log(RCNET_LOG_WARN,
                      "Backlog NET IN trop grand: catch-up atteint (%u). Drop backlog.",
                      kMaxCatchUpTicks);

            accNetInNs = networkIncomingTickDurationNs;
        }

        // -----------------------------------------
        // 6b) Ticks réseau (OUT): rattrapage limité
        // -----------------------------------------
        uint32_t catchUpOut = 0;
        while (accNetOutNs >= networkOutgoingTickDurationNs && catchUpOut < kMaxCatchUpTicks)
        {
            rcnet_engine_networkOutgoingTick();
            accNetOutNs -= networkOutgoingTickDurationNs;
            catchUpOut++;
        }
        // Si backlog réseau encore trop grand => drop contrôlé
        if (accNetOutNs >= networkOutgoingTickDurationNs)
        {
            RCNET_log(RCNET_LOG_WARN,
                      "Backlog NET OUT trop grand: catch-up atteint (%u). Drop backlog.",
                      kMaxCatchUpTicks);

            accNetOutNs = networkOutgoingTickDurationNs;
        }

        // -----------------------------------------
        // 7) Sleep jusqu'au prochain "événement"
        // -----------------------------------------
        // On calcule combien de temps avant le prochain tick SIM et le prochain tick NET,
        // puis on dort jusqu'au plus proche des deux (minimum).
        uint64_t simRemainingNs = (accSimNs < simulationTickDurationNs) ? (simulationTickDurationNs - accSimNs) : 0;
        uint64_t inRemainingNs  = (accNetInNs < networkIncomingTickDurationNs) ? (networkIncomingTickDurationNs - accNetInNs) : 0;
        uint64_t outRemainingNs = (accNetOutNs < networkOutgoingTickDurationNs) ? (networkOutgoingTickDurationNs - accNetOutNs) : 0;

        // Le prochain événement = min(simRemaining, netRemaining) si > 0
        uint64_t sleepNs = 0;

        // min non-zero
        sleepNs = simRemainingNs;
        if (sleepNs == 0 || (inRemainingNs != 0 && inRemainingNs < sleepNs)) sleepNs = inRemainingNs;
        if (sleepNs == 0 || (outRemainingNs != 0 && outRemainingNs < sleepNs)) sleepNs = outRemainingNs;

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