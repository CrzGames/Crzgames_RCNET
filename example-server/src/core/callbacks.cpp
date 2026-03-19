#include "core/callbacks.h"

#include "core/context.h"
#include "network/transport/incoming/entrypoint.h"
#include "network/transport/encryption/enet_host_xchacha20poly1305_encryptor.h"
#include "network/transport/compression/enet_host_lz4_compressor.h"
#include "network/transport/outgoing/entrypoint.h"
#include "simulation/entrypoint.h"
#include "services/http/entrypoint.h"
#include "services/nats/entrypoint.h"
#include "crypto/kx.h"
#include "crypto/signing.h"
#include "services/http/client.h"

#include <RCNET/RCNET.h>

void rcnet_unload(void)
{
    // Nettoie le client HTTP global.
    ServerHttp_ShutdownClient();
}

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

    // Initialise l'identite Ed25519 utilisee pour signer la cle KX du boot.
    if (!ServerCryptoSigning_Initialize(networkState.cryptoSigningState))
    {
        rcnet_engine_eventQuit();
        RCNET_log(RCNET_LOG_ERROR, "Failed to initialize server crypto signing state");
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

void rcnet_http_update(void)
{
    // Exécute une passe du thread HTTP :
    // - bloque jusqu'a avoir au moins un job
    // - draine un batch de jobs Simulation -> HTTP
    // - traite le batch hors lock
    ServerHttp_WaitAndProcessOneSimulationMessage_And_RunHttpLogic();
}

void rcnet_network_host_setup(ENetHost* host)
{
    if (host == nullptr)
    {
        return;
    }

    // Installe l'encryptor au niveau host une seule fois juste après enet_host_create.
    ServerNetworkEncryption_EnsureHostEncryptorInstalled(host);

    // Installe le compresseur au niveau host une seule fois juste après enet_host_create.
    ServerNetworkCompression_EnsureHostCompressorInstalled(host);
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
    ServerNetworkOutgoing_DrainSimulationMessages_And_RunOutgoingNetworkLogic(host);
}

void rcnet_simulation_update(uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt)
{
    // À chaque tick simulation :
    // - on draine les messages entrants (Network IN / HTTP / NATS)
    // - puis on exécute la logique de simulation pour le tick courant
    ServerSimulation_DrainNetworkIncomingAndHttpAndNatsMessages_And_RunSimulationLogic(
        currentTick,
        serverTimeNs,
        dtNs,
        dt
    );
}

void rcnet_nats_update(RCNET_NATSContext* natsContext)
{
    // Exécute une passe du thread NATS :
    // - bloque jusqu'a avoir au moins un job
    // - draine un batch de jobs Simulation -> NATS
    // - traite le batch hors lock
    ServerNats_WaitAndProcessOneSimulationMessage_And_RunNatsLogic(natsContext);
}

void rcnet_wake_blocking_threads(void)
{
    // Demande aux threads bloqués sur les queues Simulation -> HTTP 
    // et Simulation -> NATS de se réveiller pour terminer proprement.
    GetSimulationToHttpQueue().stop();
    GetSimulationToNatsQueue().stop();
}
