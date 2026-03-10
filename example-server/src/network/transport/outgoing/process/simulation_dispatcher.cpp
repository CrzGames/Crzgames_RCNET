#include "network/transport/outgoing/process/simulation_dispatcher.h"

#include "network/transport/outgoing/process/simulation/reliable_messages.h"
#include "network/transport/outgoing/process/simulation/unreliable_messages.h"

void ServerNetworkOutgoing_ProcessSimulationDispatcher(
    NetworkState& networkState,
    const ServerNetworkOutgoingPreparedMessages& preparedMessages)
{
    // Traiter d'abord les messages reliable.
    ServerNetworkOutgoing_ProcessSimulationDispatcher_HandleReliableMessages(
        networkState,
        preparedMessages.reliableMessages);

    // Traiter ensuite les messages unreliable.
    ServerNetworkOutgoing_ProcessSimulationDispatcher_HandleUnreliableMessages(
        networkState,
        preparedMessages.lastSnapshotFullUnreliablePerConnectionId,
        preparedMessages.lastClockSyncUnreliablePerConnectionId);
}
