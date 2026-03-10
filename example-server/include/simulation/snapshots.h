#pragma once

#include <cstdint>

#include "simulation/session/client.h"
#include "network/state.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Construit les snapshots pour toutes les sessions puis les ajoute à la queue réseau sortante.
 *
 * @param simToNetQueue Queue simulation -> réseau utilisée pour l'envoi des snapshots.
 * @param networkState État réseau global du serveur.
 * @param currentTick Tick courant de simulation.
 */
void ServerSimulation_CreateFullSnapshotsForAllSessionsAndEnqueueForNetworkOutgoing(
    SimulationToNetworkOUTQueue& simToNetQueue,
    const NetworkState& networkState,
    uint64_t currentTick);
