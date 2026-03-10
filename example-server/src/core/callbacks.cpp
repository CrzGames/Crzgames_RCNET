#include "core/callbacks.h"

#include "core/context.h"
#include "network/transport/incoming/entrypoint.h"
#include "network/transport/outgoing/entrypoint.h"
#include "simulation/entrypoint.h"
#include "services/http/entrypoint.h"
#include "crypto/kx.h"
#include "services/http/client.h"

#include <RCNET/RCNET.h>

void rcnet_load(void)
{
    // Récupère l'état réseau global du serveur.
    NetworkState& networkState = GetNetworkState();

    // Initialise les clés de cryptographie KX côté serveur.
    // Si l'initialisation échoue, on demande l'arrêt du moteur.
    if (!ServerCryptoKx_Initialize(networkState.cryptoKxState))
    {
        // Demande d'arrêt propre du moteur.
        rcnet_engine_eventQuit();

        // Log explicite de l'erreur pour diagnostic.
        RCNET_log(RCNET_LOG_ERROR, "Failed to initialize server crypto KX state");

        // Sort de la fonction de callback pour éviter de continuer l'initialisation du serveur dans un état potentiellement instable.
        return;
    }

    // Initialise le client HTTP global utilisé par le thread HTTP.
    // Si l'initialisation échoue, on demande l'arrêt du moteur.
    if (!ServerHttp_InitializeClient())
    {
        // Demande d'arrêt propre du moteur.
        rcnet_engine_eventQuit();

        // Log explicite de l'erreur pour diagnostic.
        RCNET_log(RCNET_LOG_ERROR, "Failed to initialize HTTP client");

        // Sort de la fonction de callback pour éviter de continuer l'initialisation du serveur dans un état potentiellement instable.
        return;
    }

    // À ce stade, l'initialisation applicative est terminée.
    // Le serveur peut commencer à accepter et traiter son activité normale.
    RCNET_log(RCNET_LOG_INFO, "Server is ready");
}

void rcnet_unload(void)
{
    // Point de nettoyage applicatif à la fermeture du serveur.
    // Actuellement vide.
}

void rcnet_network_incoming_update(ENetHost* host, const ENetEvent* event)
{
    // Délègue tout le traitement des événements ENet entrants à la couche réseau applicative.
    ServerNetworkIncoming_ProcessENetEvent(host, event);
}

void rcnet_network_outgoing_update(ENetHost* host)
{
    // Demande à la couche réseau sortante de :
    // - drainer les messages produits par simulation
    // - sérialiser / préparer si nécessaire
    // - envoyer les packets via ENet
    ServerNetworkOutgoing_DrainSimulationMessages_And_SendPackets(host);
}

void rcnet_simulation_update(uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt)
{
    // À chaque tick simulation :
    // - on draine les messages entrants (Network IN / HTTP / NATS)
    // - puis on exécute la logique de simulation pour le tick courant
    ServerSimulation_DrainNetworkIncomingAndHttpAndNatsMessages_And_RunSimulationForCurrentTick(
        currentTick,
        serverTimeNs,
        dtNs,
        dt
    );
}

void rcnet_http_update(void)
{
    // Exécute une unité de travail du thread HTTP.
    // Cette fonction peut bloquer en attendant un job depuis la queue Simulation -> HTTP.
    ServerHttp_WaitAndProcessOneSimulationMessage();
}

void rcnet_nats_update(RCNET_NATSContext* natsContext)
{
    // Point d'entrée prévu pour le traitement NATS.
    // Actuellement vide.
    (void)natsContext;
}

void rcnet_wake_blocking_threads(void)
{
    GetSimulationToHttpQueue().stop();
    //GetSimulationToNatsQueue().stop();
}