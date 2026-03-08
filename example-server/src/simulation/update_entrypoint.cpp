#include "simulation/update_entrypoint.h"

#include "core/context.h"
#include "game/world/update_entrypoint.h"
#include "simulation/queue_draining.h"
#include "simulation/process/network_incoming_messages.h"
#include "simulation/process/http_messages.h"
#include "simulation/match_flow.h"
#include "simulation/snapshots.h"

#include <deque>

void ServerSimulationUpdate_RunFullSimulationPipelineForCurrentTick(
    uint64_t currentTick,
    uint64_t serverTimeNs,
    uint64_t dtNs,
    double dt)
{
    // Récupérer les références vers les queues inter-threads.
    NetworkINToSimulationQueue& networkInToSimulationQueue = GetNetworkINToSimulationQueue();
    SimulationToNetworkOUTQueue& simulationToNetworkOUTQueue = GetSimulationToNetworkOUTQueue();
    SimulationToHttpQueue& simulationToHttpQueue = GetSimulationToHttpQueue();
    HttpToSimulationQueue& httpToSimulationQueue = GetHttpToSimulationQueue();

    // Préparer la deque locale qui recevra les messages réseau entrants drainés.
    std::deque<NetworkINToSimulationMessage> networkInToSimulationMessages;

    // Drainer la queue réseau -> simulation.
    ServerSimulationUpdate_DrainNetworkIncomingMessages(
        networkInToSimulationQueue,
        networkInToSimulationMessages);

    // Préparer la deque locale qui recevra les messages HTTP entrants drainés.
    std::deque<HttpToSimulationMessage> httpToSimulationMessages;

    // Drainer la queue HTTP -> simulation.
    ServerSimulationUpdate_DrainHttpIncomingMessages(
        httpToSimulationQueue,
        httpToSimulationMessages);

    // Récupérer l'état global du jeu.
    GameState& gameState = GetGameState();

    // Récupérer l'état global du réseau.
    NetworkState& networkState = GetNetworkState();

    // Traiter tous les messages entrants provenant du thread réseau.
    ServerSimulationUpdate_ProcessNetworkIncomingMessages(
        networkState,
        simulationToNetworkOUTQueue,
        simulationToHttpQueue,
        networkInToSimulationMessages);

    // Traiter tous les messages entrants provenant du thread HTTP.
    ServerSimulationUpdate_ProcessHttpMessages(
        networkState,
        simulationToNetworkOUTQueue,
        httpToSimulationMessages);

    // Gérer le flow global de match.
    ServerSimulationUpdate_CheckMatchFlow(
        gameState,
        networkState,
        simulationToNetworkOUTQueue,
        currentTick);

    // Simuler le monde gameplay pour le tick courant.
    ServerWorld_Simulate(
        gameState,
        currentTick,
        serverTimeNs,
        dtNs,
        dt);

    // Produire les snapshots au rythme maximal du thread réseau sortant.
    if (ServerSimulationUpdate_IsNetworkOutgoingProductionTick(currentTick, rcnet_engine_getNetworkOutgoingTickRateHz()))
    {
        ServerSimulationUpdate_BuildSnapshotsForAllSessionsAndEnqueue(
            simulationToNetworkOUTQueue,
            networkState,
            currentTick);
    }

    // Produire les messages de synchronisation d'horloge au rythme de 3 Hz.
    /*if (ServerSimulationUpdate_IsNetworkOutgoingProductionTick(currentTick, 3))
    {
        ServerSimulationUpdate_BuildServerClockSyncMessagesAndEnqueue(
            simulationToNetworkOUTQueue,
            networkState,
            currentTick);
    }*/
}