#pragma once

#include <cstdint>       // uint32_t
#include <unordered_map> // std::unordered_map

#include "network/state.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Traite les messages unreliable issus de la simulation.
 *
 * Cette fonction reçoit les messages unreliable coalescés par connectionId,
 * applique les patchs de dernière minute nécessaires (ex: assignation d'un
 * identifiant de snapshot au moment de l'envoi), puis envoie les packets
 * correspondants à chaque client.
 *
 * @param networkState État réseau global du serveur, utilisé pour accéder aux sessions clients.
 * @param lastSnapshotFullUnreliablePerConnectionId Dernier message snapshot full unreliable retenu
 *        pour chaque connectionId, à traiter et envoyer.
 * @param lastClockSyncUnreliablePerConnectionId Dernier message clock sync unreliable retenu
 *        pour chaque connectionId, à traiter et envoyer.
 */
void ServerNetworkOutgoing_ProcessSimulationDispatcher_HandleUnreliableMessages(
    NetworkState& networkState,
    const std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastSnapshotFullUnreliablePerConnectionId,
    const std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastClockSyncUnreliablePerConnectionId);
