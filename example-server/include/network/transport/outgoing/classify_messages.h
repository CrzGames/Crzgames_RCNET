#pragma once

#include <deque>         // std::deque
#include <unordered_map> // std::unordered_map
#include <cstdint>       // uint32_t

#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Classe les messages sortants en lots reliable et unreliable.
 *
 * Les messages reliable sont conservés tels quels et envoyés dans l'ordre.
 * Les messages unreliable sont coalescés par connectionId afin de ne garder
 * que le dernier message pertinent pour chaque client pendant le tick courant.
 *
 * @param outMessages Ensemble des messages sortants drainés depuis la queue simulation.
 * @param reliableMessages Deque de sortie recevant tous les messages reliable à envoyer.
 * @param lastUnreliablePerConnectionId Table de sortie recevant le dernier message unreliable
 *        à conserver pour chaque connectionId.
 */
void ServerNetworkOutgoingUpdate_ClassifyOutgoingMessages(
    const std::deque<SimulationToNetworkOUTMessage>& outMessages,
    std::deque<SimulationToNetworkOUTMessage>& reliableMessages,
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastUnreliablePerConnectionId);