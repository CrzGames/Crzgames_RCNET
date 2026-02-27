#include "server.h"

#include <RCNET/RCNET.h>

// ------------------------------------------------------------
// Standard C/C++ Libraries
// ------------------------------------------------------------
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

// ------------------------------------------------------------
// RCENet (fork ENet)
// ------------------------------------------------------------
#include <rcenet/RCENET_enet.h>

// ------------------------------------------------------------
// SDL3 (threads + mutex)
// ------------------------------------------------------------
#include <SDL3/SDL.h>

// ------------------------------------------------------------
// cJSON (parsing JSON)
// ------------------------------------------------------------
#include <cJSON.h>

// ============================================================
// 1) Structures de données
// ============================================================

// Input "gameplay" côté serveur (ce que la simulation consomme)
struct RCNET_ClientInput
{
    uint32_t clientId;         // id du client (ex: peer->incomingPeerID)
    uint32_t clientTickId;     // tick côté client (utile pour debug/prediction)
    uint32_t clientInputSeq;   // séquence input (anti-duplication/ack)
    uint32_t buttonsMask;      // bitmask des inputs (WASD, jump, shoot, etc.)
    float axisX;               // stick/mouse axis X
    float axisY;               // stick/mouse axis Y
};

// Ce que le thread réseau envoie au thread simulation :
// un input + le tick serveur auquel il doit s'appliquer.
struct RCNET_QueuedInputForSimulation
{
    uint64_t targetServerSimTickId; // "appliquer à ce tick serveur"
    RCNET_ClientInput input;        // input parsé
};

// ============================================================
// ACK par client : dernier input reçu / appliqué
// ============================================================

// Doit correspondre à la capacité de ton enet_host_create(..., maxClients, ...)
// Ici tu as 64.
static constexpr uint32_t kMaxServerClients = 64;

// Dernier seq reçu (mis à jour par le thread réseau)
static std::atomic<uint32_t> gLastReceivedInputSeqByClientId[kMaxServerClients];

// Dernier seq appliqué (mis à jour par la simulation)
static std::atomic<uint32_t> gLastAppliedInputSeqByClientId[kMaxServerClients];

// Petit helper sécurité: vérifier que clientId est valide
static inline bool IsClientIdInRange(uint32_t clientId)
{
    return clientId < kMaxServerClients;
}

// ============================================================
// 2) Etat global réseau ENet
// ============================================================

// Host serveur ENet (gère les peers, envoi/réception)
static ENetHost* gEnetServerHost = nullptr;

// Thread réseau SDL
static SDL_Thread* gEnetNetworkThreadHandle = nullptr;

// Flag pour arrêter proprement le thread réseau
static std::atomic<bool> gEnetNetworkThreadRunning{false};

// ============================================================
// 3) Communication inter-threads : queue thread-safe
// ============================================================

// Mutex SDL pour protéger la queue
static SDL_Mutex* gIncomingInputsQueueMutex = nullptr;

// Queue: inputs venant du thread réseau, en attente d'être consommés par la simulation
static std::vector<RCNET_QueuedInputForSimulation> gIncomingInputsQueue;

// ============================================================
// 4) Tick serveur partagé
// ============================================================

// Tick simulation serveur courant (mis à jour par la simulation)
static std::atomic<uint64_t> gCurrentServerSimulationTickId{0};

// Input delay : "au tick suivant" => 1
static constexpr uint32_t kServerInputDelayInTicks = 1;

// ============================================================
// 5) Buffer inputs par tick serveur
// ============================================================
//
// On stocke les inputs à appliquer à des ticks futurs.
// Comme on ne veut pas un std::map infini, on utilise un ring buffer.
//
// Exemple : 256 ticks de buffer.
// A 60 Hz, 256 ticks ~ 4.26 secondes.
// Si tu veux plus de marge pour rollback/debug, augmente.
static constexpr uint32_t kScheduledInputsRingBufferSize = 256;

// Un slot du ring buffer : "tous les inputs à appliquer pour ce tick"
struct RCNET_ScheduledInputsSlot
{
    uint64_t serverTickIdForThisSlot = 0;          // quel tick ce slot représente réellement
    std::vector<RCNET_ClientInput> inputsToApply;  // inputs à appliquer
};

// Ring buffer complet
static RCNET_ScheduledInputsSlot gScheduledInputsRing[kScheduledInputsRingBufferSize];

// Helper : obtenir l’index dans le ring buffer pour un tick donné
static inline uint32_t GetRingIndexForServerTick(uint64_t serverTickId)
{
    return static_cast<uint32_t>(serverTickId % kScheduledInputsRingBufferSize);
}

// ============================================================
// 6) Helpers queue thread-safe (réseau -> simulation)
// ============================================================

// Le thread réseau push des inputs ici
static void PushIncomingInputToQueue(const RCNET_QueuedInputForSimulation& queuedInput)
{
    SDL_LockMutex(gIncomingInputsQueueMutex);
    gIncomingInputsQueue.push_back(queuedInput);
    SDL_UnlockMutex(gIncomingInputsQueueMutex);
}

// La simulation récupère tous les inputs d’un coup (swap => lock minimal)
static void PopAllIncomingInputsFromQueue(std::vector<RCNET_QueuedInputForSimulation>& outInputs)
{
    outInputs.clear();

    SDL_LockMutex(gIncomingInputsQueueMutex);
    outInputs.swap(gIncomingInputsQueue); // O(1)
    SDL_UnlockMutex(gIncomingInputsQueueMutex);
}

// Clamp helper (évite les valeurs absurdes / triche / NaN)
static inline float ClampFloat(float v, float minV, float maxV)
{
    if (v < minV) return minV;
    if (v > maxV) return maxV;
    return v;
}

// Parse JSON -> RCNET_ClientInput
// Retourne true si OK, false si JSON invalide ou champs manquants.
static bool ParseJsonClientInput_cJSON(const char* jsonBytes, size_t jsonLength, uint32_t clientId, RCNET_ClientInput& outInput)
{
    // ------------------------------------------------------------
    // 0) Initialiser des valeurs par défaut
    // ------------------------------------------------------------
    outInput.clientId = clientId;
    outInput.clientTickId = 0;
    outInput.clientInputSeq = 0;
    outInput.buttonsMask = 0;
    outInput.axisX = 0.0f;
    outInput.axisY = 0.0f;

    // ------------------------------------------------------------
    // 1) cJSON attend une string null-terminated.
    // ENet packet data n'est pas forcément null-terminated.
    // Donc on copie dans un buffer temporaire avec '\0'.
    // ------------------------------------------------------------
    std::string jsonText;
    jsonText.assign(jsonBytes, jsonBytes + jsonLength);
    jsonText.push_back('\0');

    // ------------------------------------------------------------
    // 2) Parser JSON
    // ------------------------------------------------------------
    cJSON* root = cJSON_Parse(jsonText.c_str());
    if (!root)
        return false;

    // Garantir free à la sortie
    auto cleanup = [&]() { cJSON_Delete(root); };

    // ------------------------------------------------------------
    // 4) Champs requis: clientTick et seq
    // ------------------------------------------------------------
    cJSON* clientTickItem = cJSON_GetObjectItemCaseSensitive(root, "clientTick");
    cJSON* seqItem        = cJSON_GetObjectItemCaseSensitive(root, "seq");
    if (!cJSON_IsNumber(clientTickItem) || !cJSON_IsNumber(seqItem))
    {
        cleanup();
        return false;
    }

    // cJSON number est double -> cast contrôlé
    // (tu peux aussi vérifier plage >0)
    outInput.clientTickId   = (uint32_t)clientTickItem->valuedouble;
    outInput.clientInputSeq = (uint32_t)seqItem->valuedouble;

    // ------------------------------------------------------------
    // 5) Champs optionnels: buttons, ax, ay
    // ------------------------------------------------------------
    cJSON* buttonsItem = cJSON_GetObjectItemCaseSensitive(root, "buttons");
    if (cJSON_IsNumber(buttonsItem))
    {
        // Ici tu peux limiter le mask à un set de bits autorisés
        outInput.buttonsMask = (uint32_t)buttonsItem->valuedouble;
    }

    cJSON* axItem = cJSON_GetObjectItemCaseSensitive(root, "ax");
    if (cJSON_IsNumber(axItem))
    {
        float ax = (float)axItem->valuedouble;
        outInput.axisX = ClampFloat(ax, -1.0f, 1.0f);
    }

    cJSON* ayItem = cJSON_GetObjectItemCaseSensitive(root, "ay");
    if (cJSON_IsNumber(ayItem))
    {
        float ay = (float)ayItem->valuedouble;
        outInput.axisY = ClampFloat(ay, -1.0f, 1.0f);
    }

    // ------------------------------------------------------------
    // 6) Fin
    // ------------------------------------------------------------
    cleanup();
    return true;
}

// ============================================================
// 8) Thread réseau ENet (réception + push queue)
// ============================================================
static int EnetNetworkThreadMain(void* /*userdata*/)
{
    ENetEvent event;

    while (gEnetNetworkThreadRunning.load(std::memory_order_relaxed))
    {
        int serviceResult = enet_host_service(gEnetServerHost, &event, 1); // timeout 1 ms (petit pour réactivité, mais pas 0 pour éviter CPU à 100% si pas de clients)

        if (serviceResult <= 0)
        {
            // 0 => pas d'événement ; <0 => erreur (optionnel à traiter)
            continue;
        }

        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                RCNET_log(RCNET_LOG_INFO, "[ENET] Client connected. peerID=%u\n", event.peer->incomingPeerID);
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
            {
                // 1) Récupérer les bytes du packet ENet
                const char* packetBytes  = reinterpret_cast<const char*>(event.packet->data);
                size_t      packetLength = static_cast<size_t>(event.packet->dataLength);
                uint32_t    clientId     = event.peer->incomingPeerID;
                
                // 2) Parser JSON -> ClientInput
                RCNET_ClientInput parsedInput;
                if (ParseJsonClientInput_cJSON(packetBytes, packetLength, clientId, parsedInput))
                {
                    // Met à jour "dernier seq reçu" pour ce client
                    if (IsClientIdInRange(parsedInput.clientId))
                    {
                        gLastReceivedInputSeqByClientId[parsedInput.clientId].store(parsedInput.clientInputSeq, std::memory_order_relaxed);
                    }
                    
                    // 3) Calculer le tick serveur cible : "tick courant + inputDelay"
                    uint64_t currentServerTickId = gCurrentServerSimulationTickId.load(std::memory_order_relaxed);
                    uint64_t targetServerTickId = currentServerTickId + static_cast<uint64_t>(kServerInputDelayInTicks);

                    RCNET_QueuedInputForSimulation queued;
                    queued.targetServerSimTickId = targetServerTickId;
                    queued.input = parsedInput;

                    // 4) Push dans la queue thread-safe
                    PushIncomingInputToQueue(queued);
                }
                else
                {
                    RCNET_log(RCNET_LOG_WARN, "[ENET] Invalid input JSON from client=%u (len=%zu)\n", clientId, packetLength);
                }

                // 5) Libérer le packet ENet
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                RCNET_log(RCNET_LOG_INFO, "[ENET] Client disconnected. peerID=%u\n", event.peer->incomingPeerID);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            {
                RCNET_log(RCNET_LOG_INFO, "[ENET] Client timed out. peerID=%u\n", event.peer->incomingPeerID);
                break;
            }

            default:
                break;
        }
    }

    return 0;
}

// ============================================================
// 9) Callbacks RCNET : load / unload
// ============================================================

void rcnet_load(void)
{
    RCNET_log(RCNET_LOG_INFO, "Server Loaded (ENet example)\n");

    // Initialiser les ACK seq par client
    for (uint32_t i = 0; i < kMaxServerClients; ++i)
    {
        gLastReceivedInputSeqByClientId[i].store(0, std::memory_order_relaxed);
        gLastAppliedInputSeqByClientId[i].store(0, std::memory_order_relaxed);
    }

    // ----------------------------
    // A) Créer le mutex de queue
    // ----------------------------
    gIncomingInputsQueueMutex = SDL_CreateMutex();
    if (!gIncomingInputsQueueMutex)
    {
        RCNET_log(RCNET_LOG_CRITICAL, "SDL_CreateMutex failed: %s\n", SDL_GetError());
        rcnet_engine_eventQuit();
        return;
    }

    // ----------------------------
    // B) Créer le serveur ENet
    // ----------------------------
    ENetAddress address;
    enet_address_build_any(&address, ENET_ADDRESS_TYPE_IPV6);
    address.port = 7777;

    gEnetServerHost = enet_host_create(
        ENET_ADDRESS_TYPE_ANY, // dual-stack IPv4/IPv6
        &address,
        64,  // max clients
        2,   // channels
        0,   // incoming bandwidth
        0    // outgoing bandwidth
    );
    if (!gEnetServerHost)
    {
        RCNET_log(RCNET_LOG_CRITICAL, "Failed to create ENet server host\n");
        rcnet_engine_eventQuit();
        return;
    }
    RCNET_log(RCNET_LOG_INFO, "ENet server listening on port %u (dual-stack)\n", address.port);

    // ----------------------------
    // C) Lancer le thread réseau
    // ----------------------------
    gEnetNetworkThreadRunning.store(true, std::memory_order_relaxed);
    gEnetNetworkThreadHandle = SDL_CreateThread(EnetNetworkThreadMain, "RCNET_EnetNetworkThread", nullptr);
    if (!gEnetNetworkThreadHandle)
    {
        RCNET_log(RCNET_LOG_CRITICAL, "SDL_CreateThread failed: %s\n", SDL_GetError());
        gEnetNetworkThreadRunning.store(false, std::memory_order_relaxed);
        rcnet_engine_eventQuit();
        return;
    }
}

void rcnet_unload(void)
{
    RCNET_log(RCNET_LOG_INFO, "Server Unloading (ENet example)...\n");

    // ----------------------------
    // A) Stop thread réseau
    // ----------------------------
    if (gEnetNetworkThreadHandle)
    {
        gEnetNetworkThreadRunning.store(false, std::memory_order_relaxed);
        SDL_WaitThread(gEnetNetworkThreadHandle, nullptr);
        gEnetNetworkThreadHandle = nullptr;
    }

    // ----------------------------
    // B) Détruire ENet host
    // ----------------------------
    if (gEnetServerHost)
    {
        enet_host_destroy(gEnetServerHost);
        gEnetServerHost = nullptr;
    }

    // ----------------------------
    // C) Détruire mutex + vider queue
    // ----------------------------
    if (gIncomingInputsQueueMutex)
    {
        SDL_DestroyMutex(gIncomingInputsQueueMutex);
        gIncomingInputsQueueMutex = nullptr;
    }

    // Reset queue et scheduled ring
    gIncomingInputsQueue.clear();
    for (uint32_t i = 0; i < kScheduledInputsRingBufferSize; i++)
    {
        gScheduledInputsRing[i].serverTickIdForThisSlot = 0;
        gScheduledInputsRing[i].inputsToApply.clear();
    }

    RCNET_log(RCNET_LOG_INFO, "Server Unloaded (ENet example)\n");
}

// ============================================================
// 10) Simulation tick (60 Hz)
// ============================================================
//
// Etapes à chaque tick :
// 1) incrémenter serverSimTickId
// 2) publier serverSimTickId dans atomic (pour le thread réseau)
// 3) récupérer tous les inputs reçus (queue)
// 4) ranger ces inputs dans inputsByTick[targetTickId]
// 5) appliquer les inputs du tick courant
// 6) simuler le monde (dt fixe)

void rcnet_simulation_update(double dt)
{
    // 1) Incrémenter le tick serveur
    uint64_t serverSimTickId = gCurrentServerSimulationTickId.load(std::memory_order_relaxed) + 1;
    gCurrentServerSimulationTickId.store(serverSimTickId, std::memory_order_relaxed);

    // 2) Récupérer tous les inputs entrants depuis le réseau (sans bloquer longtemps)
    std::vector<RCNET_QueuedInputForSimulation> newlyReceivedInputs;
    PopAllIncomingInputsFromQueue(newlyReceivedInputs);

    // 3) Placer ces inputs dans le ring buffer pour leur tick cible
    for (const auto& queued : newlyReceivedInputs)
    {
        uint64_t targetTick = queued.targetServerSimTickId;

        uint32_t ringIndex = GetRingIndexForServerTick(targetTick);
        RCNET_ScheduledInputsSlot& slot = gScheduledInputsRing[ringIndex];

        // IMPORTANT : le ring buffer réutilise les slots.
        // Si le slot contient un ancien tick, on doit le reset.
        if (slot.serverTickIdForThisSlot != targetTick)
        {
            slot.serverTickIdForThisSlot = targetTick;
            slot.inputsToApply.clear();
        }

        // Ajout de l'input dans le slot cible
        slot.inputsToApply.push_back(queued.input);
    }

    // 4) Récupérer la liste d’inputs pour CE tick
    uint32_t currentRingIndex = GetRingIndexForServerTick(serverSimTickId);
    RCNET_ScheduledInputsSlot& currentTickSlot = gScheduledInputsRing[currentRingIndex];

    // Si le slot ne correspond pas au tick courant, alors aucun input planifié
    if (currentTickSlot.serverTickIdForThisSlot == serverSimTickId)
    {
        // 5) Appliquer les inputs de CE tick
        for (const RCNET_ClientInput& in : currentTickSlot.inputsToApply)
        {
            // TODO: Ajouter logique de jeu ici réellement

            // Met à jour "dernier seq appliqué"
            if (IsClientIdInRange(in.clientId))
            {
                gLastAppliedInputSeqByClientId[in.clientId].store(in.clientInputSeq, std::memory_order_relaxed);
            }

            // TODO: Ajouter logique de jeu ici: par exemple, convertir buttons/axes -> velocity -> position.
            RCNET_log(
                RCNET_LOG_DEBUG,
                "[SIM tick=%llu] Apply input: client=%u clientTick=%u seq=%u buttons=%u ax=%.2f ay=%.2f\n",
                (unsigned long long)serverSimTickId,
                in.clientId,
                in.clientTickId,
                in.clientInputSeq,
                in.buttonsMask,
                in.axisX,
                in.axisY
            );
        }

        // Important : on peut clear après usage pour libérer mémoire
        // (mais on garde serverTickIdForThisSlot pour la cohérence)
        currentTickSlot.inputsToApply.clear();
    }
    else
    {
        // Aucun input prévu pour ce tick
        // (normal si pas de joueurs ou si aucun input reçu)
    }

    // 6) Simuler le monde (dt fixe = 1/60)
    // -> update gameplay, collisions simples, timers, etc.
}

// ============================================================
// 11) Network tick (30 Hz)
// ============================================================
//
// Ici : envoyer des snapshots/deltas.
// L'exemple : broadcast un JSON minimal avec serverTickId.
//
// IMPORTANT :
// - ne fais pas de logique gameplay ici
// - idéalement tu packs + envoies

void rcnet_network_update(void)
{
    if (!gEnetServerHost)
        return;

    // Tick serveur à inclure dans le snapshot
    uint64_t serverTick = gCurrentServerSimulationTickId.load(std::memory_order_relaxed);

    // On envoie un snapshot par peer car ackSeq est différent pour chaque client.
    for (size_t peerIndex = 0; peerIndex < gEnetServerHost->peerCount; ++peerIndex)
    {
        ENetPeer* peer = &gEnetServerHost->peers[peerIndex];

        // On ne parle qu'aux peers connectés
        if (peer->state != ENET_PEER_STATE_CONNECTED)
            continue;

        // Dans ton code, clientId == incomingPeerID.
        uint32_t clientId = peer->incomingPeerID;

        // Sécurité : si jamais incomingPeerID dépasse ton tableau (devrait pas)
        uint32_t ackSeqApplied = 0;
        uint32_t ackSeqReceived = 0;

        if (IsClientIdInRange(clientId))
        {
            ackSeqApplied  = gLastAppliedInputSeqByClientId[clientId].load(std::memory_order_relaxed);
            ackSeqReceived = gLastReceivedInputSeqByClientId[clientId].load(std::memory_order_relaxed);
        }

        // JSON snapshot minimal :
        // serverTick : tick courant serveur
        // ackApplied : dernier seq input appliqué (le plus important)
        // ackRecv    : dernier seq input reçu (optionnel mais utile debug)
        std::string snapshotJson =
            std::string("{\"serverTick\":") + std::to_string(serverTick) +
            ",\"ackApplied\":" + std::to_string(ackSeqApplied) +
            ",\"ackRecv\":" + std::to_string(ackSeqReceived) +
            "}";

        ENetPacket* packet = enet_packet_create(
            snapshotJson.data(),
            snapshotJson.size(),
            ENET_PACKET_FLAG_UNSEQUENCED
        );

        // Envoi au peer uniquement (pas broadcast)
        enet_peer_send(peer, 0, packet);
    }

    // Flush: pousse les paquets à la socket (latence plus faible)
    enet_host_flush(gEnetServerHost);

    // Log debug (optionnel, mais évite spam si tu as plein de clients)
    // RCNET_log(RCNET_LOG_DEBUG, "[NET] Sent per-peer snapshots tick=%llu\n", (unsigned long long)serverTick);
}