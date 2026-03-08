#include "network/transport/incoming/event_disconnect.h"

#include <cstdint>
#include <RCNET/RCNET.h>

void ServerNetworkIncomingUpdate_HandleDisconnectEvent(
    const ENetEvent* event,
    NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue)
{
    if (event->peer == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [DISCONNECT] - Invalid disconnect event: event->peer == nullptr\n");
        return;
    }

    if (event->peer->data == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [DISCONNECT] - Invalid disconnect event: peer->data == nullptr\n");
        return;
    }

    uint32_t connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));

    networkState.connectionIdToEnetPeer.erase(connectionId);
    event->peer->data = nullptr;

    NetworkINToSimulationMessage message{};
    message.type = NetworkINToSimulationMessageType::CLIENT_EVENT_DISCONNECT;
    message.connectionId = connectionId;
    netToSimQueue.push(message);

    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [NETWORK_IN] [DISCONNECT] - connectionId=%u\n",
              connectionId);
}