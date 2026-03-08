#include "network/transport/incoming/channel_guards.h"

#include "core/context.h"
#include "network/client_session.h"
#include "network/state.h"

#include <unordered_map> // std::unordered_map

static ClientSession* FindSessionByConnectionId(uint32_t connectionId)
{
    NetworkState& networkState = GetNetworkState();

    std::unordered_map<uint32_t, ClientSession>::iterator it =
        networkState.sessions.find(connectionId);

    if (it == networkState.sessions.end())
    {
        return nullptr;
    }

    return &it->second;
}

static bool IsConnectionTransportConnected(uint32_t connectionId)
{
    ClientSession* session = FindSessionByConnectionId(connectionId);
    if (session == nullptr)
    {
        return false;
    }

    return session->isTransportConnected;
}

static bool IsConnectionSecureSessionEstablished(uint32_t connectionId)
{
    ClientSession* session = FindSessionByConnectionId(connectionId);
    if (session == nullptr)
    {
        return false;
    }

    return session->isSecureSessionEstablished;
}

static bool IsConnectionAuthenticated(uint32_t connectionId)
{
    ClientSession* session = FindSessionByConnectionId(connectionId);
    if (session == nullptr)
    {
        return false;
    }

    return session->authStatus == AuthStatus::Valid;
}

bool IsConnectionAllowedForAuthChannel(uint32_t connectionId)
{
    return IsConnectionTransportConnected(connectionId) &&
           IsConnectionSecureSessionEstablished(connectionId);
}

bool IsConnectionAllowedForGameplayChannels(uint32_t connectionId)
{
    return IsConnectionTransportConnected(connectionId) &&
           IsConnectionSecureSessionEstablished(connectionId) &&
           IsConnectionAuthenticated(connectionId);
}