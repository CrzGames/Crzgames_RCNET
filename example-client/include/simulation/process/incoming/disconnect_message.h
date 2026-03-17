#pragma once

#include "network/state.h"
#include "core/threading/queues/network_incoming_to_simulation.h"

/**
 * @brief Traite un message de déconnexion provenant du thread réseau entrant.
 *
 * Cette fonction supprime la session associée à la connexion si elle existe
 * encore dans l'état réseau global.
 *
 * @param networkState État réseau global du serveur.
 * @param networkInToSimMessage Message réseau entrant de type déconnexion.
 */
void ClientSimulation_ProcessNetworkIncomingDispatcher_HandleDisconnectMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& networkInToSimMessage);