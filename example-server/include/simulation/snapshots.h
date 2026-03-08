#pragma once

#include <cstdint>

#include "network/client_session.h"
#include "network/state.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Indique si le tick courant est un tick autorisé pour l'envoi de snapshots.
 *
 * Cette fonction applique le gating snapshots selon le ratio entre
 * la fréquence de simulation et la fréquence du thread réseau sortant.
 *
 * @param currentTick Tick courant de simulation.
 *
 * @return `true` si des snapshots peuvent être construits et envoyés à ce tick,
 *         `false` sinon.
 */
bool ServerSimulationUpdate_IsSnapshotSendTick(uint64_t currentTick);

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