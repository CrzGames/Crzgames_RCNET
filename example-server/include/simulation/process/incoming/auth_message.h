#pragma once

#include "network/state.h"
#include "core/threading/queues/network_incoming_to_simulation.h"
#include "core/threading/queues/simulation_to_http.h"

/**
 * @brief Traite un message d'authentification provenant du thread réseau entrant.
 *
 * Cette fonction vérifie l'état d'authentification courant de la session,
 * prépare une requête de validation backend si nécessaire, puis l'envoie
 * au thread HTTP.
 *
 * @param networkState État réseau global du serveur.
 * @param simToHttpQueue Queue simulation -> HTTP utilisée pour demander
 *        la validation du token d'authentification.
 * @param msg Message réseau entrant transportant un token d'authentification.
 */
void ServerSimulationUpdate_ProcessNetworkIncomingDispatcher_HandleAuthMessage(
    NetworkState& networkState,
    SimulationToHttpQueue& simToHttpQueue,
    const NetworkINToSimulationMessage& msg);