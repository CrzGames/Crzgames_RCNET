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
 * @param networkState État réseau global du client, utilisé pour accéder aux sessions clients.
 * @param lastInputUnreliable Dernier message input unreliable retenu
 *        pour chaque connectionId, à traiter et envoyer.
 * @param lastClockSyncUnreliable Dernier message clock sync unreliable retenu
 *        pour chaque connectionId, à traiter et envoyer.
 */
void ClientNetworkOutgoing_ProcessSimulationDispatcher_HandleUnreliableMessages(
    NetworkState& networkState,
    const std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastInputUnreliable,
    const std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastClockSyncUnreliable);
