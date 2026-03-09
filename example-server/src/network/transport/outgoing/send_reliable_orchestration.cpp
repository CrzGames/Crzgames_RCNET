#include "network/transport/outgoing/send_reliable_orchestration.h"

#include "network/transport/outgoing/enet_send_packets.h"

#include <unordered_map>

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
                  "[SERVER] [NETWORK_OUT] [RELIABLE] - No ENet peer found for connectionId=%u\n",
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
                  "[SERVER] [NETWORK_OUT] [RELIABLE] - ENet peer is null for connectionId=%u\n",
                  connectionId);

        // Retourner nullptr pour signaler l'échec.
        return nullptr;
    }

    // Retourner le peer valide.
    return peer;
}

void ServerNetworkOutgoing_SendReliableMessages(
    NetworkState& networkState,
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
            // Envoyer un packet MATCH_INIT reliable.
            ServerNetworkOutgoing_SendMatchInitPacketReliable(peer, msg.serializedPacket);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_WORLD_STATIC_STATE_INIT_PACKET_RELIABLE)
        {
            // Envoyer un packet WORLD_STATIC_STATE_INIT reliable.
            ServerNetworkOutgoing_SendWorldStaticStateInitPacketReliable(peer, msg.serializedPacket);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE)
        {
            // Envoyer un packet de réponse secure session reliable.
            ServerNetworkOutgoing_SendSecureSessionHelloResponsePacketReliable(peer, msg.serializedPacket);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_AUTH_RESPONSE_PACKET_RELIABLE)
        {
            // Envoyer un packet de réponse d'authentification reliable.
            ServerNetworkOutgoing_SendAuthResponsePacketReliable(peer, msg.serializedPacket);
        }
        else if (msg.type == SimulationToNetworkOUTMessageType::SERVER_MATCH_START_PACKET_RELIABLE)
        {
            // Envoyer un packet MATCH_START reliable.
            ServerNetworkOutgoing_SendMatchStartPacketReliable(peer, msg.serializedPacket);
        }
    }
}