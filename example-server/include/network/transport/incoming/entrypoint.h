#pragma once

#include <rcenet/RCENET_enet.h> // ENetHost, ENetEvent

/**
 * @brief Point d'entrée principal du traitement des événements ENet entrants côté serveur.
 *
 * Cette fonction est appelée par la boucle réseau du serveur pour traiter
 * un événement ENet brut. Elle redirige ensuite le traitement vers les
 * handlers spécialisés selon le type de l'événement :
 * - connexion ;
 * - déconnexion ;
 * - réception de paquet.
 *
 * @param host Instance ENetHost serveur ayant produit l'événement.
 * @param event Événement ENet brut à traiter.
 */
void ServerNetworkIncoming_ProcessENetEvent(ENetHost* host, const ENetEvent* event);