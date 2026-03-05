#include "server_callbacks.h"
#include "server_context.h"
#include "server_network_snapshot_packets.h"

#include <RCNET/RCNET.h>

#include <cstdint> // uintptr_t
#include <cstring> // memcpy
#include <deque>   // std::deque
#include <unordered_map> // std::unordered_map
#include <vector>

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
                RCNET_log(RCNET_LOG_DEBUG, "[SERVER] [NETWORK_IN] [INPUT] - Packet received from connectionId=%u (size=%u bytes)\n",
                          connectionId,
                          (unsigned)event->packet->dataLength);

                // Initialiser une struct d'input à partir des données du packet reçu
                ClientInputCommand inputCmd{};

                // Copier les données du packet dans notre struct d'input (attention à la taille et à l'ordre des données)
                std::memcpy(&inputCmd, event->packet->data, sizeof(ClientInputCommand));

                // Push un message vers la simulation pour traiter cet input
                NetworkINToSimulationMessage message{};
                message.type = NetworkINToSimulationMessageType::INPUT;
                message.connectionId = connectionId;
                message.input = inputCmd;

                netToSimQueue.push(message);
            }
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

    // 4) Traiter les messages à envoyer au réseau
    for (std::deque<SimulationToNetworkOUTMessage>::iterator it = outMessages.begin();
         it != outMessages.end();
         ++it)
    {
        SimulationToNetworkOUTMessage& msg = *it;

        // Identifier le client (ENetPeer*) à qui envoyer ce message en utilisant msg.connectionId et le mapping dans networkState
        std::unordered_map<uint32_t, ENetPeer*>::iterator pit = networkState.connectionIdToEnetPeer.find(msg.connectionId);
        if (pit == networkState.connectionIdToEnetPeer.end())
            continue;

        // ENetPeer* trouvé pour ce connectionId, envoyer le message à ce client
        ENetPeer* peer = pit->second;
        if (!peer)
            continue;

        // Traiter le message à envoyer en fonction de son type (snapshot full, delta, event, etc.)
        if (msg.type == SimulationToNetworkOUTMessageType::SNAPSHOT_FULL)
        {
            // channel snapshots (ex: 2)
            const enet_uint8 channelId = 2;

            ENetPacket* packet = enet_packet_create(
                msg.payload.data(),
                msg.payload.size(),
                0 // UNRELIABLE pour snapshot
            );

            if (packet)
                enet_peer_send(peer, channelId, packet);

            RCNET_log(RCNET_LOG_DEBUG, "[SERVER] [NETWORK_OUT] [SNAPSHOT_FULL] - Sent snapshotId=%u to connectionId=%u (size=%zu bytes)\n",
                      ((SnapshotHeader*)msg.payload.data())->snapshotId,
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

    // 4) Traiter les messages
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
            session.accountIdDatabase = 0; // pas encore (handshake)
            session.serverNextSnapshotId = 1;
            session.serverLastSentSnapshotId = 0;
            session.clientLastAckedSnapshotId = 0;
            session.serverLastProcessedInputSequenceNumber = 0;
            session.serverLastAckedInputSequenceNumberToClient = 0;

            // Ajouter la session au network state
            networkState.sessions[msg.connectionId] = session;

            RCNET_log(RCNET_LOG_INFO, "[SERVER] [SIMULATION] [CONNECT] - connectionId=%u (session created)\n", msg.connectionId);
        }
        else if (msg.type == NetworkINToSimulationMessageType::DISCONNECT)
        {
            // Supprimer la session du client avec cette connectionId
            std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
            if (sit != networkState.sessions.end())
            {
                networkState.sessions.erase(sit);
            }

            RCNET_log(RCNET_LOG_INFO, "[SERVER] [SIMULATION] [DISCONNECT] - connectionId=%u (session removed)\n", msg.connectionId);
        }
        else if (msg.type == NetworkINToSimulationMessageType::INPUT)
        {
            // Trouver la session du client qui a envoyé cet input
            std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
            if (sit == networkState.sessions.end())
            {
                RCNET_log(RCNET_LOG_WARN, "[SERVER] [SIMULATION] [INPUT] - Received input for unknown connectionId=%u (ignoring)\n", msg.connectionId);
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

            RCNET_log(RCNET_LOG_DEBUG,
                      "[SERVER] [SIMULATION] [INPUT] - queued connectionId=%u seq=%u clientTick=%u (queue size=%zu)\n",
                      msg.connectionId,
                      msg.input.inputSequenceNumber,
                      msg.input.clientTick,
                      session.pendingInputCommandsQueue.size());
        }
        else if (msg.type == NetworkINToSimulationMessageType::HANDSHAKE)
        {
            // Plus tard : valider token, set accountIdDatabase, session.isAuthenticated = true, etc.
        }
    }

    //RCNET_log(RCNET_LOG_DEBUG, "Simulation tick %llu\n", currentTick);


    // Ensuite: appliquer inputs dans le monde (ex: move player)
    // Exemple simplifié : pour chaque session, consommer 0..N inputs
    // (souvent tu consommes jusqu'à "le plus récent <= currentTick" ou juste 1 par tick)


    // A la fin du tick, construire des snapshots de l'état du monde pour chaque client et les envoyer via la queue simulation -> réseau
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

        // Construire un snapshot de l'état du monde pour ce client (ex: position de tous les joueurs)
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