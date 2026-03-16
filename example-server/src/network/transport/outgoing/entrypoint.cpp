#include "network/transport/outgoing/entrypoint.h"

#include "core/context.h"
#include "network/transport/compression/enet_host_lz4_compressor.h"
#include "network/transport/encryption/enet_host_xchacha20poly1305_encryptor.h"
#include "network/transport/outgoing/message_preparation.h"
#include "network/transport/outgoing/queue_draining.h"
#include "network/transport/outgoing/process/simulation_dispatcher.h"

#include <deque> // std::deque

void ServerNetworkOutgoing_DrainSimulationMessages_And_RunOutgoingNetworkLogic(ENetHost* host)
{
    // Vérifier que l'host ENet est valide avant toute opération.
    if (host == nullptr)
    {
        return;
    }

    // Installer le compresseur LZ4 au niveau host.
    // Cette fonction est idempotente : si déjà installé, elle ne fait rien.
    // La compression et la décompression seront appliquées par ENet sur le trafic UDP du host.
    //ServerNetworkCompression_EnsureHostLz4CompressorInstalled(host);

    // Installer l'encryptor XChaCha20-Poly1305 au niveau host.
    // Cette fonction est idempotente : si déjà installé, elle ne fait rien.
    // Les callbacks décideront ensuite peer par peer si les paquets applicatifs doivent être chiffrés.
    //ServerNetworkEncryption_EnsureHostEncryptorInstalled(host);

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

    // Préparer la structure qui recevra les messages sortants
    // déjà classés et coalescés par famille.
    ServerNetworkOutgoingPreparedMessages preparedMessages{};

    // Classer les messages sortants en :
    // - reliable conservés dans l'ordre
    // - snapshots unreliable coalescés par connectionId
    // - clock sync unreliable coalescés par connectionId
    ServerNetworkOutgoing_SplitReliableAndCoalesceUnreliableMessages(
        outMessages,
        preparedMessages);

    // Dispatcher le traitement des messages issus de la simulation.
    ServerNetworkOutgoing_ProcessSimulationDispatcher(
        networkState,
        preparedMessages);

    // Forcer le flush ENet pour limiter la latence d'envoi.
    enet_host_flush(host);
}
