#pragma once

#include "network/state.h"
#include "core/threading/queues/network_incoming_to_simulation.h"

/**
 * @brief Traite un message "ready for match" provenant du thread réseau entrant.
 *
 * Cette fonction marque la session concernée comme prête à démarrer le match.
 *
 * @param networkState État réseau global du serveur.
 * @param msg Message réseau entrant de type ready for match.
 */
void ServerSimulation_ProcessNetworkIncomingDispatcher_HandleReadyForMatchMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& msg);