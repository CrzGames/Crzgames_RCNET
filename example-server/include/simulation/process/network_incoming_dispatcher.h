#pragma once

#include <deque> // std::deque

#include "network/state.h"
#include "core/threading/queues/network_incoming_to_simulation.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"
#include "core/threading/queues/simulation_to_http.h"

/**
 * @brief Traite les messages entrants provenant du thread réseau.
 *
 * Cette fonction parcourt les messages déjà drainés depuis la queue
 * réseau -> simulation puis dispatch le traitement en fonction du type
 * de message reçu.
 *
 * @param networkState État réseau global du serveur.
 * @param simToNetQueue Queue simulation -> réseau utilisée pour préparer
 *        les réponses à envoyer aux clients.
 * @param simToHttpQueue Queue simulation -> HTTP utilisée pour déléguer
 *        certaines opérations backend, comme la validation de token.
 * @param messages Messages entrants réseau déjà drainés pour le tick courant.
 */
void ServerSimulation_ProcessNetworkIncomingDispatcher(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    SimulationToHttpQueue& simToHttpQueue,
    const std::deque<NetworkINToSimulationMessage>& messages);
