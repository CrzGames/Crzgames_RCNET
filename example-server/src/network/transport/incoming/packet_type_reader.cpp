#include "network/transport/incoming/packet_type_reader.h"

#include "network/serialization/byte_reader.h"

bool ServerNetworkIncomingUpdate_ReadClientReliablePacketType(
    const ENetEvent* event,
    ClientReliablePacketType& outType)
{
    ByteReader reader(event->packet->data, event->packet->dataLength);

    uint8_t rawType = 0;
    if (!reader.readU8(rawType))
    {
        return false;
    }

    outType = static_cast<ClientReliablePacketType>(rawType);
    return true;
}

bool ServerNetworkIncomingUpdate_ReadClientUnreliablePacketType(
    const ENetEvent* event,
    ClientUnreliablePacketType& outType)
{
    ByteReader reader(event->packet->data, event->packet->dataLength);

    uint8_t rawType = 0;
    if (!reader.readU8(rawType))
    {
        return false;
    }

    outType = static_cast<ClientUnreliablePacketType>(rawType);
    return true;
}