#include "simulation/entrypoint.h"

#include "core/context.h"
#include "game/world/entrypoint.h"
#include "simulation/queue_draining.h"
#include "simulation/process/network_incoming_dispatcher.h"
#include "simulation/process/http_dispatcher.h"
#include "simulation/process/nats_dispatcher.h"
#include "simulation/process/nats/publish_gameserver_simulation_etat_metrics.h"
#include "simulation/match_flow.h"
#include "simulation/snapshots.h"

#include <chrono> // std::chrono::steady_clock, std::chrono::duration
#include <deque>  // std::deque

#include <RCNET/RCNET.h>

void ServerSimulation_DrainNetworkIncomingAndHttpAndNatsMessages_And_RunSimulationLogic(
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt)
{
    // Recuperer les references vers les queues inter-threads.
    NetworkINToSimulationQueue& networkInToSimulationQueue = GetNetworkINToSimulationQueue();
    SimulationToNetworkOUTQueue& simulationToNetworkOUTQueue = GetSimulationToNetworkOUTQueue();
    SimulationToHttpQueue& simulationToHttpQueue = GetSimulationToHttpQueue();
    SimulationToNatsQueue& simulationToNatsQueue = GetSimulationToNatsQueue();
    HttpToSimulationQueue& httpToSimulationQueue = GetHttpToSimulationQueue();
    NatsToSimulationQueue& natsToSimulationQueue = GetNatsToSimulationQueue();

    // Preparer la deque locale qui recevra les messages reseau entrants draines.
    std::deque<NetworkINToSimulationMessage> networkInToSimulationMessages;

    // Drainer la queue reseau -> simulation.
    ServerSimulation_DrainNetworkIncomingToSimulationMessages(
        networkInToSimulationQueue,
        networkInToSimulationMessages);

    // Preparer la deque locale qui recevra les messages HTTP entrants draines.
    std::deque<HttpToSimulationMessage> httpToSimulationMessages;

    // Drainer la queue HTTP -> simulation.
    ServerSimulation_DrainHttpToSimulationMessages(
        httpToSimulationQueue,
        httpToSimulationMessages);

    // Preparer la deque locale qui recevra les messages NATS entrants draines.
    std::deque<NatsToSimulationMessage> natsToSimulationMessages;

    // Drainer la queue NATS -> simulation.
    ServerSimulation_DrainNatsToSimulationMessages(
        natsToSimulationQueue,
        natsToSimulationMessages);

    // Recuperer l'etat global du jeu.
    GameState& gameState = GetGameState();

    // Recuperer l'etat global du reseau.
    NetworkState& networkState = GetNetworkState();

    // Preparer le payload enrichi qui sera potentiellement publie vers NATS.
    GameserverSimulationEtatMetricsPayload gameserverSimulationEtatMetricsPayload{};
    gameserverSimulationEtatMetricsPayload.server_id = networkState.serverId;

    // Helper local: convertit une duree chrono en millisecondes (double).
    const auto durationToMs =
        [](const std::chrono::steady_clock::duration& duration) -> double
        {
            return std::chrono::duration<double, std::milli>(duration).count();
        };

    // Traiter tous les messages entrants provenant du thread reseau.
    const std::chrono::steady_clock::time_point networkIncomingDispatcherBegin = std::chrono::steady_clock::now();
    ServerSimulation_ProcessNetworkIncomingDispatcher(
        networkState,
        simulationToNetworkOUTQueue,
        simulationToHttpQueue,
        networkInToSimulationMessages);
    const std::chrono::steady_clock::time_point networkIncomingDispatcherEnd = std::chrono::steady_clock::now();
    gameserverSimulationEtatMetricsPayload.temps_reel_traitement_network_incoming_dispatcher_sur_ce_tick_ms =
        durationToMs(networkIncomingDispatcherEnd - networkIncomingDispatcherBegin);

    // Traiter tous les messages entrants provenant du thread HTTP.
    const std::chrono::steady_clock::time_point httpDispatcherBegin = std::chrono::steady_clock::now();
    ServerSimulation_ProcessHttpDispatcher(
        networkState,
        simulationToNetworkOUTQueue,
        httpToSimulationMessages);
    const std::chrono::steady_clock::time_point httpDispatcherEnd = std::chrono::steady_clock::now();
    gameserverSimulationEtatMetricsPayload.temps_reel_traitement_http_dispatcher_sur_ce_tick_ms =
        durationToMs(httpDispatcherEnd - httpDispatcherBegin);

    // Traiter tous les messages entrants provenant du thread NATS.
    const std::chrono::steady_clock::time_point natsDispatcherBegin = std::chrono::steady_clock::now();
    ServerSimulation_ProcessNatsDispatcher(
        networkState,
        simulationToNetworkOUTQueue,
        natsToSimulationMessages);
    const std::chrono::steady_clock::time_point natsDispatcherEnd = std::chrono::steady_clock::now();
    gameserverSimulationEtatMetricsPayload.temps_reel_traitement_nats_dispatcher_sur_ce_tick_ms =
        durationToMs(natsDispatcherEnd - natsDispatcherBegin);

    // Gerer le flow global de match.
    const std::chrono::steady_clock::time_point checkMatchFlowBegin = std::chrono::steady_clock::now();
    ServerSimulation_CheckMatchFlow(
        gameState,
        networkState,
        simulationToNetworkOUTQueue,
        currentTick);
    const std::chrono::steady_clock::time_point checkMatchFlowEnd = std::chrono::steady_clock::now();
    gameserverSimulationEtatMetricsPayload.temps_reel_traitement_check_match_flow_sur_ce_tick_ms =
        durationToMs(checkMatchFlowEnd - checkMatchFlowBegin);

    // Simuler le monde gameplay pour le tick courant.
    const std::chrono::steady_clock::time_point worldSimulateBegin = std::chrono::steady_clock::now();
    ServerWorld_Simulate(
        gameState,
        currentTick,
        serverTimeNs,
        dtNs,
        dt);
    const std::chrono::steady_clock::time_point worldSimulateEnd = std::chrono::steady_clock::now();
    gameserverSimulationEtatMetricsPayload.temps_reel_traitement_world_simulate_sur_ce_tick_ms =
        durationToMs(worldSimulateEnd - worldSimulateBegin);

    // Produire les snapshots au rythme maximal du thread reseau sortant.
    if (rcnet_engine_isNetworkOutgoingProductionTick(currentTick, rcnet_engine_getNetworkOutgoingTickRateHz()))
    {
        const std::chrono::steady_clock::time_point createFullSnapshotsBegin = std::chrono::steady_clock::now();
        ServerSimulation_CreateFullSnapshotsForAllSessionsAndEnqueueForNetworkOutgoing(
            simulationToNetworkOUTQueue,
            networkState,
            currentTick);
        const std::chrono::steady_clock::time_point createFullSnapshotsEnd = std::chrono::steady_clock::now();
        gameserverSimulationEtatMetricsPayload.temps_reel_traitement_create_full_snapshots_sur_ce_tick_ms =
            durationToMs(createFullSnapshotsEnd - createFullSnapshotsBegin);
    }
    else
    {
        // Ce bloc n'a pas ete execute sur ce tick.
        gameserverSimulationEtatMetricsPayload.temps_reel_traitement_create_full_snapshots_sur_ce_tick_ms = 0.0;
    }

    // Publier les metrics simulation vers NATS selon une cadence temps reel.
    /*ServerSimulation_PublishGameserverSimulationEtatMetricsToNatsIfNeeded(
        serverTimeNs,
        gameserverSimulationEtatMetricsPayload,
        simulationToNatsQueue);*/

    // Produire les messages de synchronisation d'horloge au rythme de 3 Hz.
    /*if (rcnet_engine_isNetworkOutgoingProductionTick(currentTick, 3))
    {
        ServerSimulation_CreateServerClockSyncMessagesAndEnqueue(
            simulationToNetworkOUTQueue,
            networkState,
            currentTick);
    }*/
}
