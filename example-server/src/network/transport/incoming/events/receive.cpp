#include "network/transport/incoming/events/receive.h"

#include "network/transport/incoming/connection_validation.h"
#include "network/transport/incoming/dispatch_by_channel.h"

void ServerNetworkIncoming_Event_HandleReceive(
    const ENetEvent* event,
    const NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Récupère l’identifiant de connexion validé ou zéro si la validation a échoué.
    const uint32_t connectionId = ServerNetworkIncoming_GetValidatedConnectionIdOrZero(event, networkState);

    // Si l’ID est invalide, on ignore le packet.
    if (connectionId == 0)
        return;

    // Dispatch le traitement du packet selon le channel ENet utilisé.
    ServerNetworkIncoming_DispatchByChannel(
        event,
        connectionId,
        netToSimQueue);
}
