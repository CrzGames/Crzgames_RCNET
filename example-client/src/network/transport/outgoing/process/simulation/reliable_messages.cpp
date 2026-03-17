#include "network/transport/outgoing/process/simulation/reliable_messages.h"

#include "network/transport/outgoing/enet_send_packets.h"

void ClientNetworkOutgoing_ProcessSimulationDispatcher_HandleReliableMessages(
    const NetworkState& networkState,
    const std::deque<SimulationToNetworkOUTMessage>& simulationToNetworkOUTReliableMessages)
{
    // Parcourir tous les messages reliable à envoyer.
    for (std::deque<SimulationToNetworkOUTMessage>::const_iterator it = simulationToNetworkOUTReliableMessages.begin();
         it != simulationToNetworkOUTReliableMessages.end();
         ++it)
    {
        // Référence directe vers le message courant.
        const SimulationToNetworkOUTMessage& msg = *it;

        // Récupérer le peer ENet correspondant au serveur de destination du message.
        ENetPeer* peer = networkState.serverPeer;

        // Si aucun peer valide n'est trouvé, ignorer ce message.
        if (peer == nullptr)
        {
            continue;
        }

        // Envoyer le message avec la routine correspondant à son type.
        if (msg.type == SimulationToNetworkOUTMessageType::CLIENT_SECURE_SESSION_HELLO_PACKET_RELIABLE)
        {
            // ClientNetworkOutgoing_SendSecureSessionHelloPacketReliable(peer, msg.serializedPacket);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::CLIENT_AUTH_PACKET_RELIABLE)
        {
            // ClientNetworkOutgoing_SendAuthPacketReliable(peer, msg.serializedPacket);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::CLIENT_READY_FOR_MATCH_PACKET_RELIABLE)
        {
            // ClientNetworkOutgoing_SendReadyForMatchPacketReliable(peer, msg.serializedPacket);
        }
        else
        {
            RCNET_log(RCNET_LOG_ERROR, "Received unknown SimulationToNetworkOUTMessageType: %d\n", static_cast<uint8_t>(msg.type));
        }
    }
}
