#include "network/transport/outgoing/send_unreliable_orchestration.h"

#include "network/client_session.h"
#include "network/packets/server/unreliable.h"
#include "network/serialization/deserialize_packets_server.h"
#include "network/serialization/serialize_packets_server.h"
#include "network/transport/outgoing/enet_send_packets.h"

#include <unordered_map>
#include <vector>

#include <RCNET/RCNET.h>

static ENetPeer* ServerNetworkOutgoing_FindPeerByConnectionId(
    NetworkState& networkState,
    uint32_t connectionId)
{
    // Rechercher le peer ENet correspondant à cette connectionId.
    std::unordered_map<uint32_t, ENetPeer*>::iterator it =
        networkState.connectionIdToEnetPeer.find(connectionId);

    // Si aucune entrée n'existe pour cette connectionId,
    // on ne peut pas envoyer le message.
    if (it == networkState.connectionIdToEnetPeer.end())
    {
        // Log d'erreur indiquant qu'aucun peer n'a été trouvé.
        RCNET_log(RCNET_LOG_ERROR,
                  "[SERVER] [NETWORK_OUT] [UNRELIABLE] - No ENet peer found for connectionId=%u\n",
                  connectionId);

        // Retourner nullptr pour signaler l'échec.
        return nullptr;
    }

    // Récupérer le peer ENet trouvé.
    ENetPeer* peer = it->second;

    // Vérifier que le peer n'est pas nul.
    if (peer == nullptr)
    {
        // Log d'erreur indiquant un état incohérent.
        RCNET_log(RCNET_LOG_ERROR,
                  "[SERVER] [NETWORK_OUT] [UNRELIABLE] - ENet peer is null for connectionId=%u\n",
                  connectionId);

        // Retourner nullptr pour signaler l'échec.
        return nullptr;
    }

    // Retourner le peer valide.
    return peer;
}

static void ServerNetworkOutgoing_SendSnapshotFull(
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
    std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);

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
        // Log d'avertissement si la désérialisation échoue.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_OUT] [UNRELIABLE] - Failed to deserialize snapshot for connectionId=%u\n",
                  msg.connectionId);

        // Abandon du traitement.
        return;
    }

    // Allouer un nouvel identifiant de snapshot au moment réel de l'envoi.
    const uint32_t snapshotId = session.serverNextSnapshotId++;

    // Écrire cet identifiant dans le packet.
    snapshotFullPacket.snapshotId = snapshotId;

    // Mémoriser le dernier snapshot effectivement envoyé à ce client.
    session.serverLastSentSnapshotId = snapshotId;

    // Résérialiser le packet patché.
    std::vector<uint8_t> patchedPacket = serializeServerSnapshotFullPacketUnreliable(snapshotFullPacket);

    // Envoyer le snapshot unreliable patché.
    ServerNetworkOutgoing_SendSnapshotFullPacketUnreliable(peer, patchedPacket);
}

void ServerNetworkOutgoing_SendUnreliableMessages(
    NetworkState& networkState,
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>& lastUnreliablePerConnectionId)
{
    // Parcourir le dernier unreliable retenu pour chaque connexion.
    for (std::unordered_map<uint32_t, SimulationToNetworkOUTMessage>::iterator it =
             lastUnreliablePerConnectionId.begin();
         it != lastUnreliablePerConnectionId.end();
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
            // Envoyer un snapshot full unreliable patché.
            ServerNetworkOutgoing_SendSnapshotFull(
                networkState,
                peer,
                msg);
        }
    }
}