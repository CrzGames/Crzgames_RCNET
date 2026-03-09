#include "RCNET/RCNET_nats.h"     // Déclarations liées à ton wrapper/client NATS
#include "RCNET/RCNET_logger.h"   // Déclarations du système de logs RCNET

// Standard C libraries
#include <string.h>               // Pour strdup() et autres fonctions de manipulation de chaînes

// Callback utilisée par la lib NATS quand elle a besoin de signer un nonce
// pour l'authentification NKey.
static natsStatus customSignatureHandler(
    char **customErrTxt,          // Message d'erreur personnalisé à renvoyer si la signature échoue
    unsigned char **signature,    // Buffer de sortie contenant la signature générée
    int *signatureLength,         // Taille de la signature générée
    const char *nonce,            // Chaîne envoyée par le serveur NATS à signer
    void *closure                 // Donnée utilisateur passée au handler (ici la seed privée)
) {
    // On récupère la seed privée NKey depuis le pointeur générique closure
    const char *seed = (const char*) closure;

    // On demande à la lib NATS de signer le nonce avec la seed privée
    natsStatus status = nats_Sign(seed, nonce, signature, signatureLength);

    // Si la signature a échoué...
    if (status != NATS_OK)
    {
        // ...on renvoie un texte d'erreur personnalisé
        *customErrTxt = strdup("Erreur lors de la signature");
    }

    // On retourne le statut de l'opération à la lib NATS
    return status;
}

// Initialise un client NATS RCNET
bool rcnet_nats_initialize(
    RCNET_NATSClient *client,         // Structure client à initialiser
    const char *natsServerURL,        // URL du serveur NATS (ex: nats://127.0.0.1:4222)
    bool useTLS,                      // Indique si on veut activer TLS
    bool skipVerifyCertsServer,       // Indique si on ignore la vérification du certificat serveur
    const char *publicKeyNKey,        // Clé publique NKey
    const char *privateKeySeedNKey    // Seed privée NKey utilisée pour signer
) {
    // Vérifier les paramètres d'initialisation
    if (client == NULL ||
        natsServerURL == NULL ||
        publicKeyNKey == NULL ||
        privateKeySeedNKey == NULL)
    {
        // Si un paramètre obligatoire manque, on log l'erreur
        RCNET_log(RCNET_LOG_ERROR, "Invalid NATS initialization parameters");

        // Et on signale l'échec
        return false;
    }

    natsStatus status;       // Variable pour stocker les codes de retour de la lib NATS
    natsOptions *opts = NULL; // Pointeur vers la structure d'options de connexion NATS

    // Création de la structure d'options NATS
    status = natsOptions_Create(&opts);
    if (status != NATS_OK)
    {
        // Si la création échoue, log + retour false
        RCNET_log(RCNET_LOG_ERROR, "Failed to create NATS options: %s", natsStatus_GetText(status));
        return false;
    }

    // Configure l'intervalle de ping client -> serveur à 20 000 ms (20 s)
    status = natsOptions_SetPingInterval(opts, 20000);
    if (status != NATS_OK)
    {
        // Si échec, log, libération des options, puis retour false
        RCNET_log(RCNET_LOG_ERROR, "Failed to set Ping interval: %s", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return false;
    }

    // Configure le nombre max de pings non répondus avant considérer la connexion perdue
    status = natsOptions_SetMaxPingsOut(opts, 5);
    if (status != NATS_OK)
    {
        // Si échec, log, libération des options, puis retour false
        RCNET_log(RCNET_LOG_ERROR, "Failed to set Max Pings Out: %s", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return false;
    }

    // Si on veut une connexion TLS...
    if (useTLS)
    {
        // ...on active le mode sécurisé
        status = natsOptions_SetSecure(opts, true);
        if (status != NATS_OK)
        {
            // Si activation TLS échoue, log + nettoyage + retour false
            RCNET_log(RCNET_LOG_ERROR, "Failed to enable TLS: %s", natsStatus_GetText(status));
            natsOptions_Destroy(opts);
            return false;
        }

        // Si on demande d'ignorer la vérification du certificat serveur...
        if (skipVerifyCertsServer)
        {
            // ...on désactive la vérification du certificat
            status = natsOptions_SkipServerVerification(opts, true);
            if (status != NATS_OK)
            {
                // Si ça échoue, log + nettoyage + retour false
                RCNET_log(RCNET_LOG_ERROR, "Failed to skip server verification: %s", natsStatus_GetText(status));
                natsOptions_Destroy(opts);
                return false;
            }
        }
    }

    // Configure l'authentification NKey :
    // - publicKeyNKey = identité publique envoyée au serveur
    // - customSignatureHandler = fonction appelée pour signer le nonce
    // - privateKeySeedNKey = donnée privée transmise au handler via closure
    status = natsOptions_SetNKey(opts, publicKeyNKey, customSignatureHandler, (void*) privateKeySeedNKey);
    if (status != NATS_OK)
    {
        // Si la config NKey échoue, log + nettoyage + retour false
        RCNET_log(RCNET_LOG_ERROR, "Failed to set NKey options: %s", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return false;
    }

    // Définit l'URL du serveur NATS à utiliser
    status = natsOptions_SetURL(opts, natsServerURL);
    if (status != NATS_OK)
    {
        // Si l'URL est invalide ou non définissable, log + nettoyage + retour false
        RCNET_log(RCNET_LOG_ERROR, "Failed to set NATS URL: %s", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return false;
    }

    // Initialise les champs internes du client avant connexion
    client->connection = NULL;        // Pas encore de connexion active
    client->subscriptions = NULL;     // Pas encore de tableau d'abonnements
    client->subscriptionCount = 0;    // Aucun abonnement pour l'instant

    // Tente de se connecter au serveur NATS avec les options préparées
    status = natsConnection_Connect(&(client->connection), opts);
    if (status != NATS_OK)
    {
        // Si la connexion échoue, log + destruction des options + retour false
        RCNET_log(RCNET_LOG_ERROR, "Failed to connect to NATS server: %s", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return false;
    }

    // Log de succès de connexion
    RCNET_log(RCNET_LOG_INFO, "Connected to NATS server: %s", natsServerURL);

    // Les options ne sont plus nécessaires après la connexion
    natsOptions_Destroy(opts);

    // Signale que l'initialisation a réussi
    return true;
}

// Nettoie complètement un client NATS
void rcnet_nats_cleanup(RCNET_NATSClient *client) {
    // Si le pointeur client est invalide, on ne fait rien
    if (client == NULL)
    {
        return;
    }

    // S'il existe un tableau d'abonnements...
    if (client->subscriptions != NULL)
    {
        // ...on parcourt tous les abonnements stockés
        for (size_t i = 0; i < client->subscriptionCount; i++)
        {
            // Si l'abonnement courant existe...
            if (client->subscriptions[i] != NULL)
            {
                // ...on demande au serveur d'arrêter proprement l'abonnement
                // après avoir traité les messages déjà en attente
                natsSubscription_Drain(client->subscriptions[i]);

                // On attend au maximum 5 secondes la fin du drain
                natsSubscription_WaitForDrainCompletion(client->subscriptions[i], 5000);

                // Puis on libère l'objet abonnement côté client
                natsSubscription_Destroy(client->subscriptions[i]);
            }
        }

        // On libère le tableau dynamique qui stocke les pointeurs d'abonnements
        free(client->subscriptions);
    }

    // S'il existe une connexion active...
    if (client->connection != NULL)
    {
        // ...on force l'envoi des données en attente avant fermeture, max 5 secondes
        natsConnection_FlushTimeout(client->connection, 5000);

        // Puis on détruit l'objet connexion
        natsConnection_Destroy(client->connection);
    }

    // Remise à zéro de la structure pour éviter les pointeurs pendants
    client->connection = NULL;
    client->subscriptions = NULL;
    client->subscriptionCount = 0;
}

// Publie un message sur un sujet NATS
bool rcnet_nats_publish(RCNET_NATSClient *client, const char *subject, const void *data, int dataLength) {
    // Vérifie que les paramètres minimums sont valides
    if (client == NULL || client->connection == NULL || subject == NULL || dataLength < 0)
    {
        // Si non, log erreur + retour false
        RCNET_log(RCNET_LOG_ERROR, "Invalid parameters for NATS publish");
        return false;
    }

    // Si on indique une taille > 0 mais sans buffer de données, c'est incohérent
    if (data == NULL && dataLength > 0)
    {
        RCNET_log(RCNET_LOG_ERROR, "data is NULL but dataLength > 0");
        return false;
    }

    // Envoie le message sur le sujet demandé
    natsStatus status = natsConnection_Publish(client->connection, subject, data, dataLength);
    if (status != NATS_OK)
    {
        // Si l'envoi échoue, log + retour false
        RCNET_log(RCNET_LOG_ERROR, "Failed to publish message: %s", natsStatus_GetText(status));
        return false;
    }

    // Publication réussie
    return true;
}

// S'abonne à un sujet NATS et stocke l'abonnement dans le client
bool rcnet_nats_subscribe(RCNET_NATSClient *client, const char *subject, natsMsgHandler messageHandler, void *closure) {
    // Vérifie les paramètres obligatoires
    if (client == NULL || client->connection == NULL || subject == NULL || messageHandler == NULL)
    {
        // Si un paramètre manque, log erreur + retour false
        RCNET_log(RCNET_LOG_ERROR, "Invalid parameters for NATS subscribe");
        return false;
    }

    // Créer un nouvel abonnement
    natsSubscription *newSubscription = NULL;

    // S'abonner au sujet spécifié
    // Le callback messageHandler sera appelé à chaque message reçu sur ce sujet
    // closure sera repassé tel quel au callback
    natsStatus status = natsConnection_Subscribe(&newSubscription, client->connection, subject, messageHandler, closure);

    // Vérifier si l'abonnement a été créé avec succès
    if (status != NATS_OK)
    {
        // Si l'abonnement échoue, log + retour false
        RCNET_log(RCNET_LOG_ERROR, "Failed to subscribe to subject: %s", natsStatus_GetText(status));
        return false;
    }

    // Redimensionner le tableau d'abonnements pour ajouter 1 case de plus
    natsSubscription **newSubscriptions =
        (natsSubscription **)realloc(client->subscriptions, (client->subscriptionCount + 1) * sizeof(natsSubscription*));

    // Si le realloc échoue...
    if (newSubscriptions == NULL)
    {
        // ...on log l'erreur
        RCNET_log(RCNET_LOG_ERROR, "Failed to allocate memory for new subscription");

        // ...on détruit l'abonnement fraîchement créé pour éviter une fuite mémoire
        natsSubscription_Destroy(newSubscription);

        // ...et on retourne false
        return false;
    }

    // Mise à jour du pointeur du tableau d'abonnements
    client->subscriptions = newSubscriptions;

    // Ajouter le nouvel abonnement dans la dernière case,
    // puis incrémenter le compteur d'abonnements
    client->subscriptions[client->subscriptionCount++] = newSubscription;

    // Log de succès
    RCNET_log(RCNET_LOG_INFO, "Subscribed to subject: %s", subject);

    // Abonnement réussi
    return true;
}