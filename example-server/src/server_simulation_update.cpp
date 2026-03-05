#include "server_context.h"
#include "server_network_snapshot_packets.h"
#include "server_world.h"
#include "server_debug_network_stats.h"

#include <RCNET/RCNET.h>

#include <cstdint>         // uintptr_t
#include <cstring>         // memcpy
#include <deque>           // std::deque
#include <unordered_map>   // std::unordered_map

// ======================================================================================
// Internal helpers - Queue draining & message handling
// ======================================================================================

static void ServerSimulationUpdate_DrainNetworkToSimulationQueue_IntoLocalDeque(
    NetworkINToSimulationQueue& netToSimQueue,
    std::deque<NetworkINToSimulationMessage>& messages)
{
    // 2) Drainer la queue (moins de lock)
    netToSimQueue.drain(messages);
}

static void ServerSimulationUpdate_HandleConnectMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& msg)
{
    // Créer une session pour ce client avec cette connectionId
    ClientSession session{};
    session.connectionId = msg.connectionId;
    session.isAuthenticated = false; // pas encore (handshake)
    session.accountIdDatabase = 0;    // pas encore (handshake)
    session.serverLastProcessedInputSequenceNumber = 0;

    // Ajouter la session au network state
    networkState.sessions[msg.connectionId] = session;

    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [CONNECT] - connectionId=%u (session created)\n",
              msg.connectionId);
}

static void ServerSimulationUpdate_HandleDisconnectMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& msg)
{
    // Supprimer la session du client avec cette connectionId
    std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
    if (sit != networkState.sessions.end())
    {
        networkState.sessions.erase(sit);
    }

    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [DISCONNECT] - connectionId=%u (session removed)\n",
              msg.connectionId);
}

static void ServerSimulationUpdate_HandleInputMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& msg)
{
    // Trouver la session du client qui a envoyé cet input
    std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
    if (sit == networkState.sessions.end())
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [SIMULATION] [INPUT] - Received input for unknown connectionId=%u (ignoring)\n",
                  msg.connectionId);
        return;
    }

    // Session trouvée, traiter l'input
    ClientSession& session = sit->second;

    // Garde le dernier input reçu (utile si on n’a rien de neuf ce tick)
    session.latestReceivedInputCommand = msg.input;

    // Met à jour le dernier snapshot ACKé par le client (pour la reconciliation côté client et pour estimer la latence)
    session.clientLastAckedSnapshotId = msg.input.lastReceivedSnapshotId;

    // Anti-doublons / ordre
    if (msg.input.inputSequenceNumber <= session.serverLastProcessedInputSequenceNumber)
    {
        // input déjà traité ou trop vieux
        return;
    }

    // Mettre en queue pour consommation par la simulation (par tick)
    session.pendingInputCommandsQueue.push_back(msg.input);

    // NOTE : on ne met PAS serverLastProcessedInputSequenceNumber ici
    // parce que "processed" = doit être mis à jour quand l’input est réellement appliqué au monde (pas au moment où il arrive).

    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [INPUT] - queued connectionId=%u seq=%u (queue size=%zu)\n",
              msg.connectionId,
              msg.input.inputSequenceNumber,
              session.pendingInputCommandsQueue.size());
}

static void ServerSimulationUpdate_ProcessIncomingNetworkMessages(
    NetworkState& networkState,
    std::deque<NetworkINToSimulationMessage>& messages)
{
    // 4) Traiter les messages réseau (création/suppression session, input queue, etc.)
    for (std::deque<NetworkINToSimulationMessage>::iterator it = messages.begin();
         it != messages.end();
         ++it)
    {
        NetworkINToSimulationMessage& msg = *it;

        if (msg.type == NetworkINToSimulationMessageType::CONNECT)
        {
            ServerSimulationUpdate_HandleConnectMessage(networkState, msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::DISCONNECT)
        {
            ServerSimulationUpdate_HandleDisconnectMessage(networkState, msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::PACKET_INPUT)
        {
            ServerSimulationUpdate_HandleInputMessage(networkState, msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::PACKET_HANDSHAKE)
        {
            // Plus tard : valider token, set accountIdDatabase, session.isAuthenticated = true, etc.
        }
        else if (msg.type == NetworkINToSimulationMessageType::PACKET_EVENT_IMPORTANT)
        {
            // Traiter les messages importants du client (ex: events de gameplay, chat, etc.)
        }
    }
}

static void ServerSimulationUpdate_RunGameplayLogicAndWorldSimulation(GameState& gameState, uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt)
{
    // ================================================================================
    // SIMULER LE MONDE, APPLIQUER LA LOGIQUE DE JEU, ETC.
    // ================================================================================
    ServerWorld_Simulate(gameState, currentTick, serverTimeNs, dtNs, dt);
}   

static bool ServerSimulationUpdate_ShouldBuildAndSendSnapshotsBasedOnNetworkOutgoingRate(uint64_t currentTick)
{
    // ==================================================================================
    // SNAPSHOTS : GATING AU RYTHME NETWORK OUT
    //
    // Objectif :
    // - La SIMULATION tourne à 128Hz
    // - Le NETWORK OUT tourne à 32Hz
    // => On ne construit/envoye des snapshots QUE 32 fois/s (1 toutes les 4 ticks simulation)
    //
    // Avantages :
    // - évite de remplir la queue sim->net inutilement
    // - snapshotId avance au rythme réel d’envoi (propre pour ACK/delta plus tard)
    // - CPU/mémoire plus stables
    // ==================================================================================
    uint32_t simHz = rcnet_engine_getSimulationTickRateHz();
    uint32_t outHz = rcnet_engine_getNetworkOutgoingTickRateHz();
    uint32_t period = rcnet_engine_computeSnapshotPeriodFromRates(simHz, outHz);

    /*RCNET_log(RCNET_LOG_DEBUG,
              "[SERVER] [SIMULATION] currentTick=%llu simHz=%u outHz=%u snapshotPeriod=%u\n",
              (unsigned long long)currentTick,
              simHz,
              outHz,
              period);*/

    // Si on n'est pas sur un tick "d'envoi", on s'arrête ici.
    // (Tu continues évidemment à simuler ton monde au-dessus, mais pas tu ne construis/envoyes de snapshot ce tick)
    if ((period != 0) && ((currentTick % period) != 0))
    {
        return false;
    }

    return true;
}

static void ServerSimulationUpdate_CreateSnapshotFullAndPushToSimulationToNetworkQueue(
    SimulationToNetworkOUTQueue& simToNetQueue,
    ClientSession& session,
    uint64_t currentTick)
{
    // Construire un snapshot (pour l’instant: header uniquement)
    SnapshotHeader header{};
    header.serverTick = currentTick;
    header.serverTimeNs = rcnet_engine_getCurrentServerTimeNsMonotonic();
    header.lastProcessedInputSequenceNumber = session.serverLastProcessedInputSequenceNumber;

    // Construire un message de snapshot à envoyer au client via la queue simulation -> réseau
    SimulationToNetworkOUTMessage outMsg{};
    outMsg.type = SimulationToNetworkOUTMessageType::SNAPSHOT_FULL;
    outMsg.connectionId = session.connectionId;

    outMsg.payload.resize(sizeof(SnapshotHeader));
    std::memcpy(outMsg.payload.data(), &header, sizeof(SnapshotHeader));

    simToNetQueue.push(outMsg);

    // Debug stats
    g_dbg_snapshotsBuilt++;
}

static void ServerSimulationUpdate_BuildSnapshotsForAllSessionsAndEnqueueToNetworkOut(
    SimulationToNetworkOUTQueue& simToNetQueue,
    NetworkState& networkState,
    uint64_t currentTick)
{
    // ==================================================================================
    // Construire des snapshots de l'état du monde pour chaque client
    // et les envoyer via la queue simulation -> réseau
    // ==================================================================================
    for (std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.begin();
         sit != networkState.sessions.end();
         ++sit)
    {
        // Récupérer la session courante dans le tableau de sessions
        ClientSession& session = sit->second;

        ServerSimulationUpdate_CreateSnapshotFullAndPushToSimulationToNetworkQueue(
            simToNetQueue, session, currentTick);
    }
}

// ======================================================================================
// Public entry point called by server_callbacks.cpp
// ======================================================================================

void ServerSimulationUpdate_RunFullSimulationPipelineForCurrentTick(uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt)
{
    // 1) Récupérer la queue réseau -> simulation et la queue simulation -> réseau (pour envoyer des messages à la fin de ce tick)
    NetworkINToSimulationQueue& netToSimQueue = GetNetworkINToSimulationQueue();
    SimulationToNetworkOUTQueue& simToNetQueue = GetSimulationToNetworkOUTQueue();

    // 2) Drainer la queue (moins de lock)
    std::deque<NetworkINToSimulationMessage> messages;
    ServerSimulationUpdate_DrainNetworkToSimulationQueue_IntoLocalDeque(netToSimQueue, messages);

    // 3) Accès au state
    GameState& gameState = GetGameState();
    NetworkState& networkState = GetNetworkState();

    // 4) Traiter les messages réseau (création/suppression session, input queue, etc.)
    ServerSimulationUpdate_ProcessIncomingNetworkMessages(networkState, messages);

    // 5) Simuler le monde, appliquer la logique de jeu, etc.
    ServerSimulationUpdate_RunGameplayLogicAndWorldSimulation(gameState, currentTick, serverTimeNs, dtNs, dt);

    // 6) Bloquer la construction/envoi des snapshots au rythme du NETWORK OUT (ex: 32Hz) et pas de la SIMULATION (ex: 128Hz)
    if (!ServerSimulationUpdate_ShouldBuildAndSendSnapshotsBasedOnNetworkOutgoingRate(currentTick))
    {
        return;
    }

    // 7) Construire et enqueuer les snapshots (full pour l’instant)
    ServerSimulationUpdate_BuildSnapshotsForAllSessionsAndEnqueueToNetworkOut(simToNetQueue, networkState, currentTick);
}