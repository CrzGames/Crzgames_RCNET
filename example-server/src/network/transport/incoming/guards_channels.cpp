#include "network/transport/incoming/guards_channels.h"

#include "core/context.h"
#include "network/state.h"
#include "simulation/session/client.h"

#include <mutex>         // std::lock_guard
#include <unordered_map> // std::unordered_map

static bool ServerNetworkIncoming_GetSessionFlags(
    uint32_t connectionId,
    bool& isTransportConnected,
    bool& isSecureSessionEstablished,
    bool& isPacketEncryptionEnabled,
    AuthStatus& authStatus)
{
    const NetworkState& networkState = GetNetworkState();

    std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

    std::unordered_map<uint32_t, ClientSession>::const_iterator it =
        networkState.sessions.find(connectionId);
    if (it == networkState.sessions.end())
    {
        return false;
    }

    const ClientSession& session = it->second;
    isTransportConnected = session.isTransportConnected;
    isSecureSessionEstablished = session.isSecureSessionEstablished;
    isPacketEncryptionEnabled = session.isPacketEncryptionEnabled;
    authStatus = session.authStatus;

    return true;
}

bool ServerNetworkIncoming_IsConnectionAllowedForAuthChannel(uint32_t connectionId)
{
    bool isTransportConnected = false;
    bool isSecureSessionEstablished = false;
    bool isPacketEncryptionEnabled = false;
    AuthStatus authStatus = AuthStatus::None;

    if (!ServerNetworkIncoming_GetSessionFlags(
            connectionId,
            isTransportConnected,
            isSecureSessionEstablished,
            isPacketEncryptionEnabled,
            authStatus))
    {
        return false;
    }

    return isTransportConnected &&
           isSecureSessionEstablished &&
           isPacketEncryptionEnabled &&
           authStatus != AuthStatus::Invalid;
}

bool ServerNetworkIncoming_IsConnectionAllowedForGameplayChannels(uint32_t connectionId)
{
    bool isTransportConnected = false;
    bool isSecureSessionEstablished = false;
    bool isPacketEncryptionEnabled = false;
    AuthStatus authStatus = AuthStatus::None;

    if (!ServerNetworkIncoming_GetSessionFlags(
            connectionId,
            isTransportConnected,
            isSecureSessionEstablished,
            isPacketEncryptionEnabled,
            authStatus))
    {
        return false;
    }

    return isTransportConnected &&
           isSecureSessionEstablished &&
           isPacketEncryptionEnabled &&
           authStatus == AuthStatus::Valid;
}
