#include "server_callbacks.h"
#include "server_context.h"
#include "server_network_snapshot_packets.h"

#include <RCNET/RCNET.h>

#include <cstdint>         // uintptr_t
#include <cstring>         // memcpy
#include <deque>           // std::deque
#include <unordered_map>   // std::unordered_map
#include <vector>          // std::vector

void rcnet_load(void)
{
}

void rcnet_unload(void)
{
}

void rcnet_network_incoming_update(ENetHost* host, const ENetEvent* event)
{
    // Sécurité : vérifier que les pointeurs ne sont pas nuls avant de les utiliser
    if (host == nullptr || event == nullptr)
        return;

    // Accès au state pour identifier la connexion réseau (connectionId) à partir de event->peer et pour stocker le mapping connectionId <-> ENetPeer*
    NetworkState& networkState = GetNetworkState();

    // Accès à la queue réseau -> simulation pour push des messages à traiter par la simulation (ex: connexion, déconnexion, inputs reçus, etc.)
    NetworkINToSimulationQueue& netToSimQueue = GetNetworkINToSimulationQueue();

    // Traiter les événements réseau (connexion, déconnexion, message reçu)
    if (event->type == ENET_EVENT_TYPE_CONNECT)
    {
        // Générer un connectionId unique pour cette connexion réseau qui vient d'arriver
        uint32_t connectionId = networkState.nextConnectionId++;

        // Associer ce connectionId à event->peer->data pour pouvoir l'identifier lors de futurs événements (inputs, déconnexion, etc.)
        event->peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(connectionId));

        // Stocker le mapping connectionId -> ENetPeer* pour pouvoir envoyer des messages à ce client plus tard
        networkState.connectionIdToEnetPeer[connectionId] = event->peer;

        // Push un message de connexion vers la simulation pour créer une session, etc.
        NetworkINToSimulationMessage message{};
        message.type = NetworkINToSimulationMessageType::CONNECT;
        message.connectionId = connectionId;
        netToSimQueue.push(message);

        RCNET_log(RCNET_LOG_INFO, "[SERVER] [NETWORK_IN] [CONNECT] - connectionId=%u\n", connectionId);
    }
    else if (event->type == ENET_EVENT_TYPE_DISCONNECT ||
             event->type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
    {
        // Identifier la connexion réseau (connectionId) à partir de event->peer->data
        uint32_t connectionId = 0;
        if (event->peer != nullptr && event->peer->data != nullptr)
            connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));

        // Supprimer le mapping connectionId -> ENetPeer*
        networkState.connectionIdToEnetPeer.erase(connectionId);

        // Supprimer event->peer->data pour éviter les problèmes si jamais on reçoit d'autres événements pour ce peer après la déconnexion
        if (event->peer != nullptr)
            event->peer->data = nullptr;

        // Push un message de déconnexion vers la simulation pour nettoyer la session, etc.
        NetworkINToSimulationMessage message{};
        message.type = NetworkINToSimulationMessageType::DISCONNECT;
        message.connectionId = connectionId;
        netToSimQueue.push(message);

        RCNET_log(RCNET_LOG_INFO, "[SERVER] [NETWORK_IN] [DISCONNECT] - connectionId=%u\n", connectionId);
    }
    else if (event->type == ENET_EVENT_TYPE_RECEIVE)
    {
        // Identifier la connexion réseau (connectionId) à partir de event->peer->data
        uint32_t connectionId = 0;
        if (event->peer != nullptr && event->peer->data != nullptr)
            connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));

        // Si connectionId invalide, ignorer ce message reçu (sécurité)
        if (connectionId == 0)
            return;

        // channel 0 =  handshake / auth / encrypt (reliable)
        // channel 1 = inputs (unreliable)
        // channel 2 = snapshots (unreliable)
        // channel 3 = events importants (reliable)
        if (event->channelID == 0)
        {
            // TODO: traiter handshake (token / accountIdDatabase / etc.)
            // Puis push un message HANDSHAKE vers la simulation.
        }
        else if (event->channelID == 1)
        {
            if (event->packet != nullptr && event->packet->dataLength == sizeof(ClientInputCommand))
            {
                RCNET_log(RCNET_LOG_INFO,
                          "[SERVER] [NETWORK_IN] [INPUT] - Packet received from connectionId=%u (size=%u bytes)\n",
                          connectionId,
                          (unsigned)event->packet->dataLength);

                // Initialiser une struct d'input à partir des données du packet reçu
                ClientInputCommand inputCmd{};

                // Copier les données du packet dans notre struct d'input (attention à la taille et à l'ordre des données)
                std::memcpy(&inputCmd, event->packet->data, sizeof(ClientInputCommand));

                // Push un message vers la simulation pour traiter cet input
                NetworkINToSimulationMessage message{};
                message.type = NetworkINToSimulationMessageType::PACKET_INPUT;
                message.connectionId = connectionId;
                message.input = inputCmd;

                netToSimQueue.push(message);
            }
        }
        else if (event->channelID == 2)
        {
            // Pour ce channel, on n'attend rien du client, donc on peut juste ignorer les messages reçus.
        }
        else if (event->channelID == 3)
        {
            // Traiter les messages importants du client (ex: events de gameplay, chat, etc.)
        }
    }
}

void rcnet_network_outgoing_update(ENetHost* host)
{
    if (host == nullptr)
        return;

    // 1) Récupérer la queue simulation -> réseau pour envoyer des messages à la fin de ce tick
    SimulationToNetworkOUTQueue& simToNetQueue = GetSimulationToNetworkOUTQueue();

    // 2) Accès au network state pour récupérer le mapping connectionId -> ENetPeer*
    NetworkState& networkState = GetNetworkState();

    // 3) Drainer la queue simulation -> réseau (moins de lock)
    std::deque<SimulationToNetworkOUTMessage> outMessages;
    simToNetQueue.drain(outMessages);

    // 4) COALESCING : garder uniquement le DERNIER message par client (connectionId)
    //
    // Pourquoi:
    // - Si la simulation pousse plus vite que NET OUT (ou backlog), la queue peut contenir plusieurs snapshots par client.
    // - Envoyer tous les snapshots est inutile : le client veut le dernier état.
    // - Pour delta compression plus tard, tu feras quelque chose de plus fin (ACK + history), mais ce coalescing reste utile.
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage> lastMsgPerConnectionId;

    for (std::deque<SimulationToNetworkOUTMessage>::iterator it = outMessages.begin();
         it != outMessages.end();
         ++it)
    {
        SimulationToNetworkOUTMessage& msg = *it;

        // Remplace l'ancien => on conserve le dernier snapshot de cette connectionId
        lastMsgPerConnectionId[msg.connectionId] = msg;
    }

    // 5) Envoyer uniquement le dernier snapshot par client
    for (std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>::iterator it = lastMsgPerConnectionId.begin();
         it != lastMsgPerConnectionId.end();
         ++it)
    {
        SimulationToNetworkOUTMessage& msg = it->second;

        // Identifier le client (ENetPeer*) à qui envoyer ce message en utilisant msg.connectionId et le mapping dans networkState
        std::unordered_map<uint32_t, ENetPeer*>::iterator pit = networkState.connectionIdToEnetPeer.find(msg.connectionId);
        if (pit == networkState.connectionIdToEnetPeer.end())
            continue;

        // ENetPeer* trouvé pour ce connectionId, envoyer le message à ce client
        ENetPeer* peer = pit->second;
        if (!peer)
            continue;

        // Traiter le message à envoyer en fonction de son type (snapshot full, snapshot delta, event, etc.)
        if (msg.type == SimulationToNetworkOUTMessageType::SNAPSHOT_FULL)
        {
            // channel snapshots (ex: 2)
            const enet_uint8 channelId = 2;

            // Créer un ENetPacket à partir du payload du message (données du snapshot)
            ENetPacket* packet = enet_packet_create(
                msg.payload.data(),
                msg.payload.size(),
                0 // UNRELIABLE pour snapshot full
            );

            // Envoyer le packet à ce client sur le channel approprié
            // Pas vraiment envoyer le packet directement ici, mais plutôt le mettre en queue d'envoi 
            // d'ENet pour qu'il soit envoyé au bon moment (ENet gère ça en interne)
            if (packet)
                enet_peer_send(peer, channelId, packet);

            // Log : ici on lit le header qui est au début du payload
            const SnapshotHeader* header = reinterpret_cast<const SnapshotHeader*>(msg.payload.data());
            RCNET_log(RCNET_LOG_INFO,
                      "[SERVER] [NETWORK_OUT] [SNAPSHOT_FULL] - Sent snapshotId=%u to connectionId=%u (size=%zu bytes)\n",
                      (header != nullptr) ? header->snapshotId : 0u,
                      msg.connectionId,
                      msg.payload.size());
        }
    }
}

void rcnet_simulation_update(uint64_t currentTick)
{
    // 1) Récupérer la queue réseau -> simulation et la queue simulation -> réseau (pour envoyer des messages à la fin de ce tick)
    NetworkINToSimulationQueue& netToSimQueue = GetNetworkINToSimulationQueue();
    SimulationToNetworkOUTQueue& simToNetQueue = GetSimulationToNetworkOUTQueue();

    // 2) Drainer la queue (moins de lock)
    std::deque<NetworkINToSimulationMessage> messages;
    netToSimQueue.drain(messages);

    // 3) Accès au state
    GameState& gameState = GetGameState();
    NetworkState& networkState = GetNetworkState();

    // 4) Traiter les messages réseau (création/suppression session, input queue, etc.)
    for (std::deque<NetworkINToSimulationMessage>::iterator it = messages.begin();
         it != messages.end();
         ++it)
    {
        NetworkINToSimulationMessage& msg = *it;

        if (msg.type == NetworkINToSimulationMessageType::CONNECT)
        {
            // Créer une session pour ce client avec cette connectionId
            ClientSession session{};
            session.connectionId = msg.connectionId;
            session.isAuthenticated = false; // pas encore (handshake)
            session.accountIdDatabase = 0;    // pas encore (handshake)
            session.serverNextSnapshotId = 1;
            session.serverLastSentSnapshotId = 0;
            session.clientLastAckedSnapshotId = 0;
            session.serverLastProcessedInputSequenceNumber = 0;
            session.serverLastAckedInputSequenceNumberToClient = 0;

            // Ajouter la session au network state
            networkState.sessions[msg.connectionId] = session;

            RCNET_log(RCNET_LOG_INFO,
                      "[SERVER] [SIMULATION] [CONNECT] - connectionId=%u (session created)\n",
                      msg.connectionId);
        }
        else if (msg.type == NetworkINToSimulationMessageType::DISCONNECT)
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
        else if (msg.type == NetworkINToSimulationMessageType::PACKET_INPUT)
        {
            // Trouver la session du client qui a envoyé cet input
            std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
            if (sit == networkState.sessions.end())
            {
                RCNET_log(RCNET_LOG_WARN,
                          "[SERVER] [SIMULATION] [INPUT] - Received input for unknown connectionId=%u (ignoring)\n",
                          msg.connectionId);
                continue;
            }

            // Session trouvée, traiter l'input
            ClientSession& session = sit->second;

            // Garde le dernier input reçu (utile si on n’a rien de neuf ce tick)
            session.latestReceivedInputCommand = msg.input;

            // Anti-doublons / ordre
            if (msg.input.inputSequenceNumber <= session.serverLastProcessedInputSequenceNumber)
            {
                // input déjà traité ou trop vieux
                continue;
            }

            // Mettre en queue pour consommation par la simulation (par tick)
            session.pendingInputCommandsQueue.push_back(msg.input);

            // NOTE : on ne met PAS serverLastProcessedInputSequenceNumber ici
            // parce que "processed" = doit être mis à jour quand l’input est réellement appliqué au monde (pas au moment où il arrive).

            RCNET_log(RCNET_LOG_INFO,
                      "[SERVER] [SIMULATION] [INPUT] - queued connectionId=%u seq=%u clientTick=%u (queue size=%zu)\n",
                      msg.connectionId,
                      msg.input.inputSequenceNumber,
                      msg.input.clientTick,
                      session.pendingInputCommandsQueue.size());
        }
        else if (msg.type == NetworkINToSimulationMessageType::PACKET_HANDSHAKE)
        {
            // Plus tard : valider token, set accountIdDatabase, session.isAuthenticated = true, etc.
        }
        else if (msg.type == NetworkINToSimulationMessageType::PACKET_EVENT_IMPORTANT)
        {
            // Traiter les messages importants du client (ex: events de gameplay, chat, etc.)
        }
    }


    // ================================================================================
    // SIMULER LE MONDE, APPLIQUER LA LOGIQUE DE JEU, ETC.
    // ================================================================================


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

    // Si on n'est pas sur un tick "d'envoi", on s'arrête ici.
    // (Tu continues évidemment à simuler ton monde au-dessus, mais pas tu ne construis/envoyes de snapshot ce tick)
    if ((period != 0) && ((currentTick % period) != 0))
    {
        return;
    }

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

        // Générer un snapshotId unique pour ce snapshot (incrémenter le compteur de la session)
        uint32_t snapshotId = session.serverNextSnapshotId++;

        // Mettre à jour le dernier snapshotId envoyé pour cette session
        session.serverLastSentSnapshotId = snapshotId;

        // Construire un snapshot (pour l’instant: header uniquement)
        SnapshotHeader header{};
        header.snapshotId = snapshotId;
        header.serverTick = currentTick;

        // Construire un message de snapshot à envoyer au client via la queue simulation -> réseau
        SimulationToNetworkOUTMessage outMsg{};
        outMsg.type = SimulationToNetworkOUTMessageType::SNAPSHOT_FULL;
        outMsg.connectionId = session.connectionId;

        outMsg.payload.resize(sizeof(SnapshotHeader));
        std::memcpy(outMsg.payload.data(), &header, sizeof(SnapshotHeader));

        simToNetQueue.push(outMsg);
    }
}