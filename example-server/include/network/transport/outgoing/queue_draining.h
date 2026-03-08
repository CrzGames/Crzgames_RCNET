#pragma once

#include <deque> // std::deque

#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Draine la queue simulation -> réseau sortant vers un conteneur local.
 *
 * Cette fonction vide la queue thread-safe alimentée par la simulation
 * et copie les messages extraits dans une deque locale afin qu'ils puissent
 * être classifiés puis envoyés pendant le tick réseau sortant courant.
 *
 * @param simToNetQueue Queue thread-safe simulation -> réseau sortant.
 * @param outMessages Deque de sortie recevant les messages drainés.
 */
void ServerNetworkOutgoingUpdate_DrainOutgoingMessages(
    SimulationToNetworkOUTQueue& simToNetQueue,
    std::deque<SimulationToNetworkOUTMessage>& outMessages);