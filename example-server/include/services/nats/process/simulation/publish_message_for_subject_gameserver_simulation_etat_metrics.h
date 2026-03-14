#pragma once

#include "core/threading/queues/simulation_to_nats.h" // SimulationToNatsMessage

#include <RCNET/RCNET.h> // RCNET_NATSContext

/**
 * @brief Handle le message simulation -> NATS de publication des metrics simulation.
 *
 * Le handler:
 * - lit SimulationToNatsMessage::gameserverSimulationEtatMetricsPayload,
 * - construit un JSON conforme aux noms des proprietes du snapshot moteur,
 * - publie ce JSON sur SimulationToNatsMessage::subject.
 *
 * @param natsContext Contexte client NATS du thread NATS.
 * @param message Message entrant depuis la queue Simulation -> NATS.
 */
void ServerNats_ProcessSimulationDispatcher_HandlePublishMessageForSubjectGameserverSimulationEtatMetrics(
    RCNET_NATSContext* natsContext,
    const SimulationToNatsMessage& message);
