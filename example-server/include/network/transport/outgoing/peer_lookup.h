#pragma once

#include <cstdint> // uint32_t

#include <rcenet/RCENET_enet.h> // ENetPeer

#include "network/state.h"

/**
 * @brief Résout le peer ENet associé à une connectionId.
 *
 * Cette fonction recherche le peer ENet correspondant à une connexion active
 * dans l'état réseau global. Elle retourne `nullptr` si la connexion n'existe
 * pas ou si le peer associé est invalide.
 *
 * @param networkState État réseau global du serveur.
 * @param connectionId Identifiant de connexion recherché.
 *
 * @return Le `ENetPeer*` correspondant si trouvé et valide, sinon `nullptr`.
 */
ENetPeer* ServerNetworkOutgoing_FindPeerByConnectionId(
    const NetworkState& networkState,
    uint32_t connectionId);
