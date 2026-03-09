#pragma once

#include <cstdint> // uint32_t

/**
 * @brief Indique si une connexion est autorisée à envoyer sur le channel d'authentification.
 *
 * Une connexion est autorisée sur le channel auth si :
 * - la couche transport est connectée ;
 * - la session sécurisée est établie.
 *
 * @param connectionId Identifiant unique de la connexion à vérifier.
 *
 * @return `true` si la connexion peut utiliser le channel d'authentification,
 *         `false` sinon.
 */
bool ServerNetworkIncoming_IsConnectionAllowedForAuthChannel(uint32_t connectionId);

/**
 * @brief Indique si une connexion est autorisée à envoyer sur les channels gameplay.
 *
 * Une connexion est autorisée sur les channels gameplay si :
 * - la couche transport est connectée ;
 * - la session sécurisée est établie ;
 * - l'authentification applicative est valide.
 *
 * @param connectionId Identifiant unique de la connexion à vérifier.
 *
 * @return `true` si la connexion peut utiliser les channels gameplay,
 *         `false` sinon.
 */
bool ServerNetworkIncoming_IsConnectionAllowedForGameplayChannels(uint32_t connectionId);