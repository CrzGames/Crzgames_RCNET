#pragma once
#include <cstdint>

// ======================================================================================
// SnapshotHeader
//
// Header envoyé dans chaque snapshot serveur -> client.
// Contient les informations nécessaires pour :
// - ordering des snapshots
// - interpolation / timeline serveur
// - reconciliation des inputs côté client
// ======================================================================================

#pragma pack(push, 1)
struct SnapshotHeader
{
    // Identifiant unique du snapshot envoyé au client.
    // Incrémenté côté serveur au moment de l'envoi réel.
    // Utilisé plus tard pour :
    // - ACK snapshots
    // - delta compression
    // - debug réseau.
    uint32_t snapshotId;

    // Tick logique de simulation serveur auquel ce snapshot a été construit.
    // Permet au client de :
    // - connaître la timeline autoritaire du serveur
    // - interpoler correctement les snapshots
    // - debug synchronisation serveur/client.
    uint64_t serverTick;

    // Temps monotone du serveur en nanosecondes depuis le démarrage du moteur.
    // Utilisé pour :
    // - estimer la latence réseau
    // - synchroniser l'horloge client avec le serveur
    // - debug réseau.
    uint64_t serverTimeNs;

    // ACK côté serveur des inputs envoyés par CE client.
    // Indique le dernier inputSequenceNumber que le serveur a réellement
    // appliqué dans la simulation.
    //
    // Utilisé côté client pour :
    // - supprimer les inputs déjà validés par le serveur
    // - rejouer les inputs restants (client-side prediction + reconciliation).
    uint32_t lastProcessedInputSequenceNumber;
};
#pragma pack(pop)