#include "simulation/process/incoming/disconnect_message.h"

#include <RC2D/RC2D.h>

#include <mutex>

void ClientSimulation_ProcessNetworkIncomingDispatcher_HandleDisconnectMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& networkInToSimMessage)
{
    // Ce handler ne doit traiter que les evenements de deconnexion.
    if (networkInToSimMessage.type != NetworkINToSimulationMessageType::SERVER_EVENT_DISCONNECT)
    {
        RC2D_log(
            RC2D_LOG_WARN,
            "[CLIENT] [SIMULATION] [DISCONNECT] Ignored message type=%u in disconnect handler.",
            static_cast<unsigned>(networkInToSimMessage.type));
        return;
    }

    // Proteger la reinitialisation de l'etat partage.
    std::lock_guard<std::mutex> lock(networkState.cryptoMutex);

    // La secure-session n'est plus valide apres deconnexion.
    networkState.secureSessionEstablished = false;

    // Aucune validation d'auth n'est conservee hors connexion.
    networkState.authValidated = false;

    // Le token n'est plus considere valide apres deconnexion.
    networkState.authTokenValidated = false;

    // Le chiffrement transport est desactive hors session active.
    networkState.encryptionEnabled = false;

    // Aucune requete hello en attente apres deconnexion.
    networkState.hasPendingSecureSessionHello = false;

    RC2D_log(RC2D_LOG_INFO, "[CLIENT] [SIMULATION] [DISCONNECT] Session state reset.");
}

