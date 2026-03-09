#ifndef RCNET_NATS_H
#define RCNET_NATS_H

// Standard C/C++ Libraries
#include <stdbool.h> // bool
#include <stdlib.h>  // malloc, free
#include <stdint.h>  // uint32_t, uint64_t

// NATS C Client Library
#include <nats.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef {struct} RCNET_NATSClient
 * @brief Structure représentant un client NATS.
 *
 * Cette structure encapsule les composants essentiels d'un client NATS,
 * notamment la connexion au serveur, les abonnements actifs.
 *
 * @property {natsConnection*} connection - Pointeur vers la connexion NATS active.
 * @property {natsSubscription**} subscriptions - Tableau des abonnements NATS actifs.
 * @property {size_t} subscriptionCount - Nombre d'abonnements NATS actifs.
 */
typedef struct {
    natsConnection *connection;
    natsSubscription **subscriptions;
    size_t subscriptionCount;
} RCNET_NATSClient;

/**
 * @brief Initialise un client NATS avec les options spécifiées.
 * 
 * @param {RCNET_NATSClient*} client - Pointeur vers la structure RCNET_NATSClient à initialiser.
 * @param {const char*} natsServerURL - URL du serveur NATS.
 * @param {bool} useTLS - Indicateur pour utiliser TLS ou non.
 * @param {bool} skipVerifyCertsServer - Indicateur pour ignorer la vérification des certificats du serveur.
 * @param {const char*} publicKeyNKey - Clé publique NKey pour l'authentification.
 * @param {const char*} privateKeySeedNKey - Clé privée NKey pour l'authentification.
 * @return {bool} true en cas de succès, false en cas d'erreur.
 */
bool rcnet_nats_initialize(
    RCNET_NATSClient *client,
    const char *natsServerURL,
    bool useTLS,
    bool skipVerifyCertsServer,
    const char *publicKeyNKey,
    const char *privateKeySeedNKey
);

/**
 * @brief Nettoie et libère les ressources associées à un client NATS.
 * 
 * Cette fonction ferme la connexion, détruit les subscriptions,
 * et libère les ressources associées.
 * 
 * @param {RCNET_NATSClient*} client - Pointeur vers la structure RCNET_NATSClient à nettoyer.
 */
void rcnet_nats_cleanup(RCNET_NATSClient *client);

/**
 * @brief Publie un message sur un sujet spécifique via NATS.
 * 
 * Cette fonction envoie des données sur le sujet spécifié à travers la connexion NATS.
 * 
 * @param {RCNET_NATSClient*} client - Pointeur vers le client NATS.
 * @param {const char*} subject - Sujet sur lequel publier le message.
 * @param {const void*} data - Pointeur vers les données à envoyer.
 * @param {int} dataLength - Longueur des données à envoyer.
 * @return {bool} true en cas de succès, false en cas d'erreur.
 */
bool rcnet_nats_publish(RCNET_NATSClient *client, const char *subject, const void* data, int dataLength);

/**
 * @brief S'abonne à un sujet spécifique via NATS.
 * 
 * Cette fonction crée une subscription NATS pour écouter les messages envoyés sur le sujet spécifié.
 * 
 * @param {RCNET_NATSClient*} client - Pointeur vers le client NATS.
 * @param {const char*} subject - Sujet auquel s'abonner.
 * @param {natsMsgHandler} messageHandler - Fonction de rappel pour gérer les messages reçus.
 * @param {void*} closure - Données utilisateur optionnelles passées à la fonction de rappel.
 * @return {bool} true en cas de succès, false en cas d'erreur.
 */
bool rcnet_nats_subscribe(RCNET_NATSClient *client, const char *subject, natsMsgHandler messageHandler, void *closure);

#ifdef __cplusplus
}
#endif

#endif // RCNET_NATS_H
