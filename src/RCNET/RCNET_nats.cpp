#include "RCNET/RCNET_nats.h"
#include "RCNET/RCNET_logger.h"

// Standard C libraries
#include <string.h>

static natsStatus customSignatureHandler(char **customErrTxt, unsigned char **signature, int *signatureLength, const char *nonce, void *closure)
{
    const char *seed = (const char*) closure;
    natsStatus status = nats_Sign(seed, nonce, signature, signatureLength);

    if (status != NATS_OK) {
        *customErrTxt = strdup("Erreur lors de la signature");
    }

    return status;
}

int rcnet_nats_initialize(RCNET_NATSClient *client, const char *natsServerURL, const char *certFile, const char *keyFile, const char *caFile, bool skipVerifyCertsServer, const char *publicKeyNKey, void *privateKeySeedNKey)
{
    natsStatus status;
    natsOptions *opts = NULL;

    // Créer les options NATS
    status = natsOptions_Create(&opts);
    if (status != NATS_OK) 
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to create NATS options: %s\n", natsStatus_GetText(status));
        return -1;
    }

    // Set Ping interval to 20 seconds (20,000 milliseconds)
    status = natsOptions_SetPingInterval(opts, 20000);
    if (status != NATS_OK) 
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to set Ping interval: %s\n", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return -1;
    }

    // Set the limit to 5
    status = natsOptions_SetMaxPingsOut(opts, 5);
    if (status != NATS_OK) 
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to set Max Pings Out: %s\n", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return -1;
    }

    // Set the secure flag for activating SSL/TLS
    natsOptions_SetSecure(opts, true);

    // Load certificates and key if provided
    if (certFile != NULL && keyFile != NULL) {
        status = natsOptions_LoadCertificatesChain(opts, certFile, keyFile);
        if (status != NATS_OK) {
            RCNET_log(RCNET_LOG_ERROR, "Failed to load certificates: %s\n", natsStatus_GetText(status));
            natsOptions_Destroy(opts);
            return -1;
        }
    }

    // Load CA certificate if provided
    if (caFile != NULL) {
        status = natsOptions_LoadCATrustedCertificates(opts, caFile);
        if (status != NATS_OK) {
            RCNET_log(RCNET_LOG_ERROR, "Failed to load CA certificates: %s\n", natsStatus_GetText(status));
            natsOptions_Destroy(opts);
            return -1;
        }
    }

    // Skip server verification if required
    if (skipVerifyCertsServer) {
        status = natsOptions_SkipServerVerification(opts, true);
        if (status != NATS_OK) {
            RCNET_log(RCNET_LOG_ERROR, "Failed to skip server verification: %s\n", natsStatus_GetText(status));
            natsOptions_Destroy(opts);
            return -1;
        }
    }

    // Set the NKey authentication
    status = natsOptions_SetNKey(opts, publicKeyNKey, customSignatureHandler, privateKeySeedNKey);
    if (status != NATS_OK) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to set NKey options: %s\n", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return -1;
    }

    // Set the URL of the NATS server
    natsOptions_SetURL(opts, natsServerURL);

    // Connect to the NATS server
    status = natsConnection_Connect(&(client->connection), opts);
    if (status != NATS_OK) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to connect to NATS server: %s\n", natsStatus_GetText(status));
        natsOptions_Destroy(opts);
        return -1;
    } else {
       RCNET_log(RCNET_LOG_INFO, "Connected to NATS server: %s\n", natsServerURL);
    }

    // Create JetStream context
    status = natsConnection_JetStream(&(client->jetStreamContext), client->connection, NULL);
    if (status != NATS_OK) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to create JetStream context: %s\n", natsStatus_GetText(status));
        natsConnection_Close(client->connection);
        natsConnection_Destroy(client->connection);
        natsOptions_Destroy(opts);
        return -1;
    }

    // Initialiser les abonnements
    client->subscriptions = NULL;
    client->subscriptionCount = 0;

    // Destroy the options
    natsOptions_Destroy(opts);    

    return 0;
}

void rcnet_nats_cleanup(RCNET_NATSClient *client)
{
    if (client->subscriptions != NULL) {
        for (size_t i = 0; i < client->subscriptionCount; i++) {
            if (client->subscriptions[i] != NULL) {
                natsSubscription_Destroy(client->subscriptions[i]);
            }
        }
        free(client->subscriptions);
    }

    if (client->jetStreamContext != NULL) {
        jsCtx_Destroy(client->jetStreamContext);
    }

    if (client->connection != NULL) {
        // Flush the connection to ensure all messages are sent
        natsStatus status = natsConnection_FlushTimeout(client->connection, 5000); // Timeout of 5 seconds
        if (status != NATS_OK) {
            RCNET_log(RCNET_LOG_ERROR, "Failed to flush connection: %s\n", natsStatus_GetText(status));
        }

        // Drain the connection to ensure all messages are sent
        status = natsConnection_Drain(client->connection);
        if (status == NATS_OK) {
            // Wait for the connection to be drained
            natsConnection_DrainTimeout(client->connection, 0);
        } else {
            RCNET_log(RCNET_LOG_ERROR, "Failed to drain connection: %s\n", natsStatus_GetText(status));
        }

        natsConnection_Close(client->connection);
        natsConnection_Destroy(client->connection);
    }
}

int rcnet_nats_publish(RCNET_NATSClient *client, const char *subject, const void* data, int dataLength)
{
    // Publish the message on the specified subject
    natsStatus status = natsConnection_Publish(client->connection, subject, data, dataLength);

    // Check if the message was published successfully
    if (status != NATS_OK) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to publish message: %s\n", natsStatus_GetText(status));
        return -1;
    }

    // Log success
    RCNET_log(RCNET_LOG_INFO, "Published message on subject: %s\n", subject);

    return 0;
}

int rcnet_nats_subscribe(RCNET_NATSClient *client, const char *subject, natsMsgHandler messageHandler, void *closure)
{
    // Créer un nouvel abonnement
    natsSubscription *newSubscription = NULL;

    // S'abonner au sujet spécifié
    natsStatus status = natsConnection_Subscribe(&newSubscription, client->connection, subject, messageHandler, closure);
    
    // Vérifier si l'abonnement a été créé avec succès
    if (status != NATS_OK) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to subscribe to subject: %s\n", natsStatus_GetText(status));
        return -1;
    }

    // Redimensionner le tableau d'abonnements
    natsSubscription **newSubscriptions = (natsSubscription **)realloc(client->subscriptions, (client->subscriptionCount + 1) * sizeof(natsSubscription*));
    if (newSubscriptions == NULL) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to allocate memory for new subscription\n");
        natsSubscription_Destroy(newSubscription);
        return -1;
    }

    // Mise à jour du tableau d'abonnements
    client->subscriptions = newSubscriptions;

    // Ajouter le nouvel abonnement au tableau
    client->subscriptions[client->subscriptionCount++] = newSubscription;

    // Log success
    RCNET_log(RCNET_LOG_INFO, "Subscribed to subject: %s\n", subject);

    return 0;
}

int rcnet_nats_jetstream_publish(RCNET_NATSClient *client, const char *subject, const void* data, int dataLength, RCNET_JetStreamPublishOptions *options)
{
    natsStatus status;
    jsPubAck *jetStreamPubAck = NULL;
    jsErrCode jetStreamErrorCode;
    jsPubOptions jetStreamPublishOptions;

    // Initialiser les options JetStream
    memset(&jetStreamPublishOptions, 0, sizeof(jsPubOptions));
    if (options != NULL) {
        jetStreamPublishOptions.MaxWait = options->maxWaitForAck;
    }

    // Publier le message JetStream sur le sujet spécifié
    status = js_Publish(&jetStreamPubAck, client->jetStreamContext, subject, data, dataLength, &jetStreamPublishOptions, &jetStreamErrorCode);
    if (status != NATS_OK) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to publish JetStream message: %s\n", natsStatus_GetText(status));
        RCNET_log(RCNET_LOG_ERROR, "Error code: %d\n", jetStreamErrorCode);
        return -1;
    }

    // Détruire l'accusé de réception JetStream
    jsPubAck_Destroy(jetStreamPubAck);

    // Log success
    RCNET_log(RCNET_LOG_INFO, "Published JetStream message on subject: %s\n", subject);

    return 0;
}

int rcnet_nats_jetstream_subscribe(RCNET_NATSClient *client, const char *subject, natsMsgHandler messageHandler, void *closure, RCNET_JetStreamSubscribeOptions *options)
{
    natsStatus status;
    jsErrCode jetStreamErrorCode;
    jsSubOptions jetStreamSubOptions;
    natsSubscription *newSubscription = NULL;

    // Initialiser les options de souscription JetStream
    memset(&jetStreamSubOptions, 0, sizeof(jsSubOptions));
    if (options != NULL) {
        jetStreamSubOptions.Config.MaxDeliver = options->maxDeliver;
        jetStreamSubOptions.Config.AckWait = options->ackWait;
        jetStreamSubOptions.Config.MaxAckPending = options->maxAckPending;
        jetStreamSubOptions.ManualAck = options->manualAck;
    }

    // S'abonner au sujet JetStream
    status = js_Subscribe(&newSubscription, client->jetStreamContext, subject, messageHandler, closure, NULL, &jetStreamSubOptions, &jetStreamErrorCode);
    if (status != NATS_OK) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to subscribe to JetStream subject: %s\n", natsStatus_GetText(status));
        RCNET_log(RCNET_LOG_ERROR, "Error code: %d\n", jetStreamErrorCode);
        return -1;
    }

    // Redimensionner le tableau d'abonnements
    natsSubscription **newSubscriptions = (natsSubscription **)realloc(client->subscriptions, (client->subscriptionCount + 1) * sizeof(natsSubscription*));
    if (newSubscriptions == NULL) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to allocate memory for new subscription\n");
        natsSubscription_Destroy(newSubscription);
        return -1;
    }

    // Mise à jour du tableau d'abonnements
    client->subscriptions = newSubscriptions;

    // Ajouter le nouvel abonnement au tableau
    client->subscriptions[client->subscriptionCount++] = newSubscription;

    // Log success
    RCNET_log(RCNET_LOG_INFO, "Subscribed to JetStream subject: %s\n", subject);

    return 0;
}

int rcnet_nats_check_and_create_stream(RCNET_NATSClient *client, const char *streamName, const char *subjects[], int subjectsCount, RCNET_JetStreamStreamOptions *options)
{
    natsStatus status;
    jsStreamInfo *jetStreamInfo = NULL;
    jsStreamConfig jetStreamConfig;
    jsErrCode jetStreamErrorCode;

    // Essayer de récupérer les informations du stream
    status = js_GetStreamInfo(&jetStreamInfo, client->jetStreamContext, streamName, NULL, &jetStreamErrorCode);
    if (status == NATS_NOT_FOUND)
    {        
        // Le stream n'existe pas, le créer
        memset(&jetStreamConfig, 0, sizeof(jsStreamConfig));  // Initialiser la structure à zéro

        jetStreamConfig.Name = streamName;                                // Nom du stream
        jetStreamConfig.Subjects = subjects;                              // Sujets
        jetStreamConfig.SubjectsLen = subjectsCount;                      // Nombre de sujets
        jetStreamConfig.Retention = js_LimitsPolicy;                      // Politique de rétention des messages
        jetStreamConfig.MaxMsgSize = 0;                                   // Pas de limite sur la taille des messages
        jetStreamConfig.MaxMsgs = 0;                                      // Pas de limite sur le nombre de messages
        jetStreamConfig.MaxMsgsPerSubject = 0;                            // Pas de limite sur le nombre de messages par sujet
        jetStreamConfig.MaxBytes = 0;                                     // Pas de limite sur la taille des messages
        jetStreamConfig.MaxConsumers = 0;                                 // Pas de limite sur le nombre de consommateurs
        jetStreamConfig.MaxAge = (options != NULL) ? options->messageMaxAge : 0; // Age maximum des messages avant d'être supprimés
        jetStreamConfig.Discard = js_DiscardOld;                          // Supprimera les anciens messages pour revenir aux limites
        jetStreamConfig.Storage = js_FileStorage;                         // Stockage sur disque
        jetStreamConfig.NoAck = false;                                    // Renvoyer un accusé de réception après la réception du message

        status = js_AddStream(&jetStreamInfo, client->jetStreamContext, &jetStreamConfig, NULL, &jetStreamErrorCode);
        if (status != NATS_OK)
        {
            RCNET_log(RCNET_LOG_ERROR, "Failed to create stream: %s\n", natsStatus_GetText(status));
            RCNET_log(RCNET_LOG_ERROR, "Error code: %d\n", jetStreamErrorCode);
            return -1;
        }
        else
        {
            RCNET_log(RCNET_LOG_INFO, "Stream created: %s\n", streamName);
        }
    }
    else if (status == NATS_OK)
    {
        // Le stream existe déjà
        RCNET_log(RCNET_LOG_INFO, "Stream already exists: %s\n", streamName);
    }
    else
    {
        // Une autre erreur s'est produite
        RCNET_log(RCNET_LOG_ERROR, "Error checking stream existence: %s\n", natsStatus_GetText(status));
        RCNET_log(RCNET_LOG_ERROR, "Error code: %d\n", jetStreamErrorCode);
        return -1;
    }

    // Libérer les informations du stream si elles ont été récupérées
    if (jetStreamInfo != NULL)
        jsStreamInfo_Destroy(jetStreamInfo);

    return 0;
}

int rcnet_nats_update_stream_subjects(RCNET_NATSClient *client, const char *streamName, const char *newSubjects[], int newSubjectsCount)
{
    natsStatus status;
    jsStreamInfo *jetStreamInfo = NULL;
    jsStreamConfig jetStreamConfig;
    jsErrCode jetStreamErrorCode;

    // Récupérer les informations actuelles du stream
    status = js_GetStreamInfo(&jetStreamInfo, client->jetStreamContext, streamName, NULL, &jetStreamErrorCode);
    if (status != NATS_OK) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to retrieve stream info: %s\n", natsStatus_GetText(status));
        RCNET_log(RCNET_LOG_ERROR, "Error code: %d\n", jetStreamErrorCode);
        return -1;
    }

    // Initialiser la structure à zéro
    memset(&jetStreamConfig, 0, sizeof(jsStreamConfig)); 

    // Mettre à jour la configuration du stream avec les nouveaux sujets
    jetStreamConfig.Subjects = newSubjects;
    jetStreamConfig.SubjectsLen = newSubjectsCount;

    // Envoyer la mise à jour du stream
    status = js_UpdateStream(&jetStreamInfo, client->jetStreamContext, &jetStreamConfig, NULL, &jetStreamErrorCode);
    if (status != NATS_OK) {
        RCNET_log(RCNET_LOG_ERROR, "Failed to update stream: %s\n", natsStatus_GetText(status));
        jsStreamInfo_Destroy(jetStreamInfo);
        return -1;
    }

    // Libérer les informations du stream si elles ont été récupérées
    if (jetStreamInfo != NULL)
        jsStreamInfo_Destroy(jetStreamInfo);
    
    // Log success
    RCNET_log(RCNET_LOG_INFO, "Stream updated successfully: %s\n", streamName);

    return 0;
}
