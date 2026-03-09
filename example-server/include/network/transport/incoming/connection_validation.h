#pragma once

#include <rcenet/RCENET_enet.h> // ENetEvent

#include "network/state.h"

/**
 * @brief Valide la connexion associée à un événement ENet et retourne son connectionId.
 *
 * Cette fonction vérifie notamment :
 * - que `event->peer` est non nul ;
 * - que `event->peer->data` contient bien un identifiant ;
 * - que cet identifiant est valide ;
 * - que le mapping `connectionId -> ENetPeer*` dans l'état réseau est cohérent.
 *
 * @param event Événement ENet à valider.
 * @param networkState État réseau global utilisé pour vérifier la cohérence
 *        entre le peer ENet et la table des connexions actives.
 *
 * @return L'identifiant de connexion validé si tout est cohérent,
 *         ou `0` si la validation échoue.
 */
uint32_t ServerNetworkIncoming_GetValidatedConnectionIdOrZero(
    const ENetEvent* event,
    const NetworkState& networkState);