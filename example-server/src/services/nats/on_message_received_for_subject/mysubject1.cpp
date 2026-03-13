#include "services/nats/on_message_received_for_subject/mysubject1.h"

#include "core/threading/queues/nats_to_simulation.h"
#include "core/context.h"

#include <cJSON.h>

void ServerNats_OnMessageReceivedForSubjectMySubject1(natsConnection* nc, natsSubscription* sub, natsMsg* msg, void* closure)
{
    (void)nc;
    (void)sub;
    (void)closure;

    // Vérifie que le message reçu n'est pas nul.
    if (msg == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN, "[SERVER] [NATS] - Received null natsMsg for mysubject1");
        return;
    }

    // Prépare un message à envoyer du thread NATS vers le thread simulation.
    NatsToSimulationMessage outMessage{};

    // Set le type de message pour le dispatcher du thread simulation.
    outMessage.type = NatsToSimulationMessageType::MESSAGE_RECEIVED_FOR_SUBJECT_MYSUBJECT1;

    // Récupère le sujet du message NATS et le stocke dans outMessage.subject.
    const char* subject = natsMsg_GetSubject(msg);
    if (subject != nullptr)
    {
        outMessage.subject = subject;
    }

    // Récupère le payload du message NATS et sa longueur.
    const char* payloadData = natsMsg_GetData(msg);
    const int payloadLength = natsMsg_GetDataLength(msg);

    // Vérifie que le payload n'est pas nul et a une longueur positive avant de tenter de le traiter.
    if (payloadData != nullptr && payloadLength > 0)
    {
        // Desérialise le payload JSON du message NATS pour extraire la propriété "message".
        cJSON* jsonRoot = cJSON_ParseWithLength(payloadData, payloadLength);
        if (jsonRoot != nullptr)
        {
            // Récupère la propriété "message" du JSON et la stocke dans outMessage.mySubject1Message.
            cJSON* messageItem = cJSON_GetObjectItem(jsonRoot, "message");
            if (cJSON_IsString(messageItem) && messageItem->valuestring != nullptr)
            {
                outMessage.mySubject1Message = messageItem->valuestring;
            }

            // Libère l'objet JSON root après utilisation.
            cJSON_Delete(jsonRoot);
        }
        else
        {
            RCNET_log(RCNET_LOG_WARN, "[SERVER] [NATS] - Failed to parse JSON payload for mysubject1");
        }
    }

    // Envoie le message préparé vers le thread simulation en le poussant dans la queue NATS -> Simulation.
    GetNatsToSimulationQueue().push(outMessage);

    // Libère le message NATS après traitement.
    natsMsg_Destroy(msg);
}