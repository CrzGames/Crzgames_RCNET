#include "simulation/process/incoming/input_message.h"

#include <mutex>         // std::lock_guard
#include <unordered_map> // std::unordered_map

#include <RCNET/RCNET.h>

void ServerSimulation_ProcessNetworkIncomingDispatcher_HandleInputMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& msg)
{
    std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

    // Rechercher la session correspondant à cette connexion.
    std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);

    // Si la session n'existe pas, on ignore l'input reçu.
    if (sit == networkState.sessions.end())
    {
        // Log d'avertissement indiquant qu'on a reçu un input orphelin.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [SIMULATION] [INPUT] - Received input for unknown connectionId=%u (ignoring)\n",
                  msg.connectionId);

        // Abandon du traitement.
        return;
    }

    // Référence directe vers la session trouvée.
    ClientSession& session = sit->second;

    // Mémoriser le dernier input brut reçu.
    session.latestReceivedInputPacket = msg.inputPacket;

    // Mettre à jour l'identifiant du dernier snapshot ack par le client.
    session.clientLastAckedSnapshotId = msg.inputPacket.lastReceivedSnapshotId;

    // Vérifier si cet input a déjà été traité ou est trop ancien.
    if (msg.inputPacket.inputSequenceNumber <= session.serverLastProcessedInputSequenceNumber)
    {
        // Dans ce cas, on l'ignore silencieusement.
        return;
    }

    // Ajouter l'input à la queue des inputs en attente de consommation gameplay.
    session.pendingInputPacketsQueue.push_back(msg.inputPacket);

    // NOTE :
    // On ne touche pas ici à serverLastProcessedInputSequenceNumber.
    // Ce champ doit être mis à jour uniquement quand l'input
    // est réellement appliqué dans la simulation du monde.

    // Log d'information indiquant que l'input a été placé en attente.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [INPUT] - queued connectionId=%u seq=%u (queue size=%zu)\n",
              msg.connectionId,
              msg.inputPacket.inputSequenceNumber,
              session.pendingInputPacketsQueue.size());
}
