#include <game.h>
#include <game_screen.h>

GameScreen gameScreen;

void rc2d_unload(void)
{

}

void rc2d_load(void)
{
    //rc2d_engine_networkConnectToServer("127.0.0.1", 12345);
}

void rc2d_update(double dt)
{
    gameScreen.update(dt);
}

void rc2d_simulation_update(uint64_t currentTick, uint64_t dtNs, double dt)
{

}

void rc2d_network_incoming_update(ENetHost* host, const ENetEvent* event)
{
    switch (event->type)
    {
        case ENET_EVENT_TYPE_CONNECT:
            RC2D_log(RC2D_LOG_INFO, "[CLIENT] [NETWORK_IN] [CONNECT] - Connected to server.\n");
            break;

        case ENET_EVENT_TYPE_RECEIVE:
            if(event->channelID == 0) // channel SECURE_SESSION_RELIABLE (0)
            {
                RC2D_log(RC2D_LOG_INFO, "[CLIENT] [NETWORK_IN] [SECURE_SESSION] - Packet received from server (size=%u bytes)\n", (unsigned)event->packet->dataLength);
            }
            else if (event->channelID == 1) // channel AUTH_RELIABLE (1)
            {
                RC2D_log(RC2D_LOG_INFO, "[CLIENT] [NETWORK_IN] [AUTH] - Packet received from server (size=%u bytes)\n", (unsigned)event->packet->dataLength);
            }
            else if (event->channelID == 2) // channel GAME RELIABLE (2)
            {
                RC2D_log(RC2D_LOG_INFO, "[CLIENT] [NETWORK_IN] [GAME_RELIABLE] - Packet received from server (size=%u bytes)\n", (unsigned)event->packet->dataLength);
            }
            else if (event->channelID == 3) // channel GAME UNRELIABLE (3)
            {
                RC2D_log(RC2D_LOG_INFO, "[CLIENT] [NETWORK_IN] [GAME_UNRELIABLE] - Packet received from server (size=%u bytes)\n", (unsigned)event->packet->dataLength);
            }
            break;

        // Le serveur déconnecte le client
        case ENET_EVENT_TYPE_DISCONNECT:
            RC2D_log(RC2D_LOG_INFO, "[CLIENT] [NETWORK_IN] [DISCONNECT] - Disconnected from server.\n");
            break;

        // Timeout de connexion au serveur (ex: serveur éteint, crash du serveur, etc.)
        case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            RC2D_log(RC2D_LOG_INFO, "[CLIENT] [NETWORK_IN] [DISCONNECT_TIMEOUT] - Server connection timeout.\n");
            break;

        default:
            break;
    }
}

void rc2d_network_outgoing_update(ENetHost* host)
{

}

void rc2d_http_update(void)
{

}

void rc2d_websocket_update(void)
{

}

void rc2d_draw(void)
{

}

void rc2d_keypressed(const char *key, SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod mod, bool isrepeat, SDL_KeyboardID keyboardID)
{

}

void rc2d_mousepressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID)
{

}