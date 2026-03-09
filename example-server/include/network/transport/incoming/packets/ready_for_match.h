#pragma once

#include <cstdint> // uint32_t

#include <rcenet/RCENET_enet.h> // ENetEvent

#include "core/threading/queues/network_incoming_to_simulation.h"

/**
 * @brief Traite un paquet gameplay reliable de type "ready for match".
 *
 * Ce handler désérialise le paquet `ClientReadyForMatchPacketReliable`,
 * vérifie qu'il est valide, puis construit le message réseau -> simulation
 * correspondant avant de l'envoyer à la queue de simulation.
 *
 * @param event Événement ENet de réception contenant le paquet brut.
 * @param connectionId Identifiant unique de la connexion déjà validé en amont.
 * @param netToSimQueue Queue thread-safe utilisée pour transférer le message
 *        du thread réseau vers le thread simulation.
 */
void ServerNetworkIncoming_HandlePacket_ReadyForMatch(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue);