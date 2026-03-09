#pragma once

#include <rcenet/RCENET_enet.h> // ENetEvent

#include "network/packets/client/reliable.h"
#include "network/packets/client/unreliable.h"

/**
 * @brief Lit le type d'un paquet client reliable depuis le payload ENet.
 *
 * Cette fonction lit le premier octet du buffer réseau associé à l'événement
 * ENet et l'interprète comme une valeur de `ClientReliablePacketType`.
 *
 * @param event Événement ENet de réception contenant le paquet brut.
 * @param outType Paramètre de sortie recevant le type de paquet lu si la lecture réussit.
 *
 * @return `true` si le type de paquet a pu être lu correctement,
 *         `false` si le buffer est vide, invalide ou trop court.
 */
bool ServerNetworkIncoming_ReadClientReliablePacketType(
    const ENetEvent* event,
    ClientReliablePacketType& outType);

/**
 * @brief Lit le type d'un paquet client unreliable depuis le payload ENet.
 *
 * Cette fonction lit le premier octet du buffer réseau associé à l'événement
 * ENet et l'interprète comme une valeur de `ClientUnreliablePacketType`.
 *
 * @param event Événement ENet de réception contenant le paquet brut.
 * @param outType Paramètre de sortie recevant le type de paquet lu si la lecture réussit.
 *
 * @return `true` si le type de paquet a pu être lu correctement,
 *         `false` si le buffer est vide, invalide ou trop court.
 */
bool ServerNetworkIncoming_ReadClientUnreliablePacketType(
    const ENetEvent* event,
    ClientUnreliablePacketType& outType);