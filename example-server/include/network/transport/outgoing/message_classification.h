#pragma once

#include <cstdint>       // uint32_t
#include <deque>         // std::deque
#include <unordered_map> // std::unordered_map

#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Classe les messages sortants produits par la simulation.
 *
 * Les messages reliable sont conservés dans l'ordre de production.
 * Les messages unreliable sont coalescés par connectionId afin de ne garder
 * que le dernier message pertinent pour chaque connexion pendant le tick
 * réseau sortant courant.
 *
 * @param outMessages Ensemble des messages drainés depuis la queue simulation -> réseau.
 * @param reliableMessages Deque de sortie recevant tous les messages reliable à envoyer.
 * @param lastUnreliablePerConnectionId Table de sortie recevant le dernier message
 *        unreliable retenu pour chaque connectionId.
 */
void ServerNetworkOutgoing_ClassifyOutgoingMessages(
    const std::deque<SimulationToNetworkOUTMessage>& outMessages,
    std::deque<SimulationToNetworkOUTMessage>& reliableMessages,
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastUnreliablePerConnectionId);