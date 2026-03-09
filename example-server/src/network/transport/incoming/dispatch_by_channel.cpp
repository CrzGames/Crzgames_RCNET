#include "network/transport/incoming/dispatch_by_channel.h"

#include "network/channels/channel.h"
#include "network/transport/incoming/guards_channels.h"
#include "network/transport/incoming/channels/secure_session_reliable.h"
#include "network/transport/incoming/channels/auth_reliable.h"
#include "network/transport/incoming/channels/game_reliable.h"
#include "network/transport/incoming/channels/game_unreliable.h"

#include <RCNET/RCNET.h>

void ServerNetworkIncoming_DispatchByChannel(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Récupère le channel sur lequel le packet est arrivé.
    const NetworkChannel channel = static_cast<NetworkChannel>(event->channelID);

    // Vérifie que le client est autorisé à envoyer des packets sur ce channel 
    // (ex: pas de packet de gameplay si pas authentifié, pas de packet d’authentification si pas de session sécurisée, etc).
    if (channel == NetworkChannel::GAME_RELIABLE || channel == NetworkChannel::GAME_UNRELIABLE)
    {
        // Pour les channels de gameplay, vérifie que le client est autorisé à envoyer des packets de gameplay.
        // Cela implique que le client doit être authentifié et que la session sécurisée doit être établie.
        if (!ServerNetworkIncoming_IsConnectionAllowedForGameplayChannels(connectionId))
        {
            RCNET_log(RCNET_LOG_WARN,
                      "[SERVER] [NETWORK_IN] [RECEIVE] - ConnectionId=%u is not allowed to send on gameplay channel %u (not authenticated or secure session not established)\n",
                      connectionId,
                      static_cast<unsigned>(channel));
            return;
        }
    }
    else if (channel == NetworkChannel::AUTH_RELIABLE)
    {
        // Pour le channel d’authentification, vérifie que le client est autorisé à envoyer des packets d’authentification.
        // Cela implique que la session sécurisée doit être établie, mais pas forcément que le client soit déjà authentifié.
        if (!ServerNetworkIncoming_IsConnectionAllowedForAuthChannel(connectionId))
        {
            RCNET_log(RCNET_LOG_WARN,
                      "[SERVER] [NETWORK_IN] [RECEIVE] - ConnectionId=%u is not allowed to send on auth channel %u (secure session not established)\n",
                      connectionId,
                      static_cast<unsigned>(channel));
            return;
        }
    }

    // Dispatch le traitement du packet selon le channel ENet utilisé.
    switch (channel)
    {
        case NetworkChannel::SECURE_SESSION_RELIABLE:
            ServerNetworkIncoming_Channel_SecureSessionReliable(
                event, connectionId, netToSimQueue);
            break;

        case NetworkChannel::AUTH_RELIABLE:
            ServerNetworkIncoming_Channel_AuthReliable(
                event, connectionId, netToSimQueue);
            break;

        case NetworkChannel::GAME_RELIABLE:
            ServerNetworkIncoming_Channel_GameReliable(
                event, connectionId, netToSimQueue);
            break;

        case NetworkChannel::GAME_UNRELIABLE:
            ServerNetworkIncoming_Channel_GameUnreliable(
                event, connectionId, netToSimQueue);
            break;

        default:
            RCNET_log(RCNET_LOG_WARN,
                      "[SERVER] [NETWORK_IN] [RECEIVE] - Unknown channel=%u for connectionId=%u\n",
                      static_cast<unsigned>(event->channelID),
                      connectionId);
            break;
    }
}