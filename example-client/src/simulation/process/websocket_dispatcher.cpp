#include "simulation/process/websocket_dispatcher.h"

void ClientSimulation_ProcessWebsocketDispatcher(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const std::deque<WebsocketToSimulationMessage>& websocketToSimulationMessages)
{
    // Parcourir tous les messages provenant du thread WebSocket entrants
    // qui ont Ã©tÃ© drainÃ©s pendant ce tick.
    for (std::deque<NatsToSimulationMessage>::const_iterator it = natsToSimulationMessages.begin();
         it != natsToSimulationMessages.end();
         ++it)
    {
        // Réference directe vers le message WebSocket courant.
        const WebsocketToSimulationMessage& msg = *it;   

        // Dispatch du traitement selon le type de message WebSocket reçu.
        switch (msg.type)
        {
            // 

            default:
                RCNET_log(RCNET_LOG_ERROR, "Received unknown WebsocketToSimulationMessageType: %d", static_cast<uint8_t>(msg.type));
                break;
        }
    }
}


