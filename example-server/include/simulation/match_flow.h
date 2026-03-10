#pragma once

#include <cstdint> // uint64_t

#include "game/state.h"
#include "network/state.h"
#include "core/threading/queues/simulation_to_network_outgoing.h"

/**
 * @brief Vérifie si toutes les sessions connectées sont prêtes à démarrer le match.
 *
 * Les conditions minimales sont :
 * - le nombre de sessions connectées doit être suffisant ;
 * - toutes les sessions doivent être marquées prêtes.
 *
 * @param networkState État réseau global du serveur.
 *
 * @return `true` si toutes les sessions requises sont prêtes,
 *         `false` sinon.
 */
bool ServerSimulation_AreAllSessionsReadyForMatch(const NetworkState& networkState);

/**
 * @brief Gère le flow de préparation et de démarrage du match côté serveur.
 *
 * Cette fonction orchestre l'envoi des paquets initiaux de match et de monde,
 * attend que tous les clients soient prêts, envoie le countdown de démarrage,
 * puis bascule le match dans l'état "started" lorsque le tick cible est atteint.
 *
 * @param gameState État global du jeu.
 * @param networkState État réseau global du serveur.
 * @param simToNetQueue Queue simulation -> réseau utilisée pour envoyer
 *        les paquets fiables liés au flow de match.
 * @param currentTick Tick courant de simulation.
 */
void ServerSimulation_CheckMatchFlow(
    GameState& gameState,
    const NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    uint64_t currentTick);
