#include "network/transport/incoming/events/connect.h"

#include <cstdint> // uintptr_t

#include <RCNET/RCNET.h> // RCNET_log

void ServerNetworkIncoming_Event_HandleConnect(
    const ENetEvent* event,
    NetworkState& networkState,
    NetworkINToSimulationQueue& netToSimQueue)
{
    // Vérifie que le peer associé à l'événement de connexion n'est pas nul.
    if (event->peer == nullptr)
    {
        // Log un avertissement si l'événement de connexion est invalide (peer nul) et retourne sans faire d'autres traitements.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [NETWORK_IN] [CONNECT] - Invalid connect event: event->peer == nullptr\n");
        return;
    }

    // Génère un nouvel identifiant de connexion unique en utilisant le compteur nextConnectionId du NetworkState.
    const uint32_t connectionId = networkState.nextConnectionId++;

    // Associe l'identifiant de connexion généré aux données du peer en utilisant un cast pour stocker l'ID dans un pointeur void*.
    event->peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(connectionId));

    // Stocke une référence au peer dans la map connectionIdToEnetPeer du NetworkState, 
    // en utilisant l'identifiant de connexion comme clé.
    networkState.connectionIdToEnetPeer[connectionId] = event->peer;

    // Par défaut, le chiffrement réseau est désactivé pour cette connexion
    // jusqu'à confirmation de l'ACK du secure session hello response.
    networkState.connectionIdToEncryptionEnabled[connectionId] = false;

    // Crée un message destiné au thread simulation.
    NetworkINToSimulationMessage message{};

    // Indique que ce message correspond à un événement de connexion client.
    message.type = NetworkINToSimulationMessageType::CLIENT_EVENT_CONNECT;
    // Renseigne l’ID de connexion qui vient d’être créée.
    message.connectionId = connectionId;

    // Envoie le message au thread simulation via la queue thread-safe.
    netToSimQueue.push(message);

    // Log l'événement de connexion avec l'identifiant de connexion concerné.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [NETWORK_IN] [CONNECT] - connectionId: %u - Transport connected\n",
              connectionId);
}
