#pragma once

#include "network/state.h"
#include "core/threading/queues/network_incoming_to_simulation.h"

/**
 * @brief Traite un message d'input gameplay provenant du thread réseau entrant.
 *
 * Cette fonction met à jour le dernier input reçu pour la session concernée,
 * met à jour les informations d'ack snapshot côté client et, si l'input est
 * nouveau, l'ajoute à la file d'inputs en attente de consommation gameplay.
 *
 * @param networkState État réseau global du serveur.
 * @param msg Message réseau entrant transportant un input client.
 */
void ServerSimulation_ProcessNetworkIncomingDispatcher_HandleInputMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& msg);