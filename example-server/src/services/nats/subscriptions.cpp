#include "services/nats/subscriptions.h"

#include "services/nats/on_message_received_for_subject/mysubject1.h"

bool ServerNats_SubscribeAllSubjects(RCNET_NATSContext* natsContext)
{
    if (!rcnet_nats_subscribe(natsContext, "mysubject1", ServerNats_OnMessageReceivedForSubjectMySubject1, nullptr))
    {
        // Déjà logué dans rcnet_nats_subscribe, pas besoin de re-loguer ici.
        return false;
    }

    // Ajouter d'autres abonnements à d'autres sujets ici si nécessaire, en suivant le même modèle que ci-dessus.

    return true;
}