#include "simulation/process/incoming/snapshot_full_message.h"

#include <RC2D/RC2D.h>

#include <mutex>

void ClientSimulation_ProcessNetworkIncomingDispatcher_HandleSnapshotFullMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& networkInToSimMessage)
{
    // Lire le flag authTokenValidated pour contextualiser le log de snapshot.
    bool authTokenValidated = false;
    {
        std::lock_guard<std::mutex> lock(networkState.cryptoMutex);
        authTokenValidated = networkState.authTokenValidated;
    }

    const ServerSnapshotFullPacketUnreliable& packet = networkInToSimMessage.snapshotFullPacket;
    RC2D_log(
        RC2D_LOG_DEBUG,
        "[CLIENT] [SIMULATION] [SNAPSHOT] id=%u tick=%llu serverTimeNs=%llu lastProcessedInput=%u authTokenOk=%u",
        packet.snapshotId,
        static_cast<unsigned long long>(packet.serverTick),
        static_cast<unsigned long long>(packet.serverTimeNs),
        packet.lastProcessedInputSequenceNumber,
        authTokenValidated ? 1u : 0u);
}

