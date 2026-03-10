#include "simulation/process/network_incoming_dispatcher.h"

#include "simulation/process/incoming/connect_message.h"
#include "simulation/process/incoming/disconnect_message.h"
#include "simulation/process/incoming/input_message.h"
#include "simulation/process/incoming/secure_session_hello_message.h"
#include "simulation/process/incoming/auth_message.h"
#include "simulation/process/incoming/ready_for_match_message.h"

#include <RCNET/RCNET.h>

void ServerSimulation_ProcessNetworkIncomingDispatcher(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    SimulationToHttpQueue& simToHttpQueue,
    const std::deque<NetworkINToSimulationMessage>& messages)
{
    // Parcourir tous les messages réseau entrants
    // qui ont été drainés pendant ce tick.
    for (std::deque<NetworkINToSimulationMessage>::const_iterator it = messages.begin();
         it != messages.end();
         ++it)
    {
        // Référence directe vers le message courant.
        const NetworkINToSimulationMessage& msg = *it;

        // Dispatch du traitement selon le type de message.
        if (msg.type == NetworkINToSimulationMessageType::CLIENT_EVENT_CONNECT)
        {
            // Traiter la connexion d'un nouveau client.
            ServerSimulation_ProcessNetworkIncomingDispatcher_HandleConnectMessage(
                networkState,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_EVENT_DISCONNECT)
        {
            // Traiter la déconnexion d'un client.
            ServerSimulation_ProcessNetworkIncomingDispatcher_HandleDisconnectMessage(
                networkState,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_INPUT_PACKET_UNRELIABLE)
        {
            // Traiter un packet d'input gameplay.
            ServerSimulation_ProcessNetworkIncomingDispatcher_HandleInputMessage(
                networkState,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_SECURE_SESSION_HELLO_PACKET_RELIABLE)
        {
            // Traiter la demande d'établissement de session sécurisée.
            ServerSimulation_ProcessNetworkIncomingDispatcher_HandleSecureSessionHelloMessage(
                networkState,
                simToNetQueue,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_AUTH_PACKET_RELIABLE)
        {
            // Traiter une demande d'authentification.
            ServerSimulation_ProcessNetworkIncomingDispatcher_HandleAuthMessage(
                networkState,
                simToHttpQueue,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_READY_FOR_MATCH_PACKET_RELIABLE)
        {
            // Marquer le client comme prêt pour le match.
            ServerSimulation_ProcessNetworkIncomingDispatcher_HandleReadyForMatchMessage(
                networkState,
                msg);
        }
        else
        {
            RCNET_log(RCNET_LOG_ERROR, "Received unknown NetworkINToSimulationMessageType: %d\n", static_cast<uint8_t>(msg.type));
        }
    }
}
