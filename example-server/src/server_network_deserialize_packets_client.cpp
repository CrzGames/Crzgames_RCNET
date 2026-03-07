#include "server_network_deserialize_packets_client.h"

#include "server_network_byte_reader.h"

constexpr size_t kMaxAuthTokenLength = 512;

bool deserializeClientHandshakePacketReliable(const void* data, size_t size, ClientHandshakePacketReliable& outPacket)
{
    ByteReader reader(data, size);

    uint8_t rawType = 0;
    if (!reader.readU8(rawType))
    {
        return false;
    }

    outPacket.header.type = static_cast<ClientReliablePacketType>(rawType);

    if (outPacket.header.type != ClientReliablePacketType::CLIENT_HANDSHAKE_PACKET_RELIABLE)
    {
        return false;
    }

    if (!reader.readU32(outPacket.networkProtocolVersion))
    {
        return false;
    }

    if (!reader.readString(outPacket.authToken, kMaxAuthTokenLength))
    {
        return false;
    }

    if (!reader.empty())
    {
        return false;
    }

    return true;
}

bool deserializeClientReadyForMatchPacketReliable(const void* data, size_t size, ClientReadyForMatchPacketReliable& outPacket)
{
    ByteReader reader(data, size);

    uint8_t rawType = 0;
    if (!reader.readU8(rawType))
    {
        return false;
    }

    outPacket.header.type = static_cast<ClientReliablePacketType>(rawType);

    if (outPacket.header.type != ClientReliablePacketType::CLIENT_READY_FOR_MATCH_PACKET_RELIABLE)
    {
        return false;
    }

    if (!reader.empty())
    {
        return false;
    }

    return true;
}

bool deserializeClientInputPacketUnreliable(const void* data, size_t size, ClientInputPacketUnreliable& outPacket)
{
    ByteReader reader(data, size);

    uint8_t rawType = 0;
    if (!reader.readU8(rawType))
    {
        return false;
    }

    outPacket.header.type = static_cast<ClientUnreliablePacketType>(rawType);

    if (outPacket.header.type != ClientUnreliablePacketType::CLIENT_INPUT_PACKET_UNRELIABLE)
    {
        return false;
    }

    if (!reader.readU32(outPacket.inputSequenceNumber))
    {
        return false;
    }

    if (!reader.readU32(outPacket.lastReceivedSnapshotId))
    {
        return false;
    }

    if (!reader.readU8(outPacket.movementHeldFlags))
    {
        return false;
    }

    if (!reader.readU8(outPacket.movementPressedFlags))
    {
        return false;
    }

    if (!reader.readU8(outPacket.movementReleasedFlags))
    {
        return false;
    }

    if (!reader.readU8(outPacket.actionHeldFlags))
    {
        return false;
    }

    if (!reader.readU8(outPacket.actionPressedFlags))
    {
        return false;
    }

    if (!reader.readU8(outPacket.actionReleasedFlags))
    {
        return false;
    }

    if (!reader.empty())
    {
        return false;
    }

    return true;
}