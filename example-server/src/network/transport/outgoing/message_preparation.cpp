#include "network/transport/outgoing/message_preparation.h"

void ServerNetworkOutgoing_SplitReliableAndCoalesceUnreliableMessages(
    const std::deque<SimulationToNetworkOUTMessage>& outMessages,
    ServerNetworkOutgoingPreparedMessages& preparedMessages)
{
    // Parcourir tous les messages sortants produits par la simulation.
    for (std::deque<SimulationToNetworkOUTMessage>::const_iterator it = outMessages.begin();
         it != outMessages.end();
         ++it)
    {
        // Référence directe vers le message courant.
        const SimulationToNetworkOUTMessage& msg = *it;

        // Si le message est reliable, on le conserve tel quel dans l'ordre.
        if (msg.type == SimulationToNetworkOUTMessageType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE ||
            msg.type == SimulationToNetworkOUTMessageType::SERVER_AUTH_RESPONSE_PACKET_RELIABLE ||
            msg.type == SimulationToNetworkOUTMessageType::SERVER_MATCH_INIT_PACKET_RELIABLE ||
            msg.type == SimulationToNetworkOUTMessageType::SERVER_WORLD_STATIC_STATE_INIT_PACKET_RELIABLE ||
            msg.type == SimulationToNetworkOUTMessageType::SERVER_MATCH_START_PACKET_RELIABLE)
        {
            // Ajouter le message à la liste des reliable à envoyer.
            preparedMessages.reliableMessages.push_back(msg);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE)
        {
            // Pour les unreliable, ne garder que le dernier message
            // par connectionId pendant ce tick réseau sortant.
            preparedMessages.lastSnapshotFullUnreliablePerConnectionId[msg.connectionId] = msg;
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_CLOCK_SYNC_PACKET_UNRELIABLE)
        {
            // Pour les unreliable, ne garder que le dernier message
            // par connectionId pendant ce tick réseau sortant.
            preparedMessages.lastClockSyncUnreliablePerConnectionId[msg.connectionId] = msg;
        }
    }
}