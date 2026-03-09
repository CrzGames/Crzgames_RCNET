#include "network/transport/incoming/connection_validation.h"

#include <cstdint>       // uint32_t, uint64_t
#include <unordered_map> // std::unordered_map

#include <RCNET/RCNET.h>

uint32_t ServerNetworkIncoming_GetValidatedConnectionIdOrZero(
    const ENetEvent* event,
    const NetworkState& networkState)
{
    // Vérifie que le peer associé à l'événement existe.
    // Sans peer, on ne peut pas identifier la connexion source.
    if (event->peer == nullptr)
    {
        // Log d’avertissement indiquant que l’événement est invalide.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - event->peer == nullptr\n");

        // Retourne 0 pour signaler une connexion invalide.
        return 0;
    }

    // Vérifie que peer->data contient bien quelque chose.
    // Dans ton architecture, peer->data est censé stocker le connectionId.
    // Si c'est nul, alors aucune connexion valide n'est associée à ce peer.
    if (event->peer->data == nullptr)
    {
        // Log d’avertissement indiquant qu’aucun connectionId n’est attaché à ce peer.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - peer->data == nullptr\n");

        // Retourne 0 pour signaler une connexion invalide.
        return 0;
    }

    // Convertit le void* stocké dans peer->data en uintptr_t,
    // puis en uint32_t pour récupérer le connectionId.
    const uint32_t connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event->peer->data));

    // Vérifie que le connectionId n’est pas 0.
    // 0 signifie "ID invalide / non initialisé".
    if (connectionId == 0)
    {
        // Log d’avertissement indiquant qu’un ID invalide a été lu.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - connectionId == 0\n");

        // Retourne 0 pour signaler une connexion invalide.
        return 0;
    }

    // Cherche le connectionId dans la table des connexions actives.
    std::unordered_map<uint32_t, ENetPeer*>::const_iterator it = networkState.connectionIdToEnetPeer.find(connectionId);

    // Vérifie que le connectionId existe bien dans la map.
    if (it == networkState.connectionIdToEnetPeer.end())
    {
        // Log d’avertissement indiquant que l’ID n’est pas connu du serveur.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - Unknown connectionId=%u (not found in connectionIdToEnetPeer)\n",
                  connectionId);

        // Retourne 0 pour signaler une connexion invalide.
        return 0;
    }

    // Vérifie que le ENetPeer* trouvé dans la map est exactement celui de l’événement reçu.
    if (it->second != event->peer)
    {
        // Log d’avertissement indiquant une incohérence entre peer->data et la map serveur.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [VALIDATE_CONNECTION] - Peer mismatch for connectionId=%u (map peer=%p, event peer=%p)\n",
                  connectionId,
                  static_cast<void*>(it->second),
                  static_cast<void*>(event->peer));

        // Retourne 0 pour signaler une connexion invalide.
        return 0;
    }

    // Tous les checks sont bons : on retourne le connectionId validé.
    return connectionId;
}