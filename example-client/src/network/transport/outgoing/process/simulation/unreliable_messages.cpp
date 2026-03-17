#include "network/transport/outgoing/process/simulation/unreliable_messages.h"

#include "network/packets/client/unreliable.h"
#include "network/serialization/serialize_packets_client.h"
#include "network/transport/outgoing/enet_send_packets.h"
#include "network/transport/outgoing/peer_lookup.h"

#include <mutex>         // std::lock_guard
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

void ClientNetworkOutgoing_ProcessSimulationDispatcher_HandleUnreliableMessages(
    NetworkState& networkState,
    const std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastSnapshotFullUnreliablePerConnectionId,
    const std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastClockSyncUnreliablePerConnectionId)
{
    // Parcourir tous les messages unreliable de type snapshot full coalescés par connectionId.
    for (std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>::const_iterator it =
            lastSnapshotFullUnreliablePerConnectionId.begin();
         it != lastSnapshotFullUnreliablePerConnectionId.end();
         ++it)
    {
        // Référence directe vers le message courant.
        const SimulationToNetworkOUTMessage& msg = it->second;

        // Récupérer le peer ENet correspondant au serveur de destination du message.
        ENetPeer* peer = networkState.serverPeer;

        // Si aucun peer valide n'est trouvé, ignorer ce message.
        if (peer == nullptr)
        {
            continue;
        }

        // Envoyer le message unreliable en fonction de son type exact.
        if (msg.type == SimulationToNetworkOUTMessageType::CLIENT_INPUT_PACKET_UNRELIABLE)
        {
            // ClientNetworkOutgoing_SendInputPacketUnreliable(peer, msg.serializedPacket);
        }
        else
        {
            RCNET_log(RCNET_LOG_ERROR, "Received unknown SimulationToNetworkOUTMessageType in unreliable messages: %d\n", static_cast<uint8_t>(msg.type));
        }
    }


    // Parcourir tous les messages unreliable de type clock sync
    for (std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>::const_iterator it =
            lastClockSyncUnreliablePerConnectionId.begin();
         it != lastClockSyncUnreliablePerConnectionId.end();
         ++it)
    {
        // Référence directe vers le message courant.
        const SimulationToNetworkOUTMessage& msg = it->second;

        // Récupérer le peer ENet correspondant au serveur de destination du message.
        ENetPeer* peer = networkState.serverPeer;

        // Si aucun peer valide n'est trouvé, ignorer ce message.
        if (peer == nullptr)
        {
            continue;
        }

        // Envoyer le message unreliable en fonction de son type exact.
        if (msg.type == SimulationToNetworkOUTMessageType::CLIENT_CLOCK_SYNC_PACKET_UNRELIABLE)
        {
            // ClientNetworkOutgoing_SendClockSyncPacketUnreliable(peer, msg.serializedPacket);
        }
        else
        {
            RCNET_log(RCNET_LOG_ERROR, "Received unknown SimulationToNetworkOUTMessageType in unreliable messages: %d\n", static_cast<uint8_t>(msg.type));
        }
    }
}
