#include "services/websocket/process/simulation_dispatcher.h"

void ClientWebSocket_ProcessSimulationDispatcher(const SimulationToWebSocketMessage& simToWebSocketMessage)
{
    switch (simToWebSocketMessage.type)
    {
        // Ajoutez ici les cases pour chaque type de message simulation -> WebSocket que vous souhaitez traiter.

        default:
            RCNET_log(
                RCNET_LOG_ERROR,
                "[SERVER] [WEBSOCKET] - Received unknown SimulationToWebSocketMessageType: %d",
                static_cast<uint8_t>(simToWebSocketMessage.type));
            break;
    }
}