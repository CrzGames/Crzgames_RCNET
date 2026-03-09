#include "network/serialization/deserialize_packets_server.h"

#include "network/serialization/byte_reader.h"

bool deserializeServerSnapshotFullPacketUnreliable(
    const void* data,
    size_t size,
    ServerSnapshotFullPacketUnreliable& outPacket)
{
    ByteReader reader(data, size);

    uint8_t rawType = 0;
    if (!reader.readU8(rawType))
    {
        return false;
    }

    outPacket.header.type = static_cast<ServerUnreliablePacketType>(rawType);

    if (outPacket.header.type != ServerUnreliablePacketType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE)
    {
        return false;
    }

    if (!reader.readU32(outPacket.snapshotId))
    {
        return false;
    }

    if (!reader.readU64(outPacket.serverTick))
    {
        return false;
    }

    if (!reader.readU64(outPacket.serverTimeNs))
    {
        return false;
    }

    if (!reader.readU32(outPacket.lastProcessedInputSequenceNumber))
    {
        return false;
    }

    if (!reader.empty())
    {
        return false;
    }

    return true;
}