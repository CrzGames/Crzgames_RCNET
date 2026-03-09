#include "network/transport/outgoing/entrypoint.h"

#include "core/context.h"
#include "network/transport/outgoing/message_classification.h"
#include "network/transport/outgoing/queue_draining.h"
#include "network/transport/outgoing/process/simulation_dispatcher.h"

#include <cstdint>       // uint32_t
#include <deque>         // std::deque
#include <unordered_map> // std::unordered_map

void ServerNetworkOutgoing_DrainCoalesceAndSendMessages(ENetHost* host)
{
    // Vérifier que l'host ENet est valide avant toute opération.
    if (host == nullptr)
    {
        return;
    }

    // Récupérer une référence vers la queue simulation -> réseau sortant.
    SimulationToNetworkOUTQueue& simToNetQueue = GetSimulationToNetworkOUTQueue();

    // Récupérer l'état réseau global du serveur.
    NetworkState& networkState = GetNetworkState();

    // Préparer la deque locale qui recevra tous les messages drainés.
    std::deque<SimulationToNetworkOUTMessage> outMessages;

    // Drainer la queue simulation -> réseau sortant.
    ServerNetworkOutgoing_DrainSimulationToNetworkOutgoingQueue(
        simToNetQueue,
        outMessages);

    // Préparer le conteneur des messages reliable à envoyer dans l'ordre.
    std::deque<SimulationToNetworkOUTMessage> reliableMessages;

    // Préparer le conteneur des derniers unreliable retenus par connexion.
    std::unordered_map<uint32_t, SimulationToNetworkOUTMessage> lastUnreliablePerConnectionId;

    // Classifier les messages sortants entre reliable et unreliable.
    ServerNetworkOutgoing_ClassifyOutgoingMessages(
        outMessages,
        reliableMessages,
        lastUnreliablePerConnectionId);

    // Dispatcher le traitement des messages issus de la simulation.
    ServerNetworkOutgoing_ProcessSimulationDispatcher(
        networkState,
        reliableMessages,
        lastUnreliablePerConnectionId);

    // Forcer le flush ENet pour limiter la latence d'envoi.
    enet_host_flush(host);
}