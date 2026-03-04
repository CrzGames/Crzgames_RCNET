#include "server_callbacks.h"
#include "server_context.h"

#include <RCNET/RCNET.h>

#include <cstdint> // uintptr_t
#include <cstring> // memcpy
#include <deque>   // std::deque
#include <unordered_map> // std::unordered_map

void rcnet_load(void)
{

}

void rcnet_unload(void)
{
}

void rcnet_simulation_update(uint64_t currentTick)
{
    // 1) Récupérer la queue réseau -> simulation
    NetworkToSimulationQueue& netToSimQueue = GetNetToSimQueue();

    // 2) Drainer la queue (moins de lock)
    std::deque<NetworkToSimulationMessage> messages;
    netToSimQueue.drain(messages);

    // 3) Accès au state
    GameState& gameState = GetGameState();
    NetworkState& networkState = GetNetworkState();

    // 4) Traiter les messages
    for (std::deque<NetworkToSimulationMessage>::iterator it = messages.begin();
         it != messages.end();
         ++it)
    {
        NetworkToSimulationMessage& msg = *it;

        if (msg.type == NetworkToSimulationMessageType::CONNECT)
        {
            // Créer une session pour ce client avec cette connectionId
            ClientSession session{};
            session.connectionId = msg.connectionId;
            session.accountIdDatabase = 0; // pas encore (handshake)
            session.serverNextSnapshotId = 1;
            session.serverLastSentSnapshotId = 0;
            session.clientLastAckedSnapshotId = 0;
            session.serverLastProcessedInputSequenceNumber = 0;
            session.serverLastAckedInputSequenceNumberToClient = 0;

            // Ajouter la session au network state
            networkState.sessions[msg.connectionId] = session;

            RCNET_log(RCNET_LOG_INFO, "[SIM] CONNECT connectionId=%u (session created)\n", msg.connectionId);
        }
        else if (msg.type == NetworkToSimulationMessageType::DISCONNECT)
        {
            // Supprimer la session du client avec cette connectionId
            std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
            if (sit != networkState.sessions.end())
            {
                networkState.sessions.erase(sit);
            }

            RCNET_log(RCNET_LOG_INFO, "[SIM] DISCONNECT connectionId=%u (session removed)\n", msg.connectionId);
        }
        else if (msg.type == NetworkToSimulationMessageType::INPUT)
        {
            // Trouver la session du client qui a envoyé cet input
            std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
            if (sit == networkState.sessions.end())
            {
                RCNET_log(RCNET_LOG_WARN, "[SIM] INPUT for unknown connectionId=%u (ignored)\n", msg.connectionId);
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
                      "[SIM] INPUT queued connectionId=%u seq=%u clientTick=%u (queue size=%zu)\n",
                      msg.connectionId,
                      msg.input.inputSequenceNumber,
                      msg.input.clientTick,
                      session.pendingInputCommandsQueue.size());
        }
        else if (msg.type == NetworkToSimulationMessageType::HANDSHAKE)
        {
            // Plus tard : valider token, set accountIdDatabase, session.isAuthenticated = true, etc.
        }
    }

    RCNET_log(RCNET_LOG_DEBUG, "Simulation tick %llu\n", currentTick);

    // Ensuite: appliquer inputs dans le monde (ex: move player)
    // Exemple simplifié : pour chaque session, consommer 0..N inputs
    // (souvent tu consommes jusqu'à "le plus récent <= currentTick" ou juste 1 par tick)
}

void rcnet_network_incoming_update(ENetHost* host, const ENetEvent* event)
{
    if (host == nullptr || event == nullptr)
        return;

    GameState& gameState = GetGameState();
    NetworkState& networkState = GetNetworkState();
    NetworkToSimulationQueue& netToSimQueue = GetNetToSimQueue();

    if (event->type == ENET_EVENT_TYPE_CONNECT)
    {
        uint32_t connectionId = networkState.nextConnectionId++;
        event->peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(connectionId));

        NetworkToSimulationMessage message{};
        message.type = NetworkToSimulationMessageType::CONNECT;
        message.connectionId = connectionId;
        netToSimQueue.push(message);

        RCNET_log(RCNET_LOG_INFO, "Client connecté. connectionId=%u\n", connectionId);
    }
    else if (event->type == ENET_EVENT_TYPE_DISCONNECT ||
             event->type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
    {
        uint32_t connectionId = 0;
        if (event->peer != nullptr && event->peer->data != nullptr)
            connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));

        NetworkToSimulationMessage message{};
        message.type = NetworkToSimulationMessageType::DISCONNECT;
        message.connectionId = connectionId;
        netToSimQueue.push(message);

        RCNET_log(RCNET_LOG_INFO, "Client déconnecté. connectionId=%u\n", connectionId);
    }
    else if (event->type == ENET_EVENT_TYPE_RECEIVE)
    {
        // Identifier la connexion réseau (connectionId) à partir de event->peer->data
        uint32_t connectionId = 0;
        if (event->peer != nullptr && event->peer->data != nullptr)
            connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));

        // channel 0 = handshake, channel 1 = inputs
        if (event->channelID == 1)
        {
            if (event->packet != nullptr && event->packet->dataLength == sizeof(ClientInputCommand))
            {
                ClientInputCommand cmd{};
                std::memcpy(&cmd, event->packet->data, sizeof(ClientInputCommand));

                NetworkToSimulationMessage message{};
                message.type = NetworkToSimulationMessageType::INPUT;
                message.connectionId = connectionId;
                message.input = cmd;

                netToSimQueue.push(message);
            }
        }
        else if (event->channelID == 0)
        {
            // TODO: traiter handshake (token / accountIdDatabase / etc.)
            // Puis push un message HANDSHAKE vers la simulation.
        }
    }
}

void rcnet_network_outgoing_update(void)
{

}