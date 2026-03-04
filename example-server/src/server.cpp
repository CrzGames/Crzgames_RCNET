#include "server_callbacks.h"
#include "server_context.h"

#include <RCNET/RCNET.h>

#include <cstdint> // uintptr_t
#include <cstring> // memcpy
#include <deque>   // std::deque

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

    // 4) Traiter les messages
    for (std::deque<NetworkToSimulationMessage>::iterator it = messages.begin();
         it != messages.end();
         ++it)
    {
        NetworkToSimulationMessage& msg = *it;

        if (msg.type == NetworkMessageType::CONNECT)
        {
            // Exemple: enregistrer connectionId -> accountId (0 tant que non authentifié)
            // gameState.connectionToAccount[msg.connectionId] = 0;
            RCNET_log(RCNET_LOG_DEBUG, "Simulation received CONNECT message. connectionId=%u\n", msg.connectionId);
        }
        else if (msg.type == NetworkMessageType::DISCONNECT)
        {
            // Exemple: cleanup
            // gameState.connectionToAccount.erase(msg.connectionId);
            RCNET_log(RCNET_LOG_DEBUG, "Simulation received DISCONNECT message. connectionId=%u\n", msg.connectionId);
        }
        else if (msg.type == NetworkMessageType::INPUT)
        {
            // Ici tu feras:
            // connectionId -> accountIdDatabase -> session -> pendingInputCommandsQueue
            RCNET_log(RCNET_LOG_DEBUG, "Simulation received INPUT message. connectionId=%u, input.sequenceNumber=%u; input.clientTick=%u\n",
                      msg.connectionId, msg.input.inputSequenceNumber, msg.input.clientTick);
        }
        else if (msg.type == NetworkMessageType::HANDSHAKE)
        {
            // Ici tu associeras connectionId <-> accountIdDatabase (après validation)
        }
    }

    RCNET_log(RCNET_LOG_DEBUG, "Simulation tick %llu\n", currentTick);
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
        message.type = NetworkMessageType::CONNECT;
        message.connectionId = connectionId;
        message.accountIdDatabase = 0;
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
        message.type = NetworkMessageType::DISCONNECT;
        message.connectionId = connectionId;
        message.accountIdDatabase = 0;
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
                message.type = NetworkMessageType::INPUT;
                message.connectionId = connectionId;
                message.accountIdDatabase = 0; // pas encore utile ici
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