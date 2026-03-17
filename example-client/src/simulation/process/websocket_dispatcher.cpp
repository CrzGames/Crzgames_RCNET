#include "simulation/process/websocket_dispatcher.h"

#include <RC2D/RC2D.h>

#include <mutex>

void ClientSimulation_ProcessWebSocketDispatcher(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const std::deque<WebSocketToSimulationMessage>& websocketToSimulationMessages)
{
    // Lire un snapshot des flags reseau pour enrichir les logs de debug.
    bool secureSessionEstablished = false;
    bool authValidated = false;
    bool authTokenValidated = false;
    bool encryptionEnabled = false;
    {
        std::lock_guard<std::mutex> lock(networkState.cryptoMutex);
        secureSessionEstablished = networkState.secureSessionEstablished;
        authValidated = networkState.authValidated;
        authTokenValidated = networkState.authTokenValidated;
        encryptionEnabled = networkState.encryptionEnabled;
    }

    // Lire la taille de queue OUT a titre indicatif.
    size_t pendingOutMessages = 0;
    {
        std::lock_guard<std::mutex> lock(simToNetQueue.mtx);
        pendingOutMessages = simToNetQueue.q.size();
    }

    // Pour l'instant aucun message websocket->simulation metier n'est implemente.
    // On journalise simplement ce qui arrive pour faciliter l'integration future.
    for (std::deque<WebSocketToSimulationMessage>::const_iterator it = websocketToSimulationMessages.begin();
         it != websocketToSimulationMessages.end();
         ++it)
    {
        const WebSocketToSimulationMessage& msg = *it;
        RC2D_log(
            RC2D_LOG_DEBUG,
            "[CLIENT] [SIMULATION] [WEBSOCKET] Received type=%u (secure=%u authHandled=%u authTokenOk=%u enc=%u pendingOut=%llu).",
            static_cast<unsigned>(msg.type),
            secureSessionEstablished ? 1u : 0u,
            authValidated ? 1u : 0u,
            authTokenValidated ? 1u : 0u,
            encryptionEnabled ? 1u : 0u,
            static_cast<unsigned long long>(pendingOutMessages));
    }
}

