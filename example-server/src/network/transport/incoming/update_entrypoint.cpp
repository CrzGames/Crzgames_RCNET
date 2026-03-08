#include "network/transport/incoming/update_entrypoint.h"

#include "core/context.h"
#include "network/transport/incoming/events/connect.h"
#include "network/transport/incoming/events/disconnect.h"
#include "network/transport/incoming/events/receive.h"

void ServerNetworkIncomingUpdate_ProcessENetEvent(ENetHost* host, const ENetEvent* event)
{
    // Vérifie que les pointeurs d’entrée sont valides.
    if (host == nullptr || event == nullptr)
        return;

    (void)host;

    // Récupère la référence vers l’état du réseau.
    NetworkState& networkState = GetNetworkState();

    // Récupère la référence vers la file d’attente des messages du réseau vers la simulation.
    NetworkINToSimulationQueue& netToSimQueue = GetNetworkINToSimulationQueue();

    // Vérifie si l’événement est une connexion.
    if (event->type == ENET_EVENT_TYPE_CONNECT)
    {
        // Traite l’événement de connexion.
        ServerNetworkIncomingUpdate_HandleConnectEvent(event, networkState, netToSimQueue);
    }
    // Vérifie si l’événement est une déconnexion normale faite par le client ou un client qui ne répond plus (timeout).
    else if (event->type == ENET_EVENT_TYPE_DISCONNECT || event->type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
    {
        // Traite l’événement de déconnexion.
        ServerNetworkIncomingUpdate_HandleDisconnectEvent(event, networkState, netToSimQueue);
    }
    // Vérifie si l’événement est une réception de packet.
    else if (event->type == ENET_EVENT_TYPE_RECEIVE)
    {
        // Traite l’événement de réception de packet.
        ServerNetworkIncomingUpdate_HandleReceiveEvent(event, networkState, netToSimQueue);
    }
}