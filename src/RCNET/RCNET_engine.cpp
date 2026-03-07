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

// ======================================================
// 2bis) Simulation time exact (no drift)
// ======================================================
static uint64_t g_simTickHz = 128;
static uint64_t g_simTickBaseNs = 0;
static uint64_t g_simTickRem = 0;
static uint64_t g_simTickRemAcc = 0;

// ======================================================
// X) Configuration serveur (runtime) pour ENet
// ======================================================
static uint16_t g_serverPort = 0;
static uint32_t g_serverMaxClients = 0;
static uint32_t g_serverChannelCount = 0;

// ======================================================
// X) Host ENet (thread réseau uniquement)
// ======================================================
static ENetHost* g_enetServerHost = nullptr;

// ======================================================
// 0) Compteurs exposés (thread-safe)
// ======================================================

// Le tick de simulation courant du serveur (tick logique, incrémenté à chaque tick simulation)
static std::atomic<uint64_t> g_serverSimulationTick{0};

// Temps monotone serveur en nanoseconds depuis le démarrage moteur (logique, pas wall-clock)
static std::atomic<uint64_t> g_serverTimeNsMonotonic{0};

// Hz effectivement utilisés
static std::atomic<uint32_t> g_simulationTickRateHz{128};
static std::atomic<uint32_t> g_networkIncomingSleepMs{1};
static std::atomic<uint32_t> g_networkOutgoingTickRateHz{32};

// ======================================================
// 1) Etat global serveur (thread-safe)
// ======================================================
static std::atomic<bool> serverIsRunning{true};

// ======================================================
// 2) Paramètres Simulation Tick
// ======================================================

// Fréquence simulation (Hz). Exemple: 128 => 128 ticks/s
static int simulationTickRateHz = 128;

// ======================================================
// 3) Paramètres Network Tick (IN / OUT)
// ======================================================

// Durée en ms de sommeil entre chaque tick réseau IN (réception de paquets).
static uint32_t networkIncomingSleepMs = 1;

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
    nullptr, // rcnet_load
    nullptr, // rcnet_unload
    nullptr, // rcnet_simulation_update
    nullptr, // rcnet_network_incoming_update
    nullptr, // rcnet_network_outgoing_update
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
    if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr) == 0)
    {
        RCNET_log(RCNET_LOG_ERROR, "Erreur lors de l'initialisation d'OpenSSL : %s",
                  ERR_error_string(ERR_get_error(), nullptr));
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

static inline uint64_t rcnet_engine_consumeSimulationStepNs(void)
{
    uint64_t stepNs = g_simTickBaseNs;
    g_simTickRemAcc += g_simTickRem;

    if (g_simTickRemAcc >= g_simTickHz)
    {
        g_simTickRemAcc -= g_simTickHz;
        stepNs += 1;
    }

    return stepNs;
}

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
    uint64_t Hz = (simulationTickRateHz > 0) ? (uint64_t)simulationTickRateHz : 128ull;
    g_simTickHz = Hz;
    g_simTickBaseNs = 1'000'000'000ull / Hz;
    g_simTickRem    = 1'000'000'000ull % Hz;
    g_simTickRemAcc = 0;

    // 3) Calcul tick réseau OUT
    uint64_t outHz = (networkOutgoingTickRateHz > 0) ? (uint64_t)networkOutgoingTickRateHz : 32ull;
    networkOutgoingTickDurationNs = 1'000'000'000ull / outHz;

    // Expose paramètres utilisés (thread-safe)
    g_simulationTickRateHz.store((uint32_t)g_simTickHz, std::memory_order_relaxed);
    g_networkOutgoingTickRateHz.store((uint32_t)outHz, std::memory_order_relaxed);
    g_networkIncomingSleepMs.store(networkIncomingSleepMs, std::memory_order_relaxed);

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
static inline void rcnet_engine_simulationUpdate(uint64_t dtNs)
{
    simulationTickId++;
    g_serverSimulationTick.store(simulationTickId, std::memory_order_relaxed);

    constexpr double kNsToSec = 1.0 / 1'000'000'000.0;
    double dt = (double)dtNs * kNsToSec;

    uint64_t prev = g_serverTimeNsMonotonic.fetch_add(dtNs, std::memory_order_relaxed);
    uint64_t serverTimeNs = prev + dtNs;

    if (callbacksServerEngine.rcnet_simulation_update)
        callbacksServerEngine.rcnet_simulation_update(simulationTickId, serverTimeNs, dtNs, dt);
}

// 13.B) Tick réseau IN (réception de paquets, etc.)
static inline void rcnet_engine_networkIncomingUpdate(ENetHost* host, const ENetEvent* event)
{
    // Incrémente networkIncomingTickId pour le réseau IN
    networkIncomingTickId++;

    // Appel callback utilisateur (si défini)
    if (callbacksServerEngine.rcnet_network_incoming_update != nullptr)
    {
        callbacksServerEngine.rcnet_network_incoming_update(host, event);
    }
}

// 13.C) Tick réseau OUT (envoi de snapshots, etc.)
static inline void rcnet_engine_networkOutgoingUpdate(ENetHost* host)
{   
    // Incrémente networkOutgoingTickId pour le réseau OUT
    networkOutgoingTickId++;

    // Appel callback utilisateur (si défini)
    if (callbacksServerEngine.rcnet_network_outgoing_update != nullptr)
        callbacksServerEngine.rcnet_network_outgoing_update(host);
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

uint32_t rcnet_engine_getNetworkIncomingSleepMs(void)
{
    return g_networkIncomingSleepMs.load(std::memory_order_relaxed);
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

// Thread simulation : tick fixe
static void rcnet_engine_simulationThreadMain(void)
{
    uint64_t nowNs = rcnet_engine_getCurrentTimeNs();

    // Première échéance absolue
    uint64_t nextStepNs = rcnet_engine_consumeSimulationStepNs();
    uint64_t nextSimNs = nowNs + nextStepNs;

    while (serverIsRunning.load(std::memory_order_relaxed))
    {
        nowNs = rcnet_engine_getCurrentTimeNs();

        uint32_t catchUpSim = 0;

        while (nowNs >= nextSimNs && catchUpSim < kMaxCatchUpTicks)
        {
            rcnet_engine_simulationUpdate(nextStepNs);

            nextStepNs = rcnet_engine_consumeSimulationStepNs();
            nextSimNs += nextStepNs;

            catchUpSim++;
            nowNs = rcnet_engine_getCurrentTimeNs();
        }

        if (nowNs >= nextSimNs)
        {
            RCNET_log(RCNET_LOG_WARN,
                      "Backlog SIM trop grand: catch-up atteint (%u). Drop backlog.",
                      kMaxCatchUpTicks);

            nextStepNs = rcnet_engine_consumeSimulationStepNs();
            nextSimNs = nowNs + nextStepNs;
        }

        rcnet_sleep_until_ns(nextSimNs);
    }
}

// Thread réseau : SEUL thread autorisé à toucher ENet
static void rcnet_engine_networkThreadMain(void)
{
    // -----------------------
    // A) Créer le serveur ENet (bind/listen)
    // -----------------------
    ENetAddress address;
    enet_address_build_any(&address, ENET_ADDRESS_TYPE_IPV6);
    address.port = (enet_uint16)g_serverPort;
    g_enetServerHost = enet_host_create(ENET_ADDRESS_TYPE_ANY, // dual-stack IPv4/IPv6
                                        &address,
                                        (size_t)g_serverMaxClients,   // max clients
                                        (size_t)g_serverChannelCount, // number of channels
                                        0,                            // incoming bandwidth (0 = unlimited)
                                        0                             // outgoing bandwidth (0 = unlimited)
                                       );
    if (g_enetServerHost == nullptr)
    {
        RCNET_log(RCNET_LOG_CRITICAL, "enet_host_create() a échoué (port=%u, maxClients=%u, channels=%u).",
                  (unsigned)g_serverPort, (unsigned)g_serverMaxClients, (unsigned)g_serverChannelCount);

        // Stop global : sinon le sim thread tourne dans le vide
        rcnet_engine_eventQuit();
        return;
    }
    else
    {
        RCNET_log(RCNET_LOG_INFO, "ENet server listening on port %u (dual-stack)\n", address.port);
    }

    // -----------------------
    // B) Timing OUT (tick Hz)
    // -----------------------
    // IMPORTANT :
    // Ici on déclenche le tick OUT sur une horloge ABSOLUE (deadline),
    // pas sur "combien de temps a duré la loop précédente".
    // => Ça réduit fortement le jitter, même si enet_host_service() bloque un peu.
    const uint64_t outPeriodNs = networkOutgoingTickDurationNs;

    // Prochaine deadline OUT (horloge absolue)
    uint64_t nextOutNs = 0;
    if (outPeriodNs > 0)
        nextOutNs = rcnet_engine_getCurrentTimeNs() + outPeriodNs;

    // marge finale avant deadline OUT : on évite de bloquer ENet trop près de la deadline
    // (plus petit => plus précis mais plus de CPU)
    constexpr uint64_t kFinalOutMarginNs = 200'000ull; // 200 µs

    while (serverIsRunning.load(std::memory_order_relaxed))
    {
        // -----------------------------------------
        // 1) Calcul du timeout dynamique pour ENet
        // -----------------------------------------
        // Objectif : ne jamais bloquer au-delà du prochain tick OUT.
        // timeout_ms = min(networkIncomingSleepMs, ms_avant_prochain_out_tick)
        uint32_t timeoutMs = networkIncomingSleepMs;

        // Temps restant avant deadline OUT (ns)
        uint64_t outRemainingNs = 0;

        if (outPeriodNs > 0)
        {
            uint64_t nowNs = rcnet_engine_getCurrentTimeNs();

            // Temps restant avant le prochain tick OUT (deadline absolue)
            outRemainingNs = (nowNs < nextOutNs) ? (nextOutNs - nowNs) : 0;

            // PIÈGE jitter #2 :
            // - ns->ms (entier) + oversleep OS peuvent faire rater la deadline OUT
            // Solution :
            // - Tant qu'on est "loin", on peut bloquer via enet_host_service(timeoutMs).
            // - Dans la marge finale (<= kFinalOutMarginNs), on ne bloque PLUS (timeoutMs = 0).
            if (outRemainingNs <= kFinalOutMarginNs)
            {
                timeoutMs = 0;
            }
            else
            {
                // On retire une marge finale pour revenir AVANT la deadline OUT.
                // (sinon un oversleep OS de ~0.2-1ms peut créer du jitter OUT)
                uint64_t safeWaitNs = outRemainingNs - kFinalOutMarginNs;

                // Convert ns -> ms (floor) sur safeWaitNs (c'est volontaire)
                // => garantit de ne pas bloquer trop longtemps.
                uint32_t safeWaitMs = (uint32_t)(safeWaitNs / 1'000'000ull);

                // clamp avec networkIncomingSleepMs
                if (timeoutMs != 0 && safeWaitMs < timeoutMs)
                    timeoutMs = safeWaitMs;

                // si safeWaitMs tombe à 0, on met 0 (non-bloquant)
                if (safeWaitMs == 0)
                    timeoutMs = 0;
            }
        }

        // -----------------------------------------
        // 2) NETWORK IN : pump ENet via enet_host_service()
        // -----------------------------------------
        // On "service" ENet avec un timeout court (ou 0 si on veut non-bloquant).
        ENetEvent event;
        int serviceResult = enet_host_service(g_enetServerHost, &event, (enet_uint32)timeoutMs);

        if (serviceResult < 0)
        {
            RCNET_log(RCNET_LOG_ERROR, "enet_host_service() error");
            // Option: rcnet_engine_eventQuit();
            rcnet_engine_networkIncomingUpdate(g_enetServerHost, nullptr);
        }
        // Si on a au moins 1 event, on le traite et on draine les suivants sans attendre (timeout 0).
        else if (serviceResult > 0)
        {
            do
            {
                // Appel callback utilisateur (réseau IN)
                rcnet_engine_networkIncomingUpdate(g_enetServerHost, &event);

                if (event.type == ENET_EVENT_TYPE_RECEIVE)
                    enet_packet_destroy(event.packet);

                // Continue à vider la file d'events sans bloquer
                serviceResult = enet_host_service(g_enetServerHost, &event, 0);
            }
            while (serviceResult > 0);
        }
        else if (serviceResult == 0)
        {
            // Pas d'événement ENet, mais on peut quand même laisser la callback "tick IN" tourner
            rcnet_engine_networkIncomingUpdate(g_enetServerHost, nullptr);
        }

        // -----------------------------------------
        // 3) NETWORK OUT : cadencer à tick Hz (horloge absolue)
        // -----------------------------------------
        // Ici on tick OUT sur deadline absolue "nextOutNs".
        // Même si le IN a attendu, le OUT "rattrape" avec catch-up limité.
        if (outPeriodNs > 0)
        {
            uint64_t nowNs = rcnet_engine_getCurrentTimeNs();

            uint32_t catchUpOut = 0;
            while (nowNs >= nextOutNs && catchUpOut < kMaxCatchUpTicks)
            {
                rcnet_engine_networkOutgoingUpdate(g_enetServerHost);
                nextOutNs += outPeriodNs;
                catchUpOut++;
            }

            // Si backlog réseau encore trop grand => drop contrôlé
            if (nowNs >= nextOutNs)
            {
                RCNET_log(RCNET_LOG_WARN,
                          "Backlog NET OUT trop grand: catch-up atteint (%u). Drop backlog.",
                          kMaxCatchUpTicks);

                // Re-synchronise : on repart sur "maintenant + 1 période"
                nextOutNs = nowNs + outPeriodNs;
            }
        }
        else
        {
            // Sécurité : si outPeriodNs == 0, on ne tick pas OUT
            // (config invalide, mais on évite division / boucles infinies)
        }
    }

    // -----------------------
    // C) Cleanup ENet host
    // -----------------------
    if (g_enetServerHost != nullptr)
    {
        // Flush avant de détruire pour éviter de perdre des paquets en buffer
        enet_host_flush(g_enetServerHost);

        // Détruire le host ENet (ferme les connexions, libère les ressources, etc.)
        enet_host_destroy(g_enetServerHost);
        g_enetServerHost = nullptr;
    }

    RCNET_log(RCNET_LOG_INFO, "ENet server host détruit (thread réseau terminé).");
}

// ======================================================
// 15) Run moteur (multi-thread)
// ======================================================
bool rcnet_engine_run(RCNET_Callbacks* callbacksUser, const RCNET_ServerConfig* config)
{
    // -----------------------
    // A) Valider inputs
    // -----------------------
    if (config == nullptr)
        return false;

    // -----------------------
    // B) Appliquer callbacks
    // -----------------------
    if (callbacksUser != nullptr)
        rcnet_engine_setCallbacks(callbacksUser);

    // -----------------------
    // C) Lire / valider config
    // -----------------------
    simulationTickRateHz      = (config->simulationTickHz > 0) ? config->simulationTickHz : 128;
    networkOutgoingTickRateHz = (config->networkOutgoingTickHz > 0) ? (int)config->networkOutgoingTickHz : 32;
    g_serverPort = config->port;
    g_serverMaxClients = config->maxClients;
    g_serverChannelCount = config->channelCount;
    networkIncomingSleepMs = config->networkIncomingSleepMs;

    // Reset état run (si jamais rcnet_engine_run est relancé)
    serverIsRunning.store(true, std::memory_order_relaxed);
    simulationTickId = 0;
    networkIncomingTickId = 0;
    networkOutgoingTickId = 0;
    g_serverSimulationTick.store(0, std::memory_order_relaxed);
    g_serverTimeNsMonotonic.store(0, std::memory_order_relaxed);

    // -----------------------
    // D) Init moteur
    // -----------------------
    if (!rcnet_engine_init())
    {
        rcnet_engine_quit();
        return false;
    }

    // -----------------------
    // E) Callback load (thread principal)
    // -----------------------
    if (callbacksServerEngine.rcnet_load != nullptr)
        callbacksServerEngine.rcnet_load();

    // -----------------------
    // F) Lancer threads
    // -----------------------
    std::thread simThread(rcnet_engine_simulationThreadMain);
    std::thread netThread(rcnet_engine_networkThreadMain);

    // -----------------------
    // G) Join (bloquant)
    // -----------------------
    simThread.join();
    netThread.join();

    // -----------------------
    // H) Callback unload (thread principal)
    // -----------------------
    if (callbacksServerEngine.rcnet_unload != nullptr)
        callbacksServerEngine.rcnet_unload();

    // -----------------------
    // I) Quit / cleanup
    // -----------------------
    rcnet_engine_quit();
    return true;
}