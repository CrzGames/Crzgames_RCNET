#pragma once

#include <rcenet/RCENET_enet.h> // ENetEvent

#include "network/state.h"
#include "core/threading/queues/network_incoming_to_simulation.h"

/**
 * @brief Traite un événement ENet de type réception de paquet.
 *
 * Cette fonction valide d'abord la connexion source associée à l'événement,
 * récupère le `connectionId`, puis délègue ensuite le traitement au système
 * de dispatch par channel.
 *
 * @param event Événement ENet de type receive.
 * @param networkState État réseau global utilisé pour valider la connexion
 *        source de l'événement.
 * @param netToSimQueue Queue thread-safe utilisée pour transférer les messages
 *        du thread réseau vers le thread simulation.
 */
void ServerNetworkIncoming_Event_HandleReceive(
    const ENetEvent* event,
    const NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue);