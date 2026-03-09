#include "network/transport/outgoing/process/simulation/unreliable_messages.h"

#include "network/client_session.h"
#include "network/packets/server/unreliable.h"
#include "network/serialization/deserialize_packets_server.h"
#include "network/serialization/serialize_packets_server.h"
#include "network/transport/outgoing/enet_send_packets.h"
#include "network/transport/outgoing/peer_lookup.h"

#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

#include <RCNET/RCNET.h>

static void ServerNetworkOutgoing_ProcessSimulationDispatcher_HandleSnapshotFullMessage(
    NetworkState& networkState,
    ENetPeer* peer,
    const SimulationToNetworkOUTMessage& msg)
{
    // Vérifier que le message correspond bien à un snapshot full unreliable.
    if (msg.type != SimulationToNetworkOUTMessageType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE)
    {
        return;
    }

    // Rechercher la session cible pour patcher l'identifiant de snapshot.
    std::unordered_map<uint32_t, ClientSession>::iterator sit =
        networkState.sessions.find(msg.connectionId);

    // Si la session n'existe pas, on ne peut pas poursuivre.
    if (sit == networkState.sessions.end())
    {
        return;
    }

    // Référence directe vers la session cible.
    ClientSession& session = sit->second;

    // Désérialiser le snapshot construit par la simulation.
    ServerSnapshotFullPacketUnreliable snapshotFullPacket{};
    if (!deserializeServerSnapshotFullPacketUnreliable(
            msg.serializedPacket.data(),
            msg.serializedPacket.size(),
            snapshotFullPacket))
    {
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_OUT] [UNRELIABLE] - Failed to deserialize snapshot for connectionId=%u\n",
                  msg.connectionId);
        return;
    }

    // Allouer un nouvel identifiant de snapshot au moment réel de l'envoi.
    const uint32_t snapshotId = session.serverNextSnapshotId++;

    // Écrire cet identifiant dans le packet.
    snapshotFullPacket.snapshotId = snapshotId;

    // Mémoriser le dernier snapshot effectivement envoyé à ce client.
    session.serverLastSentSnapshotId = snapshotId;

    // Résérialiser le packet patché.
    std::vector<uint8_t> patchedPacket =
        serializeServerSnapshotFullPacketUnreliable(snapshotFullPacket);

    // Envoyer le snapshot unreliable patché.
    ServerNetworkOutgoing_SendSnapshotFullPacketUnreliable(peer, patchedPacket);
}

void ServerNetworkOutgoing_ProcessSimulationDispatcher_HandleUnreliableMessages(
    NetworkState& networkState,
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastSnapshotFullUnreliablePerConnectionId,
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastClockSyncUnreliablePerConnectionId)
{
    // Parcourir tous les messages unreliable de type snapshot full coalescés par connectionId.
    for (std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>::iterator it =
            lastSnapshotFullUnreliablePerConnectionId.begin();
         it != lastSnapshotFullUnreliablePerConnectionId.end();
         ++it)
    {
        // Référence directe vers le message courant.
        SimulationToNetworkOUTMessage& msg = it->second;

        // Résoudre le peer ENet correspondant à la connexion cible.
        ENetPeer* peer = ServerNetworkOutgoing_FindPeerByConnectionId(
            networkState,
            msg.connectionId);

        // Si aucun peer valide n'est trouvé, ignorer ce message.
        if (peer == nullptr)
        {
            continue;
        }

        // Envoyer le message unreliable en fonction de son type exact.
        if (msg.type == SimulationToNetworkOUTMessageType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE)
        {
            ServerNetworkOutgoing_ProcessSimulationDispatcher_HandleSnapshotFullMessage(
                networkState,
                peer,
                msg);
        }
    }


    // Parcourir tous les messages unreliable de type clock sync coalescés par connectionId.
    for (std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>::iterator it =
            lastClockSyncUnreliablePerConnectionId.begin();
         it != lastClockSyncUnreliablePerConnectionId.end();
         ++it)
    {
        // Référence directe vers le message courant.
        SimulationToNetworkOUTMessage& msg = it->second;

        // Résoudre le peer ENet correspondant à la connexion cible.
        ENetPeer* peer = ServerNetworkOutgoing_FindPeerByConnectionId(
            networkState,
            msg.connectionId);

        // Si aucun peer valide n'est trouvé, ignorer ce message.
        if (peer == nullptr)
        {
            continue;
        }

        // Envoyer le message unreliable en fonction de son type exact.
        if (msg.type == SimulationToNetworkOUTMessageType::SERVER_CLOCK_SYNC_PACKET_UNRELIABLE)
        {
            //ServerNetworkOutgoing_SendClockSyncPacketUnreliable(peer, msg.serializedPacket);
        }
    }
}