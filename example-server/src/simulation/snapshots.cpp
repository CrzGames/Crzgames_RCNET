#include "simulation/snapshots.h"

#include "core/context.h"
#include "network/packets/server/unreliable.h"
#include "network/serialization/serialize_packets_server.h"
#include "simulation/tick_scheduling.h"

#include <unordered_map> // std::unordered_map

#include <RCNET/RCNET.h> // rcnet_engine_getSimulationTickRateHz, rcnet_engine_getNetworkOutgoingTickRateHz, rcnet_engine_computeSnapshotPeriodFromRates, rcnet_engine_getCurrentServerTimeNsMonotonic

void ServerSimulation_CreateFullSnapshotAndEnqueue(
    SimulationToNetworkOUTQueue& simToNetQueue,
    ClientSession& session,
    uint64_t currentTick)
{
    // Construire un packet snapshot full.
    ServerSnapshotFullPacketUnreliable packet{};

    // Renseigner le type du packet.
    packet.header.type = ServerUnreliablePacketType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE;

    // Renseigner le tick serveur courant.
    packet.serverTick = currentTick;

    // Renseigner le temps monotonic serveur courant.
    packet.serverTimeNs = rcnet_engine_getCurrentServerTimeNsMonotonic();

    // Renseigner le dernier input réellement traité côté serveur.
    packet.lastProcessedInputSequenceNumber = session.serverLastProcessedInputSequenceNumber;

    // Construire le message simulation -> réseau.
    SimulationToNetworkOUTMessage outMsg{};

    // Renseigner le type logique du message.
    outMsg.type = SimulationToNetworkOUTMessageType::SERVER_SNAPSHOT_FULL_PACKET_UNRELIABLE;

    // Renseigner la connexion cible.
    outMsg.connectionId = session.connectionId;

    // Sérialiser le packet snapshot.
    outMsg.serializedPacket = serializeServerSnapshotFullPacketUnreliable(packet);

    // Enqueuer le message pour le thread réseau sortant.
    simToNetQueue.push(outMsg);
}

void ServerSimulation_Create_FullSnapshots_ForAllSessionsAndEnqueueForNetworkOutgoing(
    SimulationToNetworkOUTQueue& simToNetQueue,
    NetworkState& networkState,
    uint64_t currentTick)
{
    // Parcourir toutes les sessions connectées.
    for (std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.begin();
         sit != networkState.sessions.end();
         ++sit)
    {
        // Référence directe vers la session courante.
        ClientSession& session = sit->second;

        // Construire puis enqueuer un snapshot full pour cette session.
        ServerSimulation_CreateFullSnapshotAndEnqueue(
            simToNetQueue,
            session,
            currentTick);
    }
}