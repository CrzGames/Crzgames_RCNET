#pragma once

#include <deque> // std::deque

#include "network/state.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Envoie tous les messages reliable prêts à partir vers les peers ENet.
 *
 * Cette fonction résout le peer cible pour chaque message reliable puis appelle
 * la routine d'envoi ENet adaptée au type exact du packet fiable.
 *
 * @param networkState État réseau global du serveur.
 * @param reliableMessages Messages reliable à envoyer pendant ce tick réseau sortant.
 */
void ServerNetworkOutgoing_SendReliableMessages(
    NetworkState& networkState,
    const std::deque<SimulationToNetworkOUTMessage>& reliableMessages);