#include "server_authoritative.h"

#include <RCNET/RCNET.h>

void rcnet_load(void)
{

}

void rcnet_unload(void)
{

}

void rcnet_simulation_update(uint64_t currentTick)
{
    RCNET_log(RCNET_LOG_DEBUG, "Simulation tick %llu\n", currentTick);
}

void rcnet_network_incoming_update(ENetHost* host, const ENetEvent* event)
{
    if (host == nullptr || event == nullptr)
        return;

    if (event->type == ENET_EVENT_TYPE_CONNECT)
    {
        RCNET_log(RCNET_LOG_INFO, "Client connecté : %x:%u\n", event->peer->address.host, event->peer->address.port);
    }
    else if (event->type == ENET_EVENT_TYPE_DISCONNECT)
    {
        RCNET_log(RCNET_LOG_INFO, "Client déconnecté : %x:%u\n", event->peer->address.host, event->peer->address.port);
    }
    else if (event->type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
    {
        RCNET_log(RCNET_LOG_INFO, "Client timeout : %x:%u\n", event->peer->address.host, event->peer->address.port);
    }
    else if (event->type == ENET_EVENT_TYPE_RECEIVE)
    {
        RCNET_log(RCNET_LOG_DEBUG, "Packet reçu du client %x:%u, channel %u, length %zu bytes\n",
                  event->peer->address.host, event->peer->address.port, event->channelID, event->packet->dataLength);
    }
}

void rcnet_network_outgoing_update(void)
{

}