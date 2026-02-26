#ifndef RCNET_NATS_H
#define RCNET_NATS_H

// Standard C/C++ Libraries
#include <stdbool.h>
#include <stdlib.h>

#include <nats.h>

/**
 * @typedef {struct} RCNET_NATSClient
 * @brief Structure représentant un client NATS.
 *
 * Cette structure encapsule les composants essentiels d'un client NATS,
 * notamment la connexion au serveur, les abonnements, et le contexte JetStream.
 *
 * @property {natsConnection*} connection - Pointeur vers la connexion NATS active.
 * @property {natsSubscription**} subscriptions - Tableau des abonnements NATS actifs.
 * @property {size_t} subscriptionCount - Nombre d'abonnements NATS actifs.
 * @property {jsCtx*} jetStreamContext - Pointeur vers le contexte JetStream associé.
 */
typedef struct {
    natsConnection *connection;
    natsSubscription **subscriptions;
    size_t subscriptionCount;
    jsCtx *jetStreamContext;
} RCNET_NATSClient;

/**
 * @typedef {struct} RCNET_JetStreamPublishOptions
 * @brief Options de configuration pour la publication JetStream.
 *
 * Cette structure permet de configurer les options lors de la publication de messages dans JetStream,
 *
 * @property {int} maxWaitForAck - Durée maximale d'attente (en millisecondes) pour l'accusé de réception d'un message. 0 pour pas de timeout.
 */
typedef struct {
    int maxWaitForAck;
} RCNET_JetStreamPublishOptions;

/**
 * @typedef {struct} RCNET_JetStreamSubscribeOptions
 * @brief Options de configuration pour l'abonnement JetStream.
 *
 * Cette structure permet de configurer les options lors de l'abonnement à un sujet JetStream,
 * telles que le nombre maximal de tentatives de livraison, le délai d'attente pour les accusés de réception, 
 * et le nombre maximum d'accusés de réception en attente.
 *
 * @property {int} maxDeliver - Nombre maximum de tentatives de livraison d'un message. 0 pour aucune limite.
 * @property {int} ackWait - Durée maximale d'attente (en millisecondes) pour l'accusé de réception d'un message. 0 pour pas de timeout.
 * @property {bool} manualAck - Si vrai, l'utilisateur devra accuser réception des messages manuellement. Sinon, les messages seront automatiquement accusés de réception.
 * @property {int} maxAckPending - Nombre maximum d'accusés de réception en attente pour un abonnement. 0 pour aucune limite.
 */
typedef struct {
    int maxDeliver;
    int ackWait;
    bool manualAck;
    int maxAckPending;
} RCNET_JetStreamSubscribeOptions;

/**
 * @typedef {struct} RCNET_JetStreamStreamOptions
 * @brief Options de configuration pour un stream JetStream.
 *
 * Cette structure permet de configurer les options lors de la création d'un stream JetStream,
 * telles que l'activation des accusés de réception, et l'âge maximal des messages.
 *
 * @property {bool} noAck - Si vrai, les accusés de réception ne sont pas activés pour le stream.
 * @property {int64_t} messageMaxAge - Âge maximal des messages dans le stream (en nanosecondes). 0 pour pas de limite.
 */
typedef struct {
    bool noAck;
    int64_t messageMaxAge;
} RCNET_JetStreamStreamOptions;

/**
 * @brief Initialise un client NATS avec les options spécifiées.
 * 
 * Cette fonction établit une connexion sécurisée avec un serveur NATS,
 * configure les certificats TLS si fournis, et initialise le contexte JetStream.
 * 
 * @param {RCNET_NATSClient*} client - Pointeur vers la structure RCNET_NATSClient à initialiser.
 * @param {const char*} natsServerURL - URL du serveur NATS.
 * @param {const char*} certFile - Chemin vers le fichier de certificat (peut être NULL).
 * @param {const char*} keyFile - Chemin vers le fichier de clé (peut être NULL).
 * @param {const char*} caFile - Chemin vers le fichier CA (peut être NULL).
 * @param {bool} skipVerifyCertsServer - Indicateur pour ignorer la vérification des certificats du serveur.
 * @param {const char*} publicKeyNKey - Clé publique NKey pour l'authentification.
 * @param {void*} privateKeySeedNKey - Clé seed privée NKey pour la signature.
 * @return {int} 0 en cas de succès, -1 en cas d'erreur.
 */
int rcnet_nats_initialize(RCNET_NATSClient *client, const char *natsServerURL, const char *certFile, const char *keyFile, const char *caFile, bool skipVerifyCertsServer, const char *publicKeyNKey, void *privateKeySeedNKey);

/**
 * @brief Nettoie et libère les ressources associées à un client NATS.
 * 
 * Cette fonction ferme la connexion, détruit les subscriptions et le contexte JetStream,
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
 * @return {int} 0 en cas de succès, -1 en cas d'erreur.
 */
int rcnet_nats_publish(RCNET_NATSClient *client, const char *subject, const void* data, int dataLength);

/**
 * @brief S'abonne à un sujet spécifique via NATS.
 * 
 * Cette fonction crée une subscription NATS pour écouter les messages envoyés sur le sujet spécifié.
 * 
 * @param {RCNET_NATSClient*} client - Pointeur vers le client NATS.
 * @param {const char*} subject - Sujet auquel s'abonner.
 * @param {natsMsgHandler} messageHandler - Fonction de rappel pour gérer les messages reçus.
 * @param {void*} closure - Données utilisateur optionnelles passées à la fonction de rappel.
 * @return {int} 0 en cas de succès, -1 en cas d'erreur.
 */
int rcnet_nats_subscribe(RCNET_NATSClient *client, const char *subject, natsMsgHandler messageHandler, void *closure);

/**
 * @brief Publie un message sur un sujet spécifique via JetStream.
 * 
 * Cette fonction envoie des données sur le sujet spécifié en utilisant le contexte JetStream.
 * 
 * @param {RCNET_NATSClient*} client - Pointeur vers le client NATS.
 * @param {const char*} subject - Sujet sur lequel publier le message.
 * @param {const void*} data - Pointeur vers les données à envoyer.
 * @param {int} dataLength - Longueur des données à envoyer.
 * @return {int} 0 en cas de succès, -1 en cas d'erreur.
 */
int rcnet_nats_jetstream_publish(RCNET_NATSClient *client, const char *subject, const void* data, int dataLength, RCNET_JetStreamPublishOptions *options);

/**
 * @brief S'abonne à un sujet spécifique via JetStream.
 * 
 * Cette fonction crée une subscription JetStream pour écouter les messages envoyés sur le sujet spécifié.
 * 
 * @param {RCNET_NATSClient*} client - Pointeur vers le client NATS.
 * @param {const char*} subject - Sujet auquel s'abonner.
 * @param {natsMsgHandler} messageHandler - Fonction de rappel pour gérer les messages reçus.
 * @param {void*} closure - Données utilisateur optionnelles passées à la fonction de rappel.
 * @return {int} 0 en cas de succès, -1 en cas d'erreur.
 */
int rcnet_nats_jetstream_subscribe(RCNET_NATSClient *client, const char *subject, natsMsgHandler messageHandler, void *closure, RCNET_JetStreamSubscribeOptions *options);

/**
 * @brief Vérifie l'existence d'un stream JetStream et le crée si nécessaire.
 * 
 * Cette fonction vérifie si un stream JetStream existe pour le nom de stream spécifié. 
 * Si le stream n'existe pas, il est créé avec les sujets et les paramètres fournis. 
 * Si le stream existe déjà, la fonction ne modifie pas le stream existant, même si de nouveaux sujets sont spécifiés.
 *
 * IMPORTANT: Les subjects doivent être uniques et ne doit pas déjà exister dans un autre stream.
 * IMPORTANT: Le nom du stream doit être unique et ne doit pas déjà exister.
 *
 * @param {RCNET_NATSClient*} client - Pointeur vers le client NATS.
 * @param {const char*} streamName - Nom du stream à vérifier ou créer.
 * @param {const char*[]} subjects - Tableau des sujets associés au stream.
 * @param {int} subjectsCount - Nombre de sujets dans le tableau.
 * @return {int} 0 en cas de succès, -1 en cas d'erreur.
 */
int rcnet_nats_check_and_create_stream(RCNET_NATSClient *client, const char *streamName, const char *subjects[], int subjectsCount, RCNET_JetStreamStreamOptions *options);

/**
 * @brief Met à jour les sujets d'un stream JetStream existant.
 * 
 * Cette fonction met à jour les sujets d'un stream JetStream existant avec les nouveaux sujets spécifiés.
 * Les sujets existants du stream sont remplacés par les nouveaux sujets fournis.
 * 
 * IMPORTANT: Les subjects doivent être uniques et ne doit pas déjà exister dans un autre stream.
 * 
 * @param {RCNET_NATSClient*} client - Pointeur vers le client NATS.
 * @param {const char*} streamName - Nom du stream à mettre à jour.
 * @param {const char*[]} newSubjects - Tableau des nouveaux sujets à associer au stream.
 * @param {int} newSubjectsCount - Nombre de nouveaux sujets dans le tableau.
 * @return {int} 0 en cas de succès, -1 en cas d'erreur.
 */
int rcnet_nats_update_stream_subjects(RCNET_NATSClient *client, const char *streamName, const char *newSubjects[], int newSubjectsCount);

#endif // RCNET_NATS_H
