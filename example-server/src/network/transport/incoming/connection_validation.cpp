#include "network/transport/incoming/connection_validation.h"

#include <cstdint>
#include <unordered_map>
#include <RCNET/RCNET.h>

uint32_t ServerNetworkIncomingUpdate_GetValidatedConnectionIdOrZero(
    const ENetEvent* event,
    const NetworkState& networkState)
{
    if (event->peer == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - event->peer == nullptr\n");
        return 0;
    }

    if (event->peer->data == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - peer->data == nullptr\n");
        return 0;
    }

    const uint32_t connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));

    if (connectionId == 0)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - connectionId == 0\n");
        return 0;
    }

    std::unordered_map<uint32_t, ENetPeer*>::const_iterator it = networkState.connectionIdToEnetPeer.find(connectionId);

    if (it == networkState.connectionIdToEnetPeer.end())
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - Unknown connectionId=%u (not found in connectionIdToEnetPeer)\n",
                  connectionId);
        return 0;
    }

    if (it->second != event->peer)
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - Peer mismatch for connectionId=%u (map peer=%p, event peer=%p)\n",
                  connectionId,
                  static_cast<void*>(it->second),
                  static_cast<void*>(event->peer));
        return 0;
    }

    return connectionId;
}