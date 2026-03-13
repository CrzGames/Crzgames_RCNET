#include "services/nats/process/simulation/publish_message_for_subject_mysubject1.h"

#include <cstring> // strlen

#include "cJSON.h"

void ServerNats_ProcessSimulationDispatcher_HandlePublishMessageForSubjectMySubject1(
    RCNET_NATSContext* natsContext,
    const SimulationToNatsMessage& message)
{
    // Création d'un objet JSON racine : {}.
    cJSON* root = cJSON_CreateObject();

    // Vérifie que la création de l'objet JSON a réussi.
    if (root == NULL)
    {
        // En cas d'échec, on quitte immédiatement la fonction.
        return;
    }

    // Ajoute dans l'objet JSON une propriété "message"
    // contenant le texte de message.mySubject1Message.
    if (cJSON_AddStringToObject(root, "message", message.mySubject1Message.c_str()) == NULL)
    {
        // Si l'ajout échoue, on libère l'objet JSON déjà créé.
        cJSON_Delete(root);

        // Puis on quitte la fonction.
        return;
    }

    // Convertit l'objet JSON en chaîne compacte, par exemple :
    // {"message":"bonjour"}
    char* jsonString = cJSON_PrintUnformatted(root);

    // Vérifie que la conversion en chaîne JSON a réussi.
    if (jsonString == NULL)
    {
        // Si la conversion échoue, on libère l'objet JSON.
        cJSON_Delete(root);

        // Puis on quitte la fonction.
        return;
    }

    // Publie la chaîne JSON sur le sujet NATS contenu dans message.subject.
    rcnet_nats_publish(
        natsContext,                              // Contexte NATS à utiliser pour l'envoi.
        message.subject.c_str(),                  // Sujet NATS sur lequel publier.
        jsonString,                               // Buffer contenant le JSON sérialisé.
        static_cast<int>(strlen(jsonString))      // Taille du buffer JSON en octets, sans le '\0' final.
    );

    // Libère la mémoire allouée par cJSON_PrintUnformatted.
    cJSON_free(jsonString);

    // Libère l'objet JSON root.
    cJSON_Delete(root);
}