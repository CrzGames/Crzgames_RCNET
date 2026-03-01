#include <RCNET/RCNET.h>               // logger
#include <rcenet/RCENET_enet.h>        // wrapper ENet de RCENET                

#include <cJSON.h>                     // pour construire un JSON d’input

#include <cstdio>
#include <cstring>
#include <string>
#include <chrono>
#include <thread>

// ------------------------------------------------------------
// Helper: crée un JSON input comme ton serveur l'attend
// { "clientTick": X, "seq": Y, "buttons": B, "ax": ..., "ay": ... }
// ------------------------------------------------------------
static std::string BuildInputJson(uint32_t clientTickId, uint32_t inputSeq, uint32_t buttonsMask, float ax, float ay)
{
    cJSON* root = cJSON_CreateObject();

    // Champs requis
    cJSON_AddNumberToObject(root, "clientTick", (double)clientTickId);
    cJSON_AddNumberToObject(root, "seq", (double)inputSeq);

    // Champs optionnels
    cJSON_AddNumberToObject(root, "buttons", (double)buttonsMask);
    cJSON_AddNumberToObject(root, "ax", (double)ax);
    cJSON_AddNumberToObject(root, "ay", (double)ay);

    // Print JSON compact
    char* printed = cJSON_PrintUnformatted(root);
    std::string json = printed ? printed : "{}";

    // cleanup
    if (printed) cJSON_free(printed);
    cJSON_Delete(root);

    return json;
}

int initializeClient(void)
{
    // ------------------------------------------------------------
    // Serveur ENet : se connecter à localhost:7777
    // ------------------------------------------------------------
    const char* serverHost = "127.0.0.1";
    uint16_t serverPort = 7777;

    // Initialiser ENet (RCENet)
    if (enet_initialize() != 0)
    {
        std::printf("enet_initialize failed\n");
        return 1;
    }

    // ------------------------------------------------------------
    // 1) Résoudre adresse serveur
    // ------------------------------------------------------------
    ENetAddress serverAddress;
    std::memset(&serverAddress, 0, sizeof(serverAddress));

    enet_address_set_host(&serverAddress, ENET_ADDRESS_TYPE_ANY, serverHost);
    serverAddress.port = serverPort;

    // ------------------------------------------------------------
    // 2) Créer le host client
    // channels = 2 (comme ton serveur)
    // ------------------------------------------------------------
    ENetHost* clientHost = enet_host_create(
        serverAddress.type,   // IPv4/IPv6 selon ce qui a été résolu
        NULL,                 // NULL => client host
        1,                    // 1 connexion sortante max
        2,                    // channels
        0,                    // incoming bandwidth
        0                     // outgoing bandwidth
    );
    if (!clientHost)
    {
        std::printf("enet_host_create (client) failed\n");
        enet_deinitialize();
        return 1;
    }

    // ------------------------------------------------------------
    // 3) Connect
    // ------------------------------------------------------------
    ENetPeer* serverPeer = enet_host_connect(clientHost, &serverAddress, 2, 0);
    if (!serverPeer)
    {
        std::printf("enet_host_connect failed (no available peers)\n");
        enet_host_destroy(clientHost);
        enet_deinitialize();
        return 1;
    }

    // ------------------------------------------------------------
    // 4) Attendre l'event CONNECT (max 5 secondes)
    // ------------------------------------------------------------
    ENetEvent event;
    bool isConnected = false;
    if (enet_host_service(clientHost, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
    {
        isConnected = true;
        std::printf("Connected to server!\n");
    }
    else
    {
        // Echec connexion
        enet_peer_reset(serverPeer);
        std::printf("Connection to server failed.\n");
        enet_host_destroy(clientHost);
        enet_deinitialize();
        return 1;
    }

    // ------------------------------------------------------------
    // 5) Boucle client simple :
    // - service events (receive/disconnect)
    // - envoyer input JSON toutes les ~16ms (≈60Hz)
    // ------------------------------------------------------------
    uint32_t clientTickId = 0;
    uint32_t inputSeq = 0;

    auto lastSendTime = std::chrono::steady_clock::now();
    constexpr int sendIntervalMs = 16; // ~60 Hz input envoi (simple)

    while (isConnected)
    {
        // --------------------------------------------------------
        // A) Pump réseau : lire tous les events disponibles
        // --------------------------------------------------------
        while (enet_host_service(clientHost, &event, 0) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_RECEIVE:
                {
                    std::string snapshotText(
                        reinterpret_cast<const char*>(event.packet->data),
                        reinterpret_cast<const char*>(event.packet->data) + event.packet->dataLength
                    );

                    // cJSON veut une string terminée par '\0'
                    snapshotText.push_back('\0');

                    cJSON* root = cJSON_Parse(snapshotText.c_str());
                    if (root)
                    {
                        cJSON* serverTickItem = cJSON_GetObjectItemCaseSensitive(root, "serverTick");
                        cJSON* ackAppliedItem = cJSON_GetObjectItemCaseSensitive(root, "ackApplied");
                        cJSON* ackRecvItem    = cJSON_GetObjectItemCaseSensitive(root, "ackRecv");

                        uint64_t serverTick = (serverTickItem && cJSON_IsNumber(serverTickItem)) ? (uint64_t)serverTickItem->valuedouble : 0;
                        uint32_t ackApplied = (ackAppliedItem && cJSON_IsNumber(ackAppliedItem)) ? (uint32_t)ackAppliedItem->valuedouble : 0;
                        uint32_t ackRecv    = (ackRecvItem && cJSON_IsNumber(ackRecvItem)) ? (uint32_t)ackRecvItem->valuedouble : 0;

                        std::printf("[RECV] serverTick=%llu ackApplied=%u ackRecv=%u\n",
                                    (unsigned long long)serverTick, ackApplied, ackRecv);

                        cJSON_Delete(root);
                    }
                    else
                    {
                        // fallback si JSON invalide
                        std::printf("[RECV] snapshot raw: %s\n", snapshotText.c_str());
                    }

                    enet_packet_destroy(event.packet);
                    break;
                }

                case ENET_EVENT_TYPE_DISCONNECT:
                {
                    std::printf("Disconnected from server.\n");
                    isConnected = false;
                    break;
                }

                // ENet6 timeout event (si dispo dans ton fork)
                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                {
                    std::printf("Disconnected (timeout) from server.\n");
                    isConnected = false;
                    break;
                }

                default:
                    break;
            }
        }

        // --------------------------------------------------------
        // B) Envoi input à intervalle fixe
        // --------------------------------------------------------
        auto now = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSendTime).count();

        if (elapsedMs >= sendIntervalMs)
        {
            lastSendTime = now;

            clientTickId++;
            inputSeq++;

            // Exemple input: buttons=1, axes oscillent
            float ax = 0.25f;
            float ay = -0.10f;
            uint32_t buttons = 1; // ex: "W"

            std::string inputJson = BuildInputJson(clientTickId, inputSeq, buttons, ax, ay);

            ENetPacket* inputPacket = enet_packet_create(
                inputJson.data(),
                inputJson.size(),
                ENET_PACKET_FLAG_UNSEQUENCED // inputs => souvent unsequenced ou unreliable
            );

            enet_peer_send(serverPeer, 0, inputPacket);

            // Flush pour pousser vite (optionnel)
            enet_host_flush(clientHost);

            // (Option) log
            // std::printf("[SEND] %s\n", inputJson.c_str());
        }

        // Petite pause pour éviter 100% CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // ------------------------------------------------------------
    // 6) Cleanup
    // ------------------------------------------------------------
    if (serverPeer)
    {
        enet_peer_disconnect(serverPeer, 0);

        // Essayer de drainer les events 1s
        while (enet_host_service(clientHost, &event, 1000) > 0)
        {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
                enet_packet_destroy(event.packet);
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                break;
        }

        enet_peer_reset(serverPeer);
    }

    enet_host_destroy(clientHost);
    enet_deinitialize();

    std::printf("Client exit.\n");
    return 0;
}