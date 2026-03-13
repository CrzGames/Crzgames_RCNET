#pragma once

#include <deque> // std::deque

#include "network/state.h"
#include "core/threading/queues/nats_to_simulation.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Traite les messages entrants provenant du thread NATS.
 *
 * Cette fonction parcourt les messages déjà drainés depuis la queue
 * NATS -> simulation puis dispatch le traitement en fonction du type
 * de message reçu.
 *
 * @param networkState État réseau global du serveur.
 * @param simToNetQueue Queue simulation -> réseau utilisée pour préparer
 *        les réponses réseau envoyées aux clients.
 * @param natsMessages Messages entrants NATS déjà drainés pour le tick courant.
 */
void ServerSimulation_ProcessNatsDispatcher(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const std::deque<NatsToSimulationMessage>& natsMessages);
