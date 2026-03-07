#include "server_context.h"
#include "server_network_packets_server_unreliable.h"
#include "server_world.h"
#include "server_debug_network_stats.h"
#include "server_network_packets_server_reliable.h"

#include <RCNET/RCNET.h>

#include <cstdint>         // uintptr_t
#include <cstring>         // memcpy
#include <deque>           // std::deque
#include <unordered_map>   // std::unordered_map

// ======================================================================================
// Internal helpers - Queue draining & message handling
// ======================================================================================

static void ServerSimulationUpdate_DrainNetworkToSimulationQueue_IntoLocalDeque(
    NetworkINToSimulationQueue& netToSimQueue,
    std::deque<NetworkINToSimulationMessage>& messages)
{
    // 2) Drainer la queue (moins de lock)
    netToSimQueue.drain(messages);
}

static void ServerSimulationUpdate_HandleConnectMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& msg)
{
    // Créer une session pour ce client avec cette connectionId
    ClientSession session{};
    session.connectionId = msg.connectionId;

    // Ajouter la session au network state
    networkState.sessions[msg.connectionId] = session;

    // Mettre à jour des propriété de la session
    session.isTransportConnected = true;

    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [CONNECT] - connectionId=%u (session created)\n",
              msg.connectionId);
}

static void ServerSimulationUpdate_HandleDisconnectMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& msg)
{
    // Supprimer la session du client avec cette connectionId
    std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
    if (sit != networkState.sessions.end())
    {
        networkState.sessions.erase(sit);
    }

    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [DISCONNECT] - connectionId=%u (session removed)\n",
              msg.connectionId);
}

static void ServerSimulationUpdate_HandleInputMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& msg)
{
    // Trouver la session du client qui a envoyé cet input
    std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
    if (sit == networkState.sessions.end())
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [SIMULATION] [INPUT] - Received input for unknown connectionId=%u (ignoring)\n",
                  msg.connectionId);
        return;
    }

    // Session trouvée, traiter l'input
    ClientSession& session = sit->second;

    // Garde le dernier input reçu (utile si on n’a rien de neuf ce tick)
    session.latestReceivedInputPacket = msg.inputPacket;

    // Met à jour le dernier snapshot ACKé par le client (pour la reconciliation côté client et pour estimer la latence)
    session.clientLastAckedSnapshotId = msg.inputPacket.lastReceivedSnapshotId;

    // Anti-doublons / ordre
    if (msg.inputPacket.inputSequenceNumber <= session.serverLastProcessedInputSequenceNumber)
    {
        // input déjà traité ou trop vieux
        return;
    }

    // Mettre en queue pour consommation par la simulation (par tick)
    session.pendingInputPacketsQueue.push_back(msg.inputPacket);

    // NOTE : on ne met PAS serverLastProcessedInputSequenceNumber ici
    // parce que "processed" = doit être mis à jour quand l’input est réellement appliqué au monde (pas au moment où il arrive).

    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [INPUT] - queued connectionId=%u seq=%u (queue size=%zu)\n",
              msg.connectionId,
              msg.inputPacket.inputSequenceNumber,
              session.pendingInputPacketsQueue.size());
}

// Fonction helper statique locale au fichier.
// Elle traite un message de type CLIENT_SECURE_SESSION_HELLO_PACKET_RELIABLE
// déjà parsé par la couche réseau puis transféré à la simulation.
//
// Rôle :
// - retrouver la session associée au client
// - récupérer la clé publique client envoyée dans le hello
// - calculer les clés de session serveur (rx / tx) via libsodium
// - mettre à jour l’état de session
// - construire la réponse serveur à renvoyer au client
// - push cette réponse dans la queue Simulation -> Network OUT
static void ServerSimulationUpdate_HandleSecureSessionHelloMessage(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const NetworkINToSimulationMessage& msg)
{
    // Cherche la session correspondant à la connexion source du message.
    std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);

    // Si aucune session n’existe pour cette connectionId, on ne peut pas poursuivre.
    if (sit == networkState.sessions.end())
    {
        // Log d’avertissement : le message secure session a été reçu pour une connexion inconnue.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [SIMULATION] [SECURE_SESSION] - Unknown connectionId=%u\n",
                  msg.connectionId);

        // On abandonne le traitement.
        return;
    }

    // Référence directe vers la session du client.
    ClientSession& session = sit->second;

    // Copie la clé publique du client dans la session.
    // Cela permet au serveur de garder une trace de la clé client associée à cette connexion.
    session.clientPublicKey = msg.secureSessionHelloPacket.clientPublicKey;

    // Buffer qui recevra la clé serveur utilisée pour déchiffrer les messages venant du client.
    // D’après crypto_kx_server_session_keys(), c’est la clé rx côté serveur.
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> serverRxKey{};

    // Buffer qui recevra la clé serveur utilisée pour chiffrer les messages envoyés au client.
    // D’après crypto_kx_server_session_keys(), c’est la clé tx côté serveur.
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> serverTxKey{};

    // Tente de calculer les deux clés de session serveur à partir :
    // - de l’état crypto global du serveur (server public key / secret key)
    // - de la clé publique client reçue dans le packet hello
    //
    // Si la clé publique client est invalide ou suspecte, la fonction retourne false.
    const bool ok = ServerCryptoKx_ComputeSessionKeys(
        networkState.cryptoKxState,
        msg.secureSessionHelloPacket.clientPublicKey,
        serverRxKey,
        serverTxKey);

    // Crée le packet de réponse que le serveur renverra au client.
    ServerSecureSessionHelloResponsePacketReliable responsePacket{};

    // Renseigne le type du packet de réponse.
    responsePacket.header.type = ServerReliablePacketType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE;

    // Si le calcul des clés de session a échoué...
    if (!ok)
    {
        // ... le serveur indique que la clé publique client est invalide ou inacceptable.
        responsePacket.status = ServerSecureSessionHelloResponseStatus::INVALID_CLIENT_KEY;
    }
    else
    {
        // Si le calcul a réussi, on stocke la clé RX serveur dans la session.
        // Elle servira plus tard à déchiffrer les packets reçus du client.
        session.serverRxKey = serverRxKey;

        // Stocke la clé TX serveur dans la session.
        // Elle servira plus tard à chiffrer les packets envoyés au client.
        session.serverTxKey = serverTxKey;

        // Marque la session comme ayant terminé avec succès l’établissement
        // de la session sécurisée côté serveur.
        session.isSecureSessionEstablished = true;

        // Indique dans la réponse que l’opération a réussi.
        responsePacket.status = ServerSecureSessionHelloResponseStatus::SUCCESS;

        // Copie la clé publique du serveur dans le packet de réponse.
        // Le client l’utilisera pour calculer ses propres clés de session
        // avec crypto_kx_client_session_keys().
        responsePacket.serverPublicKey = networkState.cryptoKxState.serverPublicKey;
    }

    // Crée un message sortant simulation -> réseau.
    SimulationToNetworkOUTMessage outMsg{};

    // Renseigne le type de message à envoyer.
    outMsg.type = SimulationToNetworkOUTMessageType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE;

    // Renseigne le destinataire du message (la connexion client concernée).
    outMsg.connectionId = msg.connectionId;

    // Sérialise le packet de réponse serveur en payload binaire prêt à être envoyé par Network OUT.
    outMsg.payload = serializeServerSecureSessionHelloResponsePacketReliable(responsePacket);

    // Push le message dans la queue simulation -> réseau.
    // Le thread réseau se chargera ensuite de l’envoyer via ENet au bon client.
    simToNetQueue.push(outMsg);

    // Log d’information indiquant le résultat du traitement.
    // secureSessionEstablished vaut 1 si la session sécurisée est établie, sinon 0.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [SECURE_SESSION] - connectionId=%u secureSessionEstablished=%u\n",
              msg.connectionId,
              session.isSecureSessionEstablished ? 1u : 0u);
}

static void ServerSimulationUpdate_ProcessIncomingNetworkMessages(
    NetworkState& networkState,
    std::deque<NetworkINToSimulationMessage>& messages)
{
    // 4) Traiter les messages réseau (création/suppression session, input queue, etc.)
    for (std::deque<NetworkINToSimulationMessage>::iterator it = messages.begin();
         it != messages.end();
         ++it)
    {
        NetworkINToSimulationMessage& msg = *it;

        if (msg.type == NetworkINToSimulationMessageType::CLIENT_EVENT_CONNECT)
        {
            ServerSimulationUpdate_HandleConnectMessage(networkState, msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_EVENT_DISCONNECT)
        {
            ServerSimulationUpdate_HandleDisconnectMessage(networkState, msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_INPUT_PACKET_UNRELIABLE)
        {
            ServerSimulationUpdate_HandleInputMessage(networkState, msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_SECURE_SESSION_HELLO_PACKET_RELIABLE)
        {
            ServerSimulationUpdate_HandleSecureSessionHelloMessage(networkState, simToNetQueue, msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_AUTH_PACKET_RELIABLE)
        {
            // Appeler API vers le backend d'authentification pour valider le token envoyé par le client, etc. (pas montré ici), 
            // quand ok set session.isAuthenticated = true pour autoriser les étapes suivantes du flow de gameplay.
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_READY_FOR_MATCH_PACKET_RELIABLE)
        {
            // Marquer la session comme prête pour le match
            std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
            if (sit != networkState.sessions.end())
            {
                ClientSession& session = sit->second;
                session.isReadyForMatch = true;

                RCNET_log(RCNET_LOG_INFO,
                          "[SERVER] [SIMULATION] [READY_FOR_MATCH] - connectionId=%u is ready for match\n",
                          msg.connectionId);
            }
        }
    }
}

// ======================================================================================
// Vérifie si toutes les sessions connectées sont prêtes pour démarrer le match.
//
// Conditions :
// - Le nombre de sessions doit être égal ou supérieur au nombre de joueurs requis.
// - Toutes les sessions doivent avoir envoyé CLIENT_READY_FOR_MATCH.
//
// Retourne true seulement si toutes les sessions sont prêtes.
// ======================================================================================
static bool ServerSimulationUpdate_AreAllSessionsReadyForMatch(const NetworkState& networkState)
{
    // Si le nombre de sessions connectées est inférieur au nombre de joueurs requis,
    // on ne peut pas démarrer le match.
    if (networkState.sessions.size() < networkState.maxClientsConnected)
        return false;

    // Parcourir toutes les sessions actives.
    for (std::unordered_map<uint32_t, ClientSession>::const_iterator it = networkState.sessions.begin();
         it != networkState.sessions.end();
         ++it)
    {
        const ClientSession& session = it->second;

        // Si une seule session n'est pas prête,
        // le match ne peut pas encore démarrer.
        if (!session.isReadyForMatch)
            return false;
    }

    // Si toutes les sessions sont prêtes, on peut lancer la suite du flow.
    return true;
}

// ======================================================================================
// Gère tout le flow de préparation du match côté serveur.
//
// Flow général :
//
// 1) Attendre que tous les joueurs soient connectés
// 2) Envoyer MATCH_INIT (infos générales du match)
// 3) Envoyer WORLD_STATIC_STATE_INIT (état statique du monde)
// 4) Attendre que tous les clients répondent CLIENT_READY_FOR_MATCH
// 5) Envoyer MATCH_START avec un countdown synchronisé
// 6) Démarrer réellement le match lorsque le tick serveur atteint matchStartTick
// ======================================================================================
static void ServerSimulationUpdate_CheckMatchFlow(
    GameState& gameState,
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    uint64_t currentTick)
{
    // --------------------------------------------------------------------------
    // 1) Tant qu'on n'a pas assez de joueurs connectés, on ne fait rien.
    //
    // Exemple :
    // maxClientsConnected = 2
    // sessions.size() = 1
    // => on attend qu'un deuxième joueur arrive.
    // --------------------------------------------------------------------------
    if (networkState.sessions.size() < networkState.maxClientsConnected)
        return;


    // --------------------------------------------------------------------------
    // 2) Envoyer une seule fois le packet MATCH_INIT à tous les clients.
    //
    // Ce packet contient :
    // - informations de map
    // - tickrate serveur
    // - tick serveur actuel
    // - temps serveur monotone
    //
    // Il permet au client de :
    // - charger la map correcte
    // - synchroniser sa timeline avec le serveur
    // --------------------------------------------------------------------------
    if (!gameState.matchInitSent)
    {
        for (std::unordered_map<uint32_t, ClientSession>::iterator it = networkState.sessions.begin();
             it != networkState.sessions.end();
             ++it)
        {
            ClientSession& session = it->second;

            // Construction du packet MATCH_INIT.
            ServerMatchInitPacketReliable matchInitPacket{};
            matchInitPacket.header.type = ServerReliablePacketType::SERVER_MATCH_INIT_PACKET_RELIABLE;

            // TODO : ici tu mettras la vraie map du serveur.
            matchInitPacket.mapName = "ExampleNameMap";
            matchInitPacket.mapVersion = 1;
            matchInitPacket.mapChecksum = 0;

            // Informations temporelles serveur.
            matchInitPacket.serverTick = currentTick;
            matchInitPacket.serverTickRateHz = rcnet_engine_getSimulationTickRateHz();
            matchInitPacket.serverTimeNs = rcnet_engine_getCurrentServerTimeNsMonotonic();

            // Création du message simulation -> réseau.
            SimulationToNetworkOUTMessage msg{};
            msg.type = SimulationToNetworkOUTMessageType::SERVER_MATCH_INIT_PACKET_RELIABLE;
            msg.connectionId = session.connectionId;

            // Copie binaire du packet dans le payload.
            msg.payload.resize(sizeof(ServerMatchInitPacketReliable));
            std::memcpy(msg.payload.data(), &matchInitPacket, sizeof(ServerMatchInitPacketReliable));

            // Push dans la queue pour que le thread réseau l'envoie.
            simToNetQueue.push(msg);
        }

        // On marque que MATCH_INIT a été envoyé pour ne pas le renvoyer.
        gameState.matchInitSent = true;
    }


    // --------------------------------------------------------------------------
    // 3) Envoyer une seule fois WORLD_STATIC_STATE_INIT.
    //
    // Ce packet contient normalement :
    // - seed de génération
    // - spawn points
    // - zones
    // - objets statiques
    //
    // Il sert à synchroniser les éléments statiques du monde entre
    // serveur et clients avant le début du match.
    // --------------------------------------------------------------------------
    if (!gameState.worldStaticStateInitSent)
    {
        for (std::unordered_map<uint32_t, ClientSession>::iterator it = networkState.sessions.begin();
             it != networkState.sessions.end();
             ++it)
        {
            ClientSession& session = it->second;

            ServerWorldStaticStateInitPacketReliable worldStaticStateInitPacket{};
            worldStaticStateInitPacket.header.type = ServerReliablePacketType::SERVER_WORLD_STATIC_STATE_INIT_PACKET_RELIABLE;

            SimulationToNetworkOUTMessage msg{};
            msg.type = SimulationToNetworkOUTMessageType::SERVER_WORLD_STATIC_STATE_INIT_PACKET_RELIABLE;
            msg.connectionId = session.connectionId;

            msg.payload.resize(sizeof(ServerWorldStaticStateInitPacketReliable));
            std::memcpy(msg.payload.data(), &worldStaticStateInitPacket, sizeof(ServerWorldStaticStateInitPacketReliable));

            simToNetQueue.push(msg);
        }

        // On marque que ce packet a été envoyé.
        gameState.worldStaticStateInitSent = true;
    }


    // --------------------------------------------------------------------------
    // 4) Attendre que tous les clients soient prêts.
    //
    // Les clients deviennent prêts lorsqu'ils envoient
    // CLIENT_READY_FOR_MATCH après avoir :
    // - chargé la map
    // - initialisé leur monde
    // - préparé leur simulation locale
    // --------------------------------------------------------------------------
    if (!gameState.matchStartSent && ServerSimulationUpdate_AreAllSessionsReadyForMatch(networkState))
    {
        // Durée du countdown avant démarrage du match (ex : 3 secondes).
        const uint32_t countdownTicks = rcnet_engine_durationMsToTicks(3000);

        // Calcul du tick serveur auquel le match commencera réellement.
        gameState.matchStartTick = currentTick + countdownTicks;

        for (std::unordered_map<uint32_t, ClientSession>::iterator it = networkState.sessions.begin();
             it != networkState.sessions.end();
             ++it)
        {
            ClientSession& session = it->second;

            ServerMatchStartPacketReliable matchStartPacket{};
            matchStartPacket.header.type = ServerReliablePacketType::SERVER_MATCH_START_PACKET_RELIABLE;

            // Tick actuel du serveur.
            matchStartPacket.serverTick = currentTick;

            // Tick futur auquel le match démarrera réellement.
            matchStartPacket.matchStartTick = gameState.matchStartTick;

            // Durée du countdown.
            matchStartPacket.countdownTicks = countdownTicks;

            // Temps serveur monotone.
            matchStartPacket.serverTimeNs = rcnet_engine_getCurrentServerTimeNsMonotonic();

            SimulationToNetworkOUTMessage msg{};
            msg.type = SimulationToNetworkOUTMessageType::SERVER_MATCH_START_PACKET_RELIABLE;
            msg.connectionId = session.connectionId;

            msg.payload.resize(sizeof(ServerMatchStartPacketReliable));
            std::memcpy(msg.payload.data(), &matchStartPacket, sizeof(ServerMatchStartPacketReliable));

            simToNetQueue.push(msg);
        }

        // On marque que le countdown a été envoyé.
        gameState.matchStartSent = true;
    }


    // --------------------------------------------------------------------------
    // 5) Démarrage réel du match.
    //
    // Une fois que le tick serveur atteint matchStartTick,
    // le match commence réellement côté serveur.
    //
    // À partir de ce moment :
    // - la simulation gameplay devient active
    // - les inputs sont appliqués
    // - les entités sont simulées
    // --------------------------------------------------------------------------
    if (!gameState.matchStarted &&
        gameState.matchStartSent &&
        currentTick >= gameState.matchStartTick)
    {
        gameState.matchStarted = true;

        RCNET_log(RCNET_LOG_INFO,
                  "[SERVER] [MATCH] Match started at tick=%llu\n",
                  (unsigned long long)currentTick);
    }
}

static void ServerSimulationUpdate_RunGameplayLogicAndWorldSimulation(GameState& gameState, uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt)
{
    // ================================================================================
    // SIMULER LE MONDE, APPLIQUER LA LOGIQUE DE JEU, ETC.
    // ================================================================================
    ServerWorld_Simulate(gameState, currentTick, serverTimeNs, dtNs, dt);
}   

static bool ServerSimulationUpdate_ShouldBuildAndSendSnapshotsBasedOnNetworkOutgoingRate(uint64_t currentTick)
{
    // ==================================================================================
    // SNAPSHOTS : GATING AU RYTHME NETWORK OUT
    //
    // Objectif :
    // - La SIMULATION tourne à 128Hz
    // - Le NETWORK OUT tourne à 32Hz
    // => On ne construit/envoye des snapshots QUE 32 fois/s (1 toutes les 4 ticks simulation)
    //
    // Avantages :
    // - évite de remplir la queue sim->net inutilement
    // - snapshotId avance au rythme réel d’envoi (propre pour ACK/delta plus tard)
    // - CPU/mémoire plus stables
    // ==================================================================================
    uint32_t simHz = rcnet_engine_getSimulationTickRateHz();
    uint32_t outHz = rcnet_engine_getNetworkOutgoingTickRateHz();
    uint32_t period = rcnet_engine_computeSnapshotPeriodFromRates(simHz, outHz);

    /*RCNET_log(RCNET_LOG_DEBUG,
              "[SERVER] [SIMULATION] currentTick=%llu simHz=%u outHz=%u snapshotPeriod=%u\n",
              (unsigned long long)currentTick,
              simHz,
              outHz,
              period);*/

    // Si on n'est pas sur un tick "d'envoi", on s'arrête ici.
    // (Tu continues évidemment à simuler ton monde au-dessus, mais pas tu ne construis/envoyes de snapshot ce tick)
    if ((period != 0) && ((currentTick % period) != 0))
    {
        return false;
    }

    return true;
}

static void ServerSimulationUpdate_CreateSnapshotFullAndPushToSimulationToNetworkQueue(
    SimulationToNetworkOUTQueue& simToNetQueue,
    ClientSession& session,
    uint64_t currentTick)
{
    // Construire un snapshot full de l'état du monde pour ce client.
    ServerSnapshotFullPacketUnreliable packet{};
    packet.header.type = ServerUnreliablePacketType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE;
    packet.serverTick = currentTick;
    packet.serverTimeNs = rcnet_engine_getCurrentServerTimeNsMonotonic();
    packet.lastProcessedInputSequenceNumber = session.serverLastProcessedInputSequenceNumber;

    // Construire un message de snapshot à envoyer au client via la queue simulation -> réseau
    SimulationToNetworkOUTMessage outMsg{};
    outMsg.type = SimulationToNetworkOUTMessageType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE;
    outMsg.connectionId = session.connectionId;

    outMsg.payload.resize(sizeof(ServerSnapshotFullPacketUnreliable));
    std::memcpy(outMsg.payload.data(), &packet, sizeof(ServerSnapshotFullPacketUnreliable));

    simToNetQueue.push(outMsg);

    // Debug stats
    g_dbg_snapshotsBuilt++;
}

static void ServerSimulationUpdate_BuildSnapshotsForAllSessionsAndEnqueueToNetworkOut(
    SimulationToNetworkOUTQueue& simToNetQueue,
    NetworkState& networkState,
    uint64_t currentTick)
{
    // ==================================================================================
    // Construire des snapshots de l'état du monde pour chaque client
    // et les envoyer via la queue simulation -> réseau
    // ==================================================================================
    for (std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.begin();
         sit != networkState.sessions.end();
         ++sit)
    {
        // Récupérer la session courante dans le tableau de sessions
        ClientSession& session = sit->second;

        ServerSimulationUpdate_CreateSnapshotFullAndPushToSimulationToNetworkQueue(
            simToNetQueue, session, currentTick);
    }
}

// ======================================================================================
// Public entry point called by server_callbacks.cpp
// ======================================================================================

void ServerSimulationUpdate_RunFullSimulationPipelineForCurrentTick(uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt)
{
    // 1) Récupérer la queue réseau -> simulation et la queue simulation -> réseau (pour envoyer des messages à la fin de ce tick)
    NetworkINToSimulationQueue& netToSimQueue = GetNetworkINToSimulationQueue();
    SimulationToNetworkOUTQueue& simToNetQueue = GetSimulationToNetworkOUTQueue();

    // 2) Drainer la queue (moins de lock)
    std::deque<NetworkINToSimulationMessage> messages;
    ServerSimulationUpdate_DrainNetworkToSimulationQueue_IntoLocalDeque(netToSimQueue, messages);

    // 3) Accès au state
    GameState& gameState = GetGameState();
    NetworkState& networkState = GetNetworkState();

    // 4) Traiter les messages réseau (création/suppression session, input queue, etc.)
    ServerSimulationUpdate_ProcessIncomingNetworkMessages(networkState, messages);

    // 5) Gérer le flow de préparation du match (envoi des packets init, attendre que les clients soient prêts, envoyer le packet de démarrage, etc.)
    ServerSimulationUpdate_CheckMatchFlow(gameState, networkState, simToNetQueue, currentTick);

    // 6) Simuler le monde, appliquer la logique de jeu, etc.
    ServerSimulationUpdate_RunGameplayLogicAndWorldSimulation(gameState, currentTick, serverTimeNs, dtNs, dt);

    // 7) Bloquer la construction/envoi des snapshots au rythme du NETWORK OUT (ex: 32Hz) et pas de la SIMULATION (ex: 128Hz)
    if (!ServerSimulationUpdate_ShouldBuildAndSendSnapshotsBasedOnNetworkOutgoingRate(currentTick))
    {
        return;
    }

    // 8) Construire et enqueuer les snapshots (full pour l’instant)
    ServerSimulationUpdate_BuildSnapshotsForAllSessionsAndEnqueueToNetworkOut(simToNetQueue, networkState, currentTick);
}