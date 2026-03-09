#include "RCNET/RCNET_nats.h"
#include "RCNET/RCNET_logger.h"

// Standard C libraries
#include <string.h>

static natsStatus customSignatureHandler(char **customErrTxt, unsigned char **signature, int *signatureLength, const char *nonce, void *closure)
{
    const char *seed = (const char*) closure;
    natsStatus status = nats_Sign(seed, nonce, signature, signatureLength);

    if (status != NATS_OK) 
    {
        *customErrTxt = strdup("Erreur lors de la signature");
    }

    return status;
}

bool rcnet_nats_initialize(
    RCNET_NATSClient *client,
    const char *natsServerURL,
    bool useTLS,
    bool skipVerifyCertsServer,
    const char *publicKeyNKey,
    const char *privateKeySeedNKey
)
{
    // Vérfier les paramètres d'initialisation
    if (client == NULL ||
        natsServerURL == NULL ||
        publicKeyNKey == NULL ||
        privateKeySeedNKey == NULL)
    {
        RCNET_log(RCNET_LOG_ERROR, "Invalid NATS initialization parameters");
        return false;
    }

    memset(client, 0, sizeof(*client));

    natsStatus status;
    natsOptions *opts = NULL;

    status = natsOptions_Create(&opts);
    if (status != NATS_OK)
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to create NATS options: %s", natsStatus_GetText(status));
        return false;
    }

    status = natsOptions_SetPingInterval(opts, 20000);
    if (status != NATS_OK)
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to set Ping interval: %s", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return false;
    }

    status = natsOptions_SetMaxPingsOut(opts, 5);
    if (status != NATS_OK)
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to set Max Pings Out: %s", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return false;
    }

    if (useTLS)
    {
        status = natsOptions_SetSecure(opts, true);
        if (status != NATS_OK)
        {
            RCNET_log(RCNET_LOG_ERROR, "Failed to enable TLS: %s", natsStatus_GetText(status));
            natsOptions_Destroy(opts);
            return false;
        }

        if (skipVerifyCertsServer)
        {
            status = natsOptions_SkipServerVerification(opts, true);
            if (status != NATS_OK)
            {
                RCNET_log(RCNET_LOG_ERROR, "Failed to skip server verification: %s", natsStatus_GetText(status));
                natsOptions_Destroy(opts);
                return false;
            }
        }
    }

    status = natsOptions_SetNKey(opts, publicKeyNKey, customSignatureHandler, (void*) privateKeySeedNKey);
    if (status != NATS_OK)
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to set NKey options: %s", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return false;
    }

    status = natsOptions_SetURL(opts, natsServerURL);
    if (status != NATS_OK)
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to set NATS URL: %s", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return false;
    }

    client->connection = NULL;
    client->subscriptions = NULL;
    client->subscriptionCount = 0;

    status = natsConnection_Connect(&(client->connection), opts);
    if (status != NATS_OK)
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to connect to NATS server: %s", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return false;
    }

    RCNET_log(RCNET_LOG_INFO, "Connected to NATS server: %s", natsServerURL);

    natsOptions_Destroy(opts);

    return true;
}

void rcnet_nats_cleanup(RCNET_NATSClient *client)
{
    if (client == NULL) 
    {
        return;
    }

    if (client->subscriptions != NULL)
    {
        for (size_t i = 0; i < client->subscriptionCount; i++)
        {
            if (client->subscriptions[i] != NULL)
            {
                natsSubscription_Drain(client->subscriptions[i]);
                natsSubscription_WaitForDrainCompletion(client->subscriptions[i], 5000);
                natsSubscription_Destroy(client->subscriptions[i]);
            }
        }

        free(client->subscriptions);
    }

    if (client->connection != NULL)
    {
        natsConnection_FlushTimeout(client->connection, 5000);
        natsConnection_Destroy(client->connection);
    }

    client->connection = NULL;
    client->subscriptions = NULL;
    client->subscriptionCount = 0;
}

bool rcnet_nats_publish(RCNET_NATSClient *client, const char *subject, const void *data, int dataLength)
{
    if (client == NULL || client->connection == NULL || subject == NULL || dataLength < 0)
    {
        RCNET_log(RCNET_LOG_ERROR, "Invalid parameters for NATS publish");
        return false;
    }

    if (data == NULL && dataLength > 0)
    {
        RCNET_log(RCNET_LOG_ERROR, "data is NULL but dataLength > 0");
        return false;
    }

    natsStatus status = natsConnection_Publish(client->connection, subject, data, dataLength);
    if (status != NATS_OK)
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to publish message: %s", natsStatus_GetText(status));
        return false;
    }

    return true;
}

bool rcnet_nats_subscribe(RCNET_NATSClient *client, const char *subject, natsMsgHandler messageHandler, void *closure)
{
    if (client == NULL || client->connection == NULL || subject == NULL || messageHandler == NULL)
    {
        RCNET_log(RCNET_LOG_ERROR, "Invalid parameters for NATS subscribe");
        return false;
    }

    // Créer un nouvel abonnement
    natsSubscription *newSubscription = NULL;

    // S'abonner au sujet spécifié
    natsStatus status = natsConnection_Subscribe(&newSubscription, client->connection, subject, messageHandler, closure);
    
    // Vérifier si l'abonnement a été créé avec succès
    if (status != NATS_OK) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to subscribe to subject: %s", natsStatus_GetText(status));
        return false;
    }

    // Redimensionner le tableau d'abonnements
    natsSubscription **newSubscriptions = (natsSubscription **)realloc(client->subscriptions, (client->subscriptionCount + 1) * sizeof(natsSubscription*));
    if (newSubscriptions == NULL) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to allocate memory for new subscription");
        natsSubscription_Destroy(newSubscription);
        return false;
    }

    // Mise à jour du tableau d'abonnements
    client->subscriptions = newSubscriptions;

    // Ajouter le nouvel abonnement au tableau
    client->subscriptions[client->subscriptionCount++] = newSubscription;

    // Log success
    RCNET_log(RCNET_LOG_INFO, "Subscribed to subject: %s", subject);

    return true;
}