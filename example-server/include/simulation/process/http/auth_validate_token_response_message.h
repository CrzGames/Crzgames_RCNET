#pragma once

#include "network/state.h"
#include "core/threading/queues/http_to_simulation.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Traite une réponse HTTP de validation de token d'authentification.
 *
 * Cette fonction met à jour l'état d'authentification de la session concernée
 * puis construit la réponse réseau fiable à renvoyer au client.
 *
 * @param networkState État réseau global du serveur.
 * @param simToNetQueue Queue simulation -> réseau utilisée pour envoyer
 *        la réponse d'authentification au client.
 * @param httpMessage Message HTTP entrant contenant le résultat de validation.
 */
void ServerSimulation_ProcessHttpDispatcher_HandleAuthValidateTokenResponseMessage(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const HttpToSimulationMessage& httpMessage);