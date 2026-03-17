#pragma once

#include "network/state.h"
#include "core/threading/queues/network_incoming_to_simulation.h"

/**
 * @brief Traite un message de connexion provenant du thread réseau entrant.
 *
 * Cette fonction crée une nouvelle session côté simulation pour la connexion
 * concernée et initialise son état réseau de base.
 *
 * @param networkState État réseau global du serveur.
 * @param networkInToSimMessage Message réseau entrant de type connexion.
 */
void ClientSimulation_ProcessNetworkIncomingDispatcher_HandleConnectMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& networkInToSimMessage);