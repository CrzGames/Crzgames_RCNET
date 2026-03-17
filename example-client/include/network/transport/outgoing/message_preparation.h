#pragma once

#include <cstdint>       // uint32_t
#include <deque>         // std::deque
#include <unordered_map> // std::unordered_map

#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Regroupe les messages sortants préparés pour le tick réseau sortant courant.
 *
 * Cette structure contient les messages issus de la simulation après préparation
 * pour le tick réseau sortant courant.
 *
 * Politique de conservation :
 * - les messages reliable sont tous conservés dans leur ordre de production ;
 * - certains messages unreliable sont coalescés par famille, en ne gardant
 *   que le dernier message pertinent par connectionId.
 *
 * Contenu actuel :
 * - `reliableMessages` :
 *   tous les messages reliable à envoyer dans l'ordre ;
 */
struct ClientNetworkOutgoingPreparedMessages
{
    std::deque<SimulationToNetworkOUTMessage> reliableMessages;
    SimulationToNetworkOUTMessage lastInputUnreliable;
    SimulationToNetworkOUTMessage lastClockSyncUnreliable;
};

void ClientNetworkOutgoing_SplitReliableAndCoalesceUnreliableMessages(
    const std::deque<SimulationToNetworkOUTMessage>& simToNetOutMessages,
    ClientNetworkOutgoingPreparedMessages& preparedMessages);