#include "network/transport/incoming/events/receive.h"

#include "network/transport/incoming/connection_validation.h"
#include "network/transport/incoming/channels/dispatch_by_channel.h"

void ServerNetworkIncomingUpdate_HandleReceiveEvent(
    const ENetEvent* event,
    const NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Récupère l’identifiant de connexion validé ou zéro si la validation a échoué.
    uint32_t connectionId = ServerNetworkIncomingUpdate_GetValidatedConnectionIdOrZero(event, networkState);

    // Si l’ID est invalide, on ignore le packet.
    if (connectionId == 0)
        return;

    // Dispatch le traitement du packet selon le channel ENet utilisé.
    ServerNetworkIncomingUpdate_HandleReceiveEvent_DispatchByChannel(
        event,
        connectionId,
        netToSimQueue);
}