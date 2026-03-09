#include "network/transport/incoming/packets/input.h"

#include "network/packets/client/unreliable.h"
#include "network/serialization/deserialize_packets_client.h"

#include <RCNET/RCNET.h>

void ServerNetworkIncoming_HandlePacket_Input(
    const ENetEvent* event,
    uint32_t connectionId,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Tente de désérialiser le packet envoyé par le client.
    ClientInputPacketUnreliable inputPacket{};
    if (!deserializeClientInputPacketUnreliable(
            event->packet->data,
            event->packet->dataLength,
            inputPacket))
    {
        // Si la désérialisation échoue, le packet est invalide ou mal formé.
        // Log d’avertissement indiquant que le packet d’input est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [INPUT] - Failed to deserialize input packet from connectionId=%u\n",
                  connectionId);
        return;
    }

    // Log d’information indiquant que le packet d’input a été reçu avec succès.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [NETWORK_IN] [INPUT] - Packet received from connectionId=%u (size=%u bytes)\n",
              connectionId,
              (unsigned)event->packet->dataLength);

    // Crée un message destiné à la simulation.
    NetworkINToSimulationMessage message{};
    
    // Renseigne le type du message de simulation.
    message.type = NetworkINToSimulationMessageType::CLIENT_INPUT_PACKET_UNRELIABLE;
    // Renseigne la connexion source.
    message.connectionId = connectionId;
    // Attache le packet input au message.
    message.inputPacket = inputPacket;

    // Envoie le message à la simulation via la queue thread-safe.
    netToSimQueue.push(message);
}