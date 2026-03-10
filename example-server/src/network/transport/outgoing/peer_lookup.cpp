#include "network/transport/outgoing/peer_lookup.h"

#include <unordered_map> // std::unordered_map

#include <RCNET/RCNET.h>

ENetPeer* ServerNetworkOutgoing_FindPeerByConnectionId(
    const NetworkState& networkState,
    uint32_t connectionId)
{
    // Rechercher le peer ENet correspondant à cette connectionId.
    std::unordered_map<uint32_t, ENetPeer*>::const_iterator it =
        networkState.connectionIdToEnetPeer.find(connectionId);

    // Si aucune entrée n'existe pour cette connectionId,
    // on ne peut pas poursuivre.
    if (it == networkState.connectionIdToEnetPeer.end())
    {
        // Log d'erreur indiquant qu'aucun peer n'a été trouvé.
        RCNET_log(RCNET_LOG_ERROR,
                  "[SERVER] [NETWORK_OUT] [PEER_LOOKUP] - No ENet peer found for connectionId=%u\n",
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
                  "[SERVER] [NETWORK_OUT] [PEER_LOOKUP] - ENet peer is null for connectionId=%u\n",
                  connectionId);

        // Retourner nullptr pour signaler l'échec.
        return nullptr;
    }

    // Retourner le peer valide.
    return peer;
}
