#pragma once

#include <cstdint> // uint32_t

#include <rcenet/RCENET_enet.h> // ENetEvent

#include "core/threading/queues/network_incoming_to_simulation.h"

/**
 * @brief Traite un événement ENet reçu sur le channel gameplay unreliable.
 *
 * Ce handler lit le type de paquet unreliable envoyé par le client,
 * puis redirige le traitement vers le handler de paquet gameplay unreliable
 * approprié.
 *
 * @param event Événement ENet de réception contenant le paquet brut.
 * @param connectionId Identifiant unique de la connexion déjà validé en amont.
 * @param netToSimQueue Queue thread-safe utilisée pour transférer le message
 *        du thread réseau vers le thread simulation.
 */
void ServerNetworkIncoming_Channel_GameUnreliable(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue);