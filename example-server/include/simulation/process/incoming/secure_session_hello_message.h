#pragma once

#include "network/state.h"
#include "core/threading/queues/network_incoming_to_simulation.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Traite un message de demande d'établissement de session sécurisée.
 *
 * Cette fonction récupère la clé publique du client, tente de calculer les
 * clés de session serveur, met à jour l'état crypto de la session puis prépare
 * la réponse fiable à renvoyer au client.
 *
 * @param networkState État réseau global du serveur.
 * @param simToNetQueue Queue simulation -> réseau utilisée pour envoyer
 *        la réponse d'établissement de session sécurisée.
 * @param msg Message réseau entrant transportant le hello sécurisé client.
 */
void ServerSimulation_ProcessNetworkIncomingDispatcher_HandleSecureSessionHelloMessage(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const NetworkINToSimulationMessage& msg);