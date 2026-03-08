#include "simulation/process/network_incoming_messages.h"

#include "simulation/process/incoming/connect_message.h"
#include "simulation/process/incoming/disconnect_message.h"
#include "simulation/process/incoming/input_message.h"
#include "simulation/process/incoming/secure_session_hello_message.h"
#include "simulation/process/incoming/auth_message.h"
#include "simulation/process/incoming/ready_for_match_message.h"

void ServerSimulationUpdate_ProcessNetworkIncomingMessages(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    SimulationToHttpQueue& simToHttpQueue,
    std::deque<NetworkINToSimulationMessage>& messages)
{
    // Parcourir tous les messages réseau entrants
    // qui ont été drainés pendant ce tick.
    for (std::deque<NetworkINToSimulationMessage>::iterator it = messages.begin();
         it != messages.end();
         ++it)
    {
        // Référence directe vers le message courant.
        NetworkINToSimulationMessage& msg = *it;

        // Dispatch du traitement selon le type de message.
        if (msg.type == NetworkINToSimulationMessageType::CLIENT_EVENT_CONNECT)
        {
            // Traiter la connexion d'un nouveau client.
            ServerSimulationUpdate_ProcessNetworkIncomingMessages_HandleConnect(
                networkState,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_EVENT_DISCONNECT)
        {
            // Traiter la déconnexion d'un client.
            ServerSimulationUpdate_ProcessNetworkIncomingMessages_HandleDisconnect(
                networkState,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_INPUT_PACKET_UNRELIABLE)
        {
            // Traiter un packet d'input gameplay.
            ServerSimulationUpdate_ProcessNetworkIncomingMessages_HandleInput(
                networkState,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_SECURE_SESSION_HELLO_PACKET_RELIABLE)
        {
            // Traiter la demande d'établissement de session sécurisée.
            ServerSimulationUpdate_ProcessNetworkIncomingMessages_HandleSecureSessionHello(
                networkState,
                simToNetQueue,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_AUTH_PACKET_RELIABLE)
        {
            // Traiter une demande d'authentification.
            ServerSimulationUpdate_ProcessNetworkIncomingMessages_HandleAuth(
                networkState,
                simToHttpQueue,
                msg);
        }
        else if (msg.type == NetworkINToSimulationMessageType::CLIENT_READY_FOR_MATCH_PACKET_RELIABLE)
        {
            // Marquer le client comme prêt pour le match.
            ServerSimulationUpdate_ProcessNetworkIncomingMessages_HandleReadyForMatch(
                networkState,
                msg);
        }
    }
}