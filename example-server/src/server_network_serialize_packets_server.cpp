#include "server_network_serialize_packets_server.h"
#include "server_network_byte_writer.h"

std::vector<uint8_t> serializeServerMatchInitPacketReliable(const ServerMatchInitPacketReliable& packet)
{
    ByteWriter writer;

    writer.writeU8(static_cast<uint8_t>(packet.header.type));
    writer.writeString(packet.mapName);
    writer.writeU32(packet.mapVersion);
    writer.writeU32(packet.mapChecksum);
    writer.writeU64(packet.serverTick);
    writer.writeU32(packet.serverTickRateHz);
    writer.writeU64(packet.serverTimeNs);

    return writer.buffer();
}

std::vector<uint8_t> serializeServerWorldStaticStateInitPacketReliable(const ServerWorldStaticStateInitPacketReliable& packet)
{
    ByteWriter writer;

    writer.writeU8(static_cast<uint8_t>(packet.header.type));

    return writer.buffer();
}

std::vector<uint8_t> serializeServerMatchStartPacketReliable(const ServerMatchStartPacketReliable& packet)
{
    ByteWriter writer;

    writer.writeU8(static_cast<uint8_t>(packet.header.type));
    writer.writeU64(packet.serverTick);
    writer.writeU64(packet.matchStartTick);
    writer.writeU32(packet.countdownTicks);
    writer.writeU64(packet.serverTimeNs);

    return writer.buffer();
}

std::vector<uint8_t> serializeServerSnapshotFullPacketUnreliable(const ServerSnapshotFullPacketUnreliable& packet)
{
    ByteWriter writer;

    writer.writeU8(static_cast<uint8_t>(packet.header.type));
    writer.writeU32(packet.snapshotId);
    writer.writeU64(packet.serverTick);
    writer.writeU64(packet.serverTimeNs);
    writer.writeU32(packet.lastProcessedInputSequenceNumber);

    return writer.buffer();
}

std::vector<uint8_t> serializeServerSecureSessionHelloResponsePacketReliable(const ServerSecureSessionHelloResponsePacketReliable& packet)
{
    ByteWriter writer;

    writer.writeU8(static_cast<uint8_t>(packet.header.type));
    writer.writeU8(static_cast<uint8_t>(packet.status));
    writer.writeBytes(packet.serverPublicKey.data(), packet.serverPublicKey.size());

    return writer.buffer();
}

std::vector<uint8_t> serializeServerAuthResponsePacketReliable(const ServerAuthResponsePacketReliable& packet)
{
    ByteWriter writer;

    writer.writeU8(static_cast<uint8_t>(packet.header.type));
    writer.writeU8(static_cast<uint8_t>(packet.status));

    return writer.buffer();
}