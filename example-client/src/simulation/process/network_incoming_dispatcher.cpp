#include "simulation/process/network_incoming_dispatcher.h"

#include "simulation/process/incoming/connect_message.h"
#include "simulation/process/incoming/disconnect_message.h"
#include "simulation/process/incoming/secure_session_hello_response_message.h"
#include "simulation/process/incoming/auth_response_message.h"
#include "simulation/process/incoming/snapshot_full_message.h"

void ClientSimulation_ProcessNetworkIncomingDispatcher(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    SimulationToHttpQueue& simToHttpQueue,
    const std::deque<NetworkINToSimulationMessage>& networkInToSimulationMessages)
{
    // Parcourir tous les messages réseau entrants
    // qui ont été drainés pendant ce tick.
    for (std::deque<NetworkINToSimulationMessage>::const_iterator it = networkInToSimulationMessages.begin();
         it != networkInToSimulationMessages.end();
         ++it)
    {
        // Référence directe vers le message courant.
        const NetworkINToSimulationMessage& msg = *it;

        // Dispatch du traitement selon le type de message.
        if (msg.type == NetworkINToSimulationMessageType::SERVER_EVENT_CONNECT)
        {
            // Traiter la connexion d'un nouveau client.
            ClientSimulation_ProcessNetworkIncomingDispatcher_HandleConnectMessage(
                networkState,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::SERVER_EVENT_DISCONNECT)
        {
            // Traiter la déconnexion d'un client.
            ClientSimulation_ProcessNetworkIncomingDispatcher_HandleDisconnectMessage(
                networkState,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE)
        {
            // Traiter la réponse du serveur à notre message de "Secure Session Hello".
            ClientSimulation_ProcessNetworkIncomingDispatcher_HandleSecureSessionHelloResponseMessage(
                networkState,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::SERVER_AUTH_RESPONSE_PACKET_RELIABLE)
        {
            // Traiter la réponse du serveur à notre message d'authentification.
            ClientSimulation_ProcessNetworkIncomingDispatcher_HandleAuthResponseMessage(
                networkState,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE)
        {
            // Traiter un snapshot complet du serveur.
            ClientSimulation_ProcessNetworkIncomingDispatcher_HandleSnapshotFullMessage(
                networkState,
                msg);
        }
        else
        {
            RCNET_log(RCNET_LOG_ERROR, "Received unknown NetworkINToSimulationMessageType: %d\n", static_cast<uint8_t>(msg.type));
        }
    }
}
