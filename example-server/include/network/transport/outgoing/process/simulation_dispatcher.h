#pragma once

#include <cstdint>       // uint32_t
#include <deque>         // std::deque
#include <unordered_map> // std::unordered_map

#include "network/state.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"
#include "network/transport/outgoing/message_preparation.h"

/**
 * @brief Dispatch le traitement des messages simulation -> réseau sortant.
 *
 * Cette fonction reçoit les messages déjà classifiés entre reliable et
 * unreliable, puis délègue leur traitement aux handlers spécialisés
 * correspondants.
 *
 * @param networkState État réseau global du serveur.
 * @param reliableMessages Messages reliable à traiter.
 * @param lastUnreliablePerConnectionId Dernier message unreliable retenu
 *        pour chaque connectionId.
 */
void ServerNetworkOutgoing_ProcessSimulationDispatcher(
    NetworkState& networkState,
    const ServerNetworkOutgoingPreparedMessages& preparedMessages);
