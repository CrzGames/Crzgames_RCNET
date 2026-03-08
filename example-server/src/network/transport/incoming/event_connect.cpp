#include "network/transport/incoming/event_connect.h"

#include <cstdint> // uintptr_t

#include <RCNET/RCNET.h> // RCNET_log

void ServerNetworkIncomingUpdate_HandleConnectEvent(
    const ENetEvent* event,
    NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue)
{
    if (event->peer == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [CONNECT] - Invalid connect event: event->peer == nullptr\n");
        return;
    }

    uint32_t connectionId = networkState.nextConnectionId++;
    event->peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(connectionId));
    networkState.connectionIdToEnetPeer[connectionId] = event->peer;

    NetworkINToSimulationMessage message{};
    message.type = NetworkINToSimulationMessageType::CLIENT_EVENT_CONNECT;
    message.connectionId = connectionId;
    netToSimQueue.push(message);

    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [NETWORK_IN] [CONNECT] - connectionId=%u\n",
              connectionId);
}