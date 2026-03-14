#pragma once

#include "core/threading/queues/simulation_to_nats.h" // SimulationToNatsQueue

#include <cstdint> // uint64_t

/**
 * @brief Publie vers la queue Simulation -> NATS un snapshot des metrics simulation.
 *
 * Cette fonction:
 * - cadence la publication a 1 Hz en temps reel (base serverTimeNs),
 * - injecte le dernier snapshot moteur via rcnet_engine_getLastSimulationEtatMetrics(),
 * - construit un message type vers le sujet
 *   "gameserver.simulation.etat.metrics",
 * - pousse ce message dans la queue du thread NATS.
 *
 * @param serverTimeNs Temps monotone serveur (ns) du tick courant.
 * @param payloadFromCurrentTick Payload enrichi mesure sur le tick courant.
 * @param simToNatsQueue Queue thread-safe simulation -> NATS.
 */
void ServerSimulation_PublishGameserverSimulationEtatMetricsToNatsIfNeeded(
    uint64_t serverTimeNs,
    const GameserverSimulationEtatMetricsPayload& payloadFromCurrentTick,
    SimulationToNatsQueue& simToNatsQueue);
