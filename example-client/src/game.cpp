#include <game.h>
#include <game_screen.h>

#include <RC2D/RC2D.h>

#include <rcenet/RCENET_enet.h>

#include <stdio.h>
#include <stdlib.h>

GameScreen gameScreen;

static ENetHost* g_client = NULL;
static ENetPeer* g_peer   = NULL;

static void net_connect_localhost(uint16_t port)
{
    ENetAddress address;
    ENetEvent event;

    // 1) Résoudre l'adresse localhost
    enet_address_set_host(&address, ENET_ADDRESS_TYPE_ANY, "127.0.0.1");
    address.port = port;

    // 2) Créer le host client après résolution, en utilisant address.type
    g_client = enet_host_create(address.type,
                                NULL,  // client host
                                1,     // 1 connexion sortante
                                4,     // 4 channels (0, 1, 2, 3)
                                0, 0); // bandwidth illimité (assumé)
    if (!g_client)
    {
        fprintf(stderr, "ENet: impossible de créer le host client.\n");
        exit(EXIT_FAILURE);
    }

    // 3) Se connecter
    g_peer = enet_host_connect(g_client, &address, 4 /* channels */, 0);
    if (!g_peer)
    {
        fprintf(stderr, "ENet: aucun peer dispo pour initier la connexion.\n");
        exit(EXIT_FAILURE);
    }

    // 4) Attendre l'event CONNECT (max 5s)
    if (enet_host_service(g_client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        puts("ENet: connexion localhost OK.");
    }
    else
    {
        enet_peer_reset(g_peer);
        g_peer = NULL;
        puts("ENet: connexion localhost FAIL.");
    }
}

static void net_pump_events(void)
{
    if (!g_client) return;

    ENetEvent event;

    // 0 ms => non bloquant, parfait pour un loop jeu
    while (enet_host_service(g_client, &event, 0) > 0)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
                puts("ENet: EVENT CONNECT.");
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                // event.packet->data / dataLength
                printf("ENet: RECV %u bytes on channel %u\n",
                       (unsigned)event.packet->dataLength,
                       (unsigned)event.channelID);

                // IMPORTANT: détruire le packet après usage
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                puts("ENet: DISCONNECT.");
                g_peer = NULL;
                break;

            default:
                break;
        }
    }
}

static void net_disconnect_and_destroy(void)
{
    if (!g_client) return;

    if (g_peer)
    {
        // Demande de disconnect propre
        enet_peer_disconnect(g_peer, 0);

        // On pompe un peu pour recevoir DISCONNECT
        ENetEvent event;
        while (enet_host_service(g_client, &event, 3000) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
                enet_packet_destroy(event.packet);
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                break;
        }

        // Au cas où
        enet_peer_reset(g_peer);
        g_peer = NULL;
    }

    enet_host_destroy(g_client);
    g_client = NULL;
}

/* -------------------- RC2D lifecycle -------------------- */

void rc2d_unload(void)
{
    net_disconnect_and_destroy();
}

void rc2d_load(void)
{
    net_connect_localhost(1234);
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
    if (g_peer && keycode == SDLK_SPACE && !isrepeat)
    {
        const char* msg = "ping";
        ENetPacket* p = enet_packet_create(msg, strlen(msg) + 1, 0 /* UNRELIABLE */);
        enet_peer_send(g_peer, 0, p);
        enet_host_flush(g_client); // optionnel, force l'envoi immédiat
    }
}

void rc2d_mousepressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID)
{

}