#pragma once

#include <cstdint>       // uint32_t
#include <deque>         // std::deque
#include <unordered_map> // std::unordered_map

#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Structure regroupant les messages sortants préparés pour le tick réseau sortant courant.
 *
 * Cette structure contient :
 * - une deque de messages reliable à envoyer dans l'ordre de production,
 * - une table de coalescing pour les messages unreliable de type snapshot full,
 *   ne gardant que le dernier message pertinent par connectionId,
 * - une table de coalescing pour les messages unreliable de type clock sync,
 *   ne gardant que le dernier message pertinent par connectionId.
 */
struct ServerNetworkOutgoingPreparedMessages
{
    std::deque<SimulationToNetworkOUTMessage> reliableMessages;
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage> lastSnapshotPerConnectionId;
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage> lastClockSyncPerConnectionId;
};

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
void ServerNetworkOutgoing_SplitReliableAndCoalesceUnreliableMessages(
    const std::deque<SimulationToNetworkOUTMessage>& outMessages,
    ServerNetworkOutgoingPreparedMessages& preparedMessages);