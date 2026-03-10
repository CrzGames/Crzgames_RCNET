#include "network/transport/outgoing/enet_send_packets.h"
#include "network/channels/channel.h" // NetworkChannel
#include "core/context.h"

#include <cstdint>       // uintptr_t
#include <mutex>         // std::lock_guard
#include <new>           // std::nothrow
#include <unordered_map> // std::unordered_map

#include <RCNET/RCNET.h> // RCNET_log

// ============================================================================
// Post-ACK actions context
// ============================================================================
//
// This context is attached to ENetPacket::userData only for reliable packets
// that need an action after a real ACK from the remote peer.
//
// Why:
// - ENet ACK callback only receives ENetPacket*;
// - we therefore store the state needed by post-ACK actions inside userData.
//
// Lifetime:
// 1) allocated before enet_peer_send,
// 2) read in ACK callback,
// 3) always destroyed in packet free callback (ACK or no ACK).
struct ServerNetworkOutgoingReliableAckActionContext
{
    // Peer targeted by this packet.
    ENetPeer* peer = nullptr;

    // Disconnect peer only after real ACK.
    bool disconnectAfterAck = false;

    // Enable encryption only after real ACK.
    bool enableEncryptionAfterAck = false;
};

// Toggle encryption state for the connection linked to this peer.
//
// Defensive checks:
// - peer must exist,
// - peer->data must contain a valid connectionId,
// - connectionId must still map to the same ENetPeer*.
static void ServerNetworkOutgoing_SetPeerEncryptionEnabled(ENetPeer* peer, bool enabled)
{
    if (peer == nullptr)
    {
        return;
    }

    if (peer->data == nullptr)
    {
        RCNET_log(RCNET_LOG_WARN, "[SERVER] [NETWORK_OUT] [ENCRYPTION] - Cannot toggle encryption: peer->data is null");
        return;
    }

    const uint32_t connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(peer->data));
    NetworkState& networkState = GetNetworkState();

    std::unordered_map<uint32_t, ENetPeer*>::const_iterator it =
        networkState.connectionIdToEnetPeer.find(connectionId);
    if (it == networkState.connectionIdToEnetPeer.end() || it->second != peer)
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "[SERVER] [NETWORK_OUT] [ENCRYPTION] - Cannot toggle encryption: peer mismatch for connectionId=%u",
            connectionId);
        return;
    }

    networkState.connectionIdToEncryptionEnabled[connectionId] = enabled;

    // Mirror encryption state in session so ENet encrypt/decrypt callbacks can
    // read both "enabled" and per-peer keys under the same sessions mutex.
    {
        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);
        std::unordered_map<uint32_t, ClientSession>::iterator sit =
            networkState.sessions.find(connectionId);
        if (sit != networkState.sessions.end())
        {
            sit->second.isPacketEncryptionEnabled = enabled;
        }
    }

    RCNET_log(
        RCNET_LOG_INFO,
        "[SERVER] [NETWORK_OUT] [ENCRYPTION] - connectionId=%u encryptionEnabled=%u",
        connectionId,
        enabled ? 1u : 0u);
}

// ENet packet free callback.
//
// ENet calls this when it finally releases the packet from its internal queues.
// We destroy the context allocated with new here to avoid leaks in every path.
static void ENET_CALLBACK ServerNetworkOutgoing_OnReliablePacketFreed_DestroyAckContext(ENetPacket* packet)
{
    if (packet == nullptr)
    {
        return;
    }

    ServerNetworkOutgoingReliableAckActionContext* context =
        static_cast<ServerNetworkOutgoingReliableAckActionContext*>(packet->userData);
    if (context != nullptr)
    {
        delete context;
        packet->userData = nullptr;
    }
}

// ENet ACK callback for reliable packets.
//
// This callback runs only when ENet confirms the packet has been ACKed by the
// remote side. We run business actions that must happen after real delivery.
static void ENET_CALLBACK ServerNetworkOutgoing_OnReliablePacketAcknowledged_RunActions(ENetPacket* packet)
{
    if (packet == nullptr)
    {
        return;
    }

    ServerNetworkOutgoingReliableAckActionContext* context =
        static_cast<ServerNetworkOutgoingReliableAckActionContext*>(packet->userData);
    if (context == nullptr || context->peer == nullptr)
    {
        return;
    }

    if (context->enableEncryptionAfterAck)
    {
        ServerNetworkOutgoing_SetPeerEncryptionEnabled(context->peer, true);
    }

    if (context->disconnectAfterAck)
    {
        RCNET_log(
            RCNET_LOG_INFO,
            "[SERVER] [NETWORK_OUT] [RELIABLE_ACK] - ACK received, disconnecting peer=%p\n",
            static_cast<void*>(context->peer));
        enet_peer_disconnect_later(context->peer, 0);
    }
}

static bool ServerNetworkOutgoing_SendPacket(
    ENetPeer* peer,
    NetworkChannel channel,
    const std::vector<uint8_t>& bytes,
    enet_uint32 flags,
    bool disconnectAfterAck,
    bool enableEncryptionAfterAck)
{
    // Peer must be valid before creating/sending any packet.
    if (peer == nullptr)
    {
        return false;
    }

    // Create ENet packet from serialized bytes.
    ENetPacket* enetPacket = enet_packet_create(
        bytes.data(),
        bytes.size(),
        flags
    );

    // Abort if packet creation failed.
    if (enetPacket == nullptr)
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to create ENet packet for sending");
        return false;
    }

    // If a post-ACK action is requested, prepare callback context.
    //
    // Important:
    // - post-ACK actions only make sense for reliable packets;
    // - unreliable packets have no delivery ACK in ENet.
    if (disconnectAfterAck || enableEncryptionAfterAck)
    {
        if ((flags & ENET_PACKET_FLAG_RELIABLE) == 0)
        {
            RCNET_log(
                RCNET_LOG_WARN,
                "[SERVER] [NETWORK_OUT] [RELIABLE_ACK] - post-ACK actions ignored on non-reliable packet");
        }
        else
        {
            // Allocate callback context with nothrow to keep explicit failure path.
            ServerNetworkOutgoingReliableAckActionContext* context =
                new (std::nothrow) ServerNetworkOutgoingReliableAckActionContext();
            if (context == nullptr)
            {
                // Fallback: if caller wanted disconnect-after-ACK, force disconnect
                // now because we cannot track ACK without context.
                if (disconnectAfterAck)
                {
                    enet_peer_disconnect_later(peer, 0);
                }

                // ENet did not take ownership of this packet yet.
                enet_packet_destroy(enetPacket);
                RCNET_log(RCNET_LOG_ERROR, "Failed to allocate post-ACK context");
                return false;
            }

            context->peer = peer;
            context->disconnectAfterAck = disconnectAfterAck;
            context->enableEncryptionAfterAck = enableEncryptionAfterAck;

            // Attach context to packet and register both callbacks.
            enetPacket->userData = context;
            enet_packet_set_acknowledge_callback(
                enetPacket,
                ServerNetworkOutgoing_OnReliablePacketAcknowledged_RunActions);
            enet_packet_set_free_callback(
                enetPacket,
                ServerNetworkOutgoing_OnReliablePacketFreed_DestroyAckContext);
        }
    }

    const int sendResult = enet_peer_send(
        peer,
        static_cast<enet_uint8>(channel),
        enetPacket
    );

    // Handle send failure.
    if (sendResult < 0)
    {
        // Fallback: preserve disconnect intent even when send failed.
        if (disconnectAfterAck)
        {
            enet_peer_disconnect_later(peer, 0);
        }

        // Send failed: ENet did not queue this packet, so manual destroy is required.
        enet_packet_destroy(enetPacket);
        RCNET_log(RCNET_LOG_ERROR, "Failed to send ENet packet");
        return false;
    }

    return true;
}

bool ServerNetworkOutgoing_SendMatchInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        disconnectAfterAck,
        false
    );
}

bool ServerNetworkOutgoing_SendWorldStaticStateInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        disconnectAfterAck,
        false
    );
}

bool ServerNetworkOutgoing_SendMatchStartPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::GAME_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        disconnectAfterAck,
        false
    );
}

bool ServerNetworkOutgoing_SendSnapshotFullPacketUnreliable(ENetPeer* peer, const std::vector<uint8_t>& bytes)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::GAME_UNRELIABLE,
        bytes,
        0, // 0 = unreliable packet
        false,
        false
    );
}

bool ServerNetworkOutgoing_SendSecureSessionHelloResponsePacketReliable(
    ENetPeer* peer,
    const std::vector<uint8_t>& bytes,
    bool disconnectAfterAck,
    bool enableEncryptionAfterAck)
{
    // Secure-session response can do two post-ACK actions:
    // - disconnect after ACK when secure session failed,
    // - enable encryption after ACK when secure session succeeded.
    //
    // Enabling encryption only after ACK avoids switching too early, before
    // the client has confirmed reception of this secure-session response.
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::SECURE_SESSION_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        disconnectAfterAck,
        enableEncryptionAfterAck
    );
}

bool ServerNetworkOutgoing_SendAuthResponsePacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck)
{
    return ServerNetworkOutgoing_SendPacket(
        peer,
        NetworkChannel::AUTH_RELIABLE,
        bytes,
        ENET_PACKET_FLAG_RELIABLE,
        disconnectAfterAck,
        false
    );
}
