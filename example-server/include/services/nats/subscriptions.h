#pragma once

#include <RCNET/RCNET.h> // RCNET_NATSContext

// Abonne le thread NATS a tous les sujets utilises par le serveur.
// Cette fonction est prevue pour etre appelee une seule fois au demarrage
// de la boucle NATS.
bool ServerNats_SubscribeAllSubjects(RCNET_NATSContext* natsContext);

