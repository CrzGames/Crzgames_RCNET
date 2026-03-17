#include "network/transport/outgoing/process/simulation/reliable_messages.h"

#include "network/transport/outgoing/enet_send_packets.h"
#include "network/transport/outgoing/peer_lookup.h"

#include <RCNET/RCNET.h>

void ServerNetworkOutgoing_ProcessSimulationDispatcher_HandleReliableMessages(
    const NetworkState& networkState,
    const std::deque<SimulationToNetworkOUTMessage>& reliableMessages)
{
    // Parcourir tous les messages reliable à envoyer.
    for (std::deque<SimulationToNetworkOUTMessage>::const_iterator it = reliableMessages.begin();
         it != reliableMessages.end();
         ++it)
    {
        // Référence directe vers le message courant.
        const SimulationToNetworkOUTMessage& msg = *it;

        // Résoudre le peer ENet correspondant à la connexion cible.
        ENetPeer* peer = ServerNetworkOutgoing_FindPeerByConnectionId(
            networkState,
            msg.connectionId);

        // Si aucun peer valide n'est trouvé, ignorer ce message.
        if (peer == nullptr)
        {
            continue;
        }

        // Envoyer le message avec la routine correspondant à son type.
        if (msg.type == SimulationToNetworkOUTMessageType::SERVER_MATCH_INIT_PACKET_RELIABLE)
        {
            ServerNetworkOutgoing_SendMatchInitPacketReliable(peer, msg.serializedPacket);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_WORLD_STATIC_STATE_INIT_PACKET_RELIABLE)
        {
            ServerNetworkOutgoing_SendWorldStaticStateInitPacketReliable(peer, msg.serializedPacket);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE)
        {
            ServerNetworkOutgoing_SendSecureSessionHelloResponsePacketReliable(
                peer,
                msg.serializedPacket,
                msg.disconnectAfterAck,
                msg.enableEncryptionAfterAck);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_AUTH_RESPONSE_PACKET_RELIABLE)
        {
            ServerNetworkOutgoing_SendAuthResponsePacketReliable(peer, msg.serializedPacket, msg.disconnectAfterAck);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_MATCH_START_PACKET_RELIABLE)
        {
            ServerNetworkOutgoing_SendMatchStartPacketReliable(peer, msg.serializedPacket);
        }
        else
        {
            RCNET_log(RCNET_LOG_ERROR, "Received unknown SimulationToNetworkOUTMessageType: %d\n", static_cast<uint8_t>(msg.type));
        }
    }
}
