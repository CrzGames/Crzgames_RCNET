#include "simulation/match_flow.h"

#include "core/context.h"
#include "network/packets/server/reliable.h"
#include "network/serialization/serialize_packets_server.h"
#include "core/config/server.h"

#include <mutex>         // std::lock_guard
#include <unordered_map> // std::unordered_map

#include <RCNET/RCNET.h>

bool ServerSimulation_AreAllSessionsReadyForMatch(const NetworkState& networkState)
{
    std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

    // Vérifier d'abord qu'il y a assez de sessions connectées.
    if (networkState.sessions.size() < ServerConfig::maxClientsConnected)
    {
        // Pas assez de joueurs pour lancer le match.
        return false;
    }

    // Parcourir toutes les sessions actives.
    for (std::unordered_map<uint32_t, ClientSession>::const_iterator it = networkState.sessions.begin();
         it != networkState.sessions.end();
         ++it)
    {
        // Référence directe vers la session courante.
        const ClientSession& session = it->second;

        // Si une session n'est pas prête, on bloque le démarrage.
        if (!session.isReadyForMatch)
        {
            return false;
        }
    }

    // Toutes les sessions requises sont prêtes.
    return true;
}

void ServerSimulation_CheckMatchFlow(
    GameState& gameState,
    const NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    uint64_t currentTick)
{
    // Tant qu'il n'y a pas assez de joueurs connectés,
    // on ne lance aucune étape du flow.
    {
        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);
        if (networkState.sessions.size() < ServerConfig::maxClientsConnected)
        {
            return;
        }
    }

    // Envoyer MATCH_INIT une seule fois à tous les clients.
    if (!gameState.matchInitSent)
    {
        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

        // Parcourir toutes les sessions connectées.
        for (std::unordered_map<uint32_t, ClientSession>::const_iterator it = networkState.sessions.begin();
             it != networkState.sessions.end();
             ++it)
        {
            // Référence vers la session courante.
            const ClientSession& session = it->second;

            // Construire le packet MATCH_INIT.
            ServerMatchInitPacketReliable matchInitPacket{};

            // Renseigner son type.
            matchInitPacket.header.type = ServerReliablePacketType::SERVER_MATCH_INIT_PACKET_RELIABLE;

            // TODO : remplacer par les vraies infos de map.
            matchInitPacket.mapName = "ExampleNameMap";
            matchInitPacket.mapVersion = 1;
            matchInitPacket.mapChecksum = 0;

            // Renseigner les informations de temps et de tick serveur.
            matchInitPacket.serverTick = currentTick;
            matchInitPacket.serverTickRateHz = rcnet_engine_getSimulationTickRateHz();
            matchInitPacket.serverTimeNs = rcnet_engine_getCurrentServerTimeNsMonotonic();

            // Construire le message simulation -> réseau.
            SimulationToNetworkOUTMessage msg{};

            // Renseigner le type logique du message.
            msg.type = SimulationToNetworkOUTMessageType::SERVER_MATCH_INIT_PACKET_RELIABLE;

            // Renseigner la connexion cible.
            msg.connectionId = session.connectionId;

            // Sérialiser le packet.
            msg.serializedPacket = serializeServerMatchInitPacketReliable(matchInitPacket);

            // Enqueuer le message pour le thread réseau sortant.
            simToNetQueue.push(msg);
        }

        // Marquer l'étape comme effectuée.
        gameState.matchInitSent = true;
    }

    // Envoyer WORLD_STATIC_STATE_INIT une seule fois à tous les clients.
    if (!gameState.worldStaticStateInitSent)
    {
        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

        // Parcourir toutes les sessions connectées.
        for (std::unordered_map<uint32_t, ClientSession>::const_iterator it = networkState.sessions.begin();
             it != networkState.sessions.end();
             ++it)
        {
            // Référence vers la session courante.
            const ClientSession& session = it->second;

            // Construire le packet d'init monde statique.
            ServerWorldStaticStateInitPacketReliable worldStaticStateInitPacket{};

            // Renseigner son type.
            worldStaticStateInitPacket.header.type = ServerReliablePacketType::SERVER_WORLD_STATIC_STATE_INIT_PACKET_RELIABLE;

            // Construire le message simulation -> réseau.
            SimulationToNetworkOUTMessage msg{};

            // Renseigner le type logique du message.
            msg.type = SimulationToNetworkOUTMessageType::SERVER_WORLD_STATIC_STATE_INIT_PACKET_RELIABLE;

            // Renseigner la connexion cible.
            msg.connectionId = session.connectionId;

            // Sérialiser le packet.
            msg.serializedPacket = serializeServerWorldStaticStateInitPacketReliable(worldStaticStateInitPacket);

            // Enqueuer le message pour le thread réseau sortant.
            simToNetQueue.push(msg);
        }

        // Marquer l'étape comme effectuée.
        gameState.worldStaticStateInitSent = true;
    }

    // Si le countdown n'a pas encore été envoyé
    // et que tous les joueurs sont prêts, lancer l'étape MATCH_START.
    if (!gameState.matchStartSent &&
        ServerSimulation_AreAllSessionsReadyForMatch(networkState))
    {
        // Calculer la durée du countdown en ticks.
        const uint32_t countdownTicks = rcnet_engine_durationMsToTicks(3000);

        // Calculer le tick de démarrage réel du match.
        gameState.matchStartTick = currentTick + countdownTicks;

        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

        // Parcourir toutes les sessions connectées.
        for (std::unordered_map<uint32_t, ClientSession>::const_iterator it = networkState.sessions.begin();
             it != networkState.sessions.end();
             ++it)
        {
            // Référence vers la session courante.
            const ClientSession& session = it->second;

            // Construire le packet MATCH_START.
            ServerMatchStartPacketReliable matchStartPacket{};

            // Renseigner son type.
            matchStartPacket.header.type = ServerReliablePacketType::SERVER_MATCH_START_PACKET_RELIABLE;

            // Renseigner le tick serveur courant.
            matchStartPacket.serverTick = currentTick;

            // Renseigner le tick réel de démarrage.
            matchStartPacket.matchStartTick = gameState.matchStartTick;

            // Renseigner le countdown.
            matchStartPacket.countdownTicks = countdownTicks;

            // Renseigner le temps monotonic serveur.
            matchStartPacket.serverTimeNs = rcnet_engine_getCurrentServerTimeNsMonotonic();

            // Construire le message simulation -> réseau.
            SimulationToNetworkOUTMessage msg{};

            // Renseigner le type logique.
            msg.type = SimulationToNetworkOUTMessageType::SERVER_MATCH_START_PACKET_RELIABLE;

            // Renseigner la connexion cible.
            msg.connectionId = session.connectionId;

            // Sérialiser le packet.
            msg.serializedPacket = serializeServerMatchStartPacketReliable(matchStartPacket);

            // Enqueuer le message pour le thread réseau sortant.
            simToNetQueue.push(msg);
        }

        // Marquer que le countdown a été envoyé.
        gameState.matchStartSent = true;
    }

    // Si le match n'a pas encore réellement démarré,
    // mais que le tick de départ est atteint, alors démarrer le match.
    if (!gameState.matchStarted &&
        gameState.matchStartSent &&
        currentTick >= gameState.matchStartTick)
    {
        // Marquer le match comme démarré.
        gameState.matchStarted = true;

        // Log d'information de démarrage.
        RCNET_log(RCNET_LOG_INFO,
                  "[SERVER] [MATCH] Match started at tick=%llu\n",
                  (unsigned long long)currentTick);
    }
}
