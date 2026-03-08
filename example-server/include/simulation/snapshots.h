#pragma once

#include <cstdint>

#include "network/client_session.h"
#include "network/state.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Construit un snapshot full pour une session puis l'ajoute à la queue réseau sortante.
 *
 * @param simToNetQueue Queue simulation -> réseau utilisée pour l'envoi des snapshots.
 * @param session Session destinataire du snapshot.
 * @param currentTick Tick courant de simulation.
 */
void ServerSimulationUpdate_CreateFullSnapshotAndEnqueue(
    SimulationToNetworkOUTQueue& simToNetQueue,
    ClientSession& session,
    uint64_t currentTick);

/**
 * @brief Construit les snapshots pour toutes les sessions puis les ajoute à la queue réseau sortante.
 *
 * @param simToNetQueue Queue simulation -> réseau utilisée pour l'envoi des snapshots.
 * @param networkState État réseau global du serveur.
 * @param currentTick Tick courant de simulation.
 */
void ServerSimulationUpdate_BuildSnapshotsForAllSessionsAndEnqueue(
    SimulationToNetworkOUTQueue& simToNetQueue,
    NetworkState& networkState,
    uint64_t currentTick);