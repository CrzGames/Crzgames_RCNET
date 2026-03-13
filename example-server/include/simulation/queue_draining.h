#pragma once

#include <deque> // std::deque

#include "core/threading/queues/network_incoming_to_simulation.h"
#include "core/threading/queues/http_to_simulation.h"
#include "core/threading/queues/nats_to_simulation.h"

/**
 * @brief Draine la queue réseau -> simulation vers un conteneur local.
 *
 * Cette fonction vide la queue thread-safe alimentée par le thread réseau
 * entrant et copie les messages extraits dans une deque locale afin qu'ils
 * puissent être traités dans le tick courant de simulation.
 *
 * @param netToSimQueue Queue thread-safe réseau -> simulation.
 * @param outMessages Deque de sortie recevant les messages drainés.
 */
void ServerSimulation_DrainNetworkIncomingToSimulationMessages(
    NetworkINToSimulationQueue& netToSimQueue,
    std::deque<NetworkINToSimulationMessage>& outMessages);

/**
 * @brief Draine la queue HTTP -> simulation vers un conteneur local.
 *
 * Cette fonction vide la queue thread-safe alimentée par le thread HTTP
 * et copie les messages extraits dans une deque locale afin qu'ils puissent
 * être traités dans le tick courant de simulation.
 *
 * @param httpToSimQueue Queue thread-safe HTTP -> simulation.
 * @param outMessages Deque de sortie recevant les messages drainés.
 */
void ServerSimulation_DrainHttpToSimulationMessages(
    HttpToSimulationQueue& httpToSimQueue,
    std::deque<HttpToSimulationMessage>& outMessages);

/**
 * @brief Draine la queue NATS -> simulation vers un conteneur local.
 *
 * Cette fonction vide la queue thread-safe alimentée par le thread NATS
 * et copie les messages extraits dans une deque locale afin qu'ils puissent
 * être traités dans le tick courant de simulation.
 *
 * @param natsToSimQueue Queue thread-safe NATS -> simulation.
 * @param outMessages Deque de sortie recevant les messages drainés.
 */
void ServerSimulation_DrainNatsToSimulationMessages(
    NatsToSimulationQueue& natsToSimQueue,
    std::deque<NatsToSimulationMessage>& outMessages);
