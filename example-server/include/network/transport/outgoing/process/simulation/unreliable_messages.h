#pragma once

#include <cstdint>       // uint32_t
#include <unordered_map> // std::unordered_map

#include "network/state.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Traite et envoie les messages unreliable produits par la simulation.
 *
 * Cette fonction parcourt les derniers messages unreliable retenus pour le tick
 * courant, résout le peer cible et applique si nécessaire des patchs de dernière
 * minute avant l'envoi effectif via ENet.
 *
 * @param networkState État réseau global du serveur.
 * @param lastUnreliablePerConnectionId Dernier message unreliable retenu pour
 *        chaque connectionId.
 */
void ServerNetworkOutgoing_ProcessSimulationDispatcher_HandleUnreliableMessages(
    NetworkState& networkState,
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastUnreliablePerConnectionId);