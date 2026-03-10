#include "network/transport/incoming/guards_channels.h"

#include "core/context.h"
#include "simulation/session/client.h"
#include "network/state.h"

#include <unordered_map> // std::unordered_map

static ClientSession* ServerNetworkIncoming_FindSessionByConnectionId(uint32_t connectionId)
{
    // Récupère une référence au NetworkState global.
    NetworkState& networkState = GetNetworkState();

    // Cherche la session client associée à l'identifiant de connexion dans la map des sessions.
    std::unordered_map<uint32_t, ClientSession>::iterator it = networkState.sessions.find(connectionId);

    // Si aucune session n'est trouvée pour cet ID de connexion, retourne nullptr.
    if (it == networkState.sessions.end())
    {
        return nullptr;
    }

    // Retourne un pointeur vers la session client trouvée.
    return &it->second;
}

static bool ServerNetworkIncoming_IsConnectionTransportConnected(uint32_t connectionId)
{
    // Cherche la session client associée à l'identifiant de connexion.
    ClientSession* session = ServerNetworkIncoming_FindSessionByConnectionId(connectionId);

    // Si aucune session n'est trouvée, considère que la connexion n'est pas valide (pas connectée).
    if (session == nullptr)
    {
        return false;
    }

    // Retourne l'état de connexion transport de la session.
    return session->isTransportConnected;
}

static bool ServerNetworkIncoming_IsConnectionSecureSessionEstablished(uint32_t connectionId)
{
    // Cherche la session client associée à l'identifiant de connexion.
    ClientSession* session = ServerNetworkIncoming_FindSessionByConnectionId(connectionId);

    // Si aucune session n'est trouvée, considère que la connexion n'est pas valide (pas de session sécurisée établie).
    if (session == nullptr)
    {
        return false;
    }

    // Retourne l'état de session sécurisée établie de la session.
    return session->isSecureSessionEstablished;
}

static bool ServerNetworkIncoming_IsConnectionAuthenticated(uint32_t connectionId)
{
    // Cherche la session client associée à l'identifiant de connexion.
    ClientSession* session = ServerNetworkIncoming_FindSessionByConnectionId(connectionId);

    // Si aucune session n'est trouvée, considère que la connexion n'est pas valide (pas authentifiée).
    if (session == nullptr)
    {
        return false;
    }

    // Retourne true si le statut d'authentification de la session est "Valid", indiquant que la connexion est authentifiée.
    return session->authStatus == AuthStatus::Valid;
}

bool ServerNetworkIncoming_IsConnectionAllowedForAuthChannel(uint32_t connectionId)
{
    // Le channel auth est autorise seulement si la connexion transport est active,
    // la session secure est etablie et l'auth n'a pas deja ete invalidee.
    ClientSession* session = ServerNetworkIncoming_FindSessionByConnectionId(connectionId);
    if (session == nullptr)
    {
        return false;
    }

    return session->isTransportConnected &&
           session->isSecureSessionEstablished &&
           session->authStatus != AuthStatus::Invalid;
}

bool ServerNetworkIncoming_IsConnectionAllowedForGameplayChannels(uint32_t connectionId)
{
    return ServerNetworkIncoming_IsConnectionTransportConnected(connectionId) &&
           ServerNetworkIncoming_IsConnectionSecureSessionEstablished(connectionId) &&
           ServerNetworkIncoming_IsConnectionAuthenticated(connectionId);
}
