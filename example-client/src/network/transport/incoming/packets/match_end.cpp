#include "network/transport/incoming/packets/match_end.h"

#include <RC2D/RC2D.h>

#include <mutex>

void ClientNetworkIncoming_HandlePacket_MatchEnd(
    const ENetEvent* event,
    NetworkINToSimulationQueue& netToSimQueue)
{
    size_t pendingMessages = 0;
    {
        std::lock_guard<std::mutex> lock(netToSimQueue.mtx);
        pendingMessages = netToSimQueue.q.size();
    }

    RC2D_log(
        RC2D_LOG_DEBUG,
        "[CLIENT] [NETWORK_IN] [MATCH_END] Packet received (size=%u bytes, pendingSimQueue=%llu). Handler TODO.",
        static_cast<unsigned>(event->packet->dataLength),
        static_cast<unsigned long long>(pendingMessages));
}

