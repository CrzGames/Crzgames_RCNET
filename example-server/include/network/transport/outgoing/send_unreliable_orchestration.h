#pragma once

#include <unordered_map> // std::unordered_map
#include <cstdint>       // uint32_t

#include "network/state.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Envoie les messages unreliable retenus pour chaque connexion.
 *
 * Cette fonction résout le peer cible pour chaque message unreliable retenu,
 * applique si nécessaire les patchs de dernière minute avant envoi
 * (par exemple pour les snapshots), puis délègue l'émission réelle à ENet.
 *
 * @param networkState État réseau global du serveur.
 * @param lastUnreliablePerConnectionId Dernier message unreliable retenu par connexion.
 */
void ServerNetworkOutgoing_SendUnreliableMessages(
    NetworkState& networkState,
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastUnreliablePerConnectionId);