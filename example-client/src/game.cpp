#include <game.h>
#include <game_screen.h>

#include <RC2D/RC2D.h>

#include <rcenet/RCENET_enet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GameScreen gameScreen;

static ENetHost* enetClientHost = NULL;
static ENetPeer* enetServerPeer = NULL;
static bool isConnectedToServer = false;

static void net_disconnect_and_destroy(void)
{
    if (!enetClientHost) return;

    if (enetServerPeer)
    {
        // Demande de disconnect propre
        enet_peer_disconnect(enetServerPeer, 0);

        // On pompe un peu pour recevoir DISCONNECT
        ENetEvent event;
        while (enet_host_service(enetClientHost, &event, 3000) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
                enet_packet_destroy(event.packet);
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                break;
        }

        // Au cas où
        enet_peer_reset(enetServerPeer);
        enetServerPeer = NULL;
    }

    enet_host_destroy(enetClientHost);
    enetClientHost = NULL;
}

static void net_connect_to_server()
{
    // ------------------------------------------------------------
    // Serveur ENet : se connecter à localhost:12345
    // ------------------------------------------------------------
    const char* serverHost = "127.0.0.1";
    uint16_t serverPort = 12345;

    // ------------------------------------------------------------
    // 1) Résoudre adresse serveur
    // ------------------------------------------------------------
    ENetAddress serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    enet_address_set_host(&serverAddress, ENET_ADDRESS_TYPE_ANY, serverHost);
    serverAddress.port = serverPort;

    // ------------------------------------------------------------
    // 2) Créer le host client
    // channels = 4 (comme le serveur)
    // ------------------------------------------------------------
    enetClientHost = enet_host_create(
        serverAddress.type,   // IPv4/IPv6 selon ce qui a été résolu
        NULL,                 // NULL => client host
        1,                    // 1 connexion sortante max
        4,                    // channels
        0,                    // incoming bandwidth
        0                     // outgoing bandwidth
    );
    if (!enetClientHost)
    {
        RC2D_log(RC2D_LOG_INFO, "ENet: échec de la création du host client.\n");
        net_disconnect_and_destroy();
        exit(EXIT_FAILURE);
    }

    // ------------------------------------------------------------
    // 3) Connect au serveur ENet : localhost:12345
    // ------------------------------------------------------------
    enetServerPeer = enet_host_connect(enetClientHost, &serverAddress, 4 /* channels */, 0);
    if (!enetServerPeer)
    {
        RC2D_log(RC2D_LOG_INFO, "ENet: échec de la connexion au serveur.\n");
        net_disconnect_and_destroy();
        exit(EXIT_FAILURE);
    }

    // ------------------------------------------------------------
    // 4) Attendre l'event CONNECT (max 5 secondes)
    // ------------------------------------------------------------
    ENetEvent event;
    if (enet_host_service(enetClientHost, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        RC2D_log(RC2D_LOG_INFO, "ENet: connexion au serveur réussie.\n");
        isConnectedToServer = true;
    }
    else
    {
        net_disconnect_and_destroy();
        RC2D_log(RC2D_LOG_INFO, "ENet: connexion au serveur FAIL.\n");
    }
}

static void net_pump_events(void)
{
    if (!enetClientHost) return;

    // ------------------------------------------------------------
    // Boucle client simple :
    // - service events (receive/disconnect)
    // - envoyer input JSON toutes les ~16ms (≈60Hz)
    // ------------------------------------------------------------
    uint32_t clientTickId = 0;
    uint32_t inputSequenceNumber = 0;

    // 0 ms => non bloquant, parfait pour un loop jeu
    ENetEvent event;
    while (enet_host_service(enetClientHost, &event, 0) > 0)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
                RC2D_log(RC2D_LOG_INFO, "[CLIENT] [NETWORK_IN] [CONNECT] - Connected to server.\n");
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                // event.packet->data / dataLength
                RC2D_log(RC2D_LOG_INFO, "[CLIENT] [NETWORK_IN] [SNAPSHOT] - Packet received from server (size=%u bytes)\n", (unsigned)event.packet->dataLength);

                // IMPORTANT: détruire le packet après usage
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                RC2D_log(RC2D_LOG_INFO, "[CLIENT] [NETWORK_IN] [DISCONNECT] - Disconnected from server.\n");
                enetServerPeer = NULL;
                isConnectedToServer = false;
                break;

            default:
                break;
        }
    }
}

/* -------------------- RC2D lifecycle -------------------- */

void rc2d_unload(void)
{
    net_disconnect_and_destroy();
}

void rc2d_load(void)
{
    net_connect_to_server();
}

void rc2d_update(double dt)
{
    net_pump_events();
    gameScreen.update(dt);
}

void rc2d_draw(void)
{

}

void rc2d_keypressed(const char *key, SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod mod, bool isrepeat, SDL_KeyboardID keyboardID)
{
    // Exemple: envoyer un message quand tu appuies sur espace
    if (enetServerPeer && isConnectedToServer && keycode == SDLK_SPACE && !isrepeat)
    {
        const char* msg = "ping";
        ENetPacket* p = enet_packet_create(msg, strlen(msg) + 1, 0 /* UNRELIABLE */);
        enet_peer_send(enetServerPeer, 0, p);
        enet_host_flush(enetClientHost); // optionnel, force l'envoi immédiat
    }
}

void rc2d_mousepressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID)
{

}