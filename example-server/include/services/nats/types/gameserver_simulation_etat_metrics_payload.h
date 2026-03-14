#pragma once

#include <cstdint> // uint64_t

#include <RCNET/RCNET_engine.h> // RCNET_SimulationEtatMetrics

// Payload metier publie sur NATS pour remonter l'etat simulation du game server.
// Ce payload regroupe:
// - l'identifiant du serveur,
// - des timings "gros blocs" mesures sur le tick courant,
// - le snapshot global moteur RCNET_SimulationEtatMetrics.
struct GameserverSimulationEtatMetricsPayload
{
    // Identifiant unique du game server (ex: Agones GameServer ID).
    uint64_t server_id = 0;

    // Temps de traitement du bloc "network incoming dispatcher" sur ce tick.
    double temps_reel_traitement_network_incoming_dispatcher_sur_ce_tick_ms = 0.0;

    // Temps de traitement du bloc "http dispatcher" sur ce tick.
    double temps_reel_traitement_http_dispatcher_sur_ce_tick_ms = 0.0;

    // Temps de traitement du bloc "nats dispatcher" sur ce tick.
    double temps_reel_traitement_nats_dispatcher_sur_ce_tick_ms = 0.0;

    // Temps de traitement du bloc "check match flow" sur ce tick.
    double temps_reel_traitement_check_match_flow_sur_ce_tick_ms = 0.0;

    // Temps de traitement du bloc "world simulate" sur ce tick.
    double temps_reel_traitement_world_simulate_sur_ce_tick_ms = 0.0;

    // Temps de traitement du bloc "create full snapshots" sur ce tick.
    // Si le bloc n'est pas execute sur ce tick, la valeur reste 0.
    double temps_reel_traitement_create_full_snapshots_sur_ce_tick_ms = 0.0;

    // Snapshot agrege moteur (1s) expose par RCNET.
    RCNET_SimulationEtatMetrics simulation_etat_metrics{};
};

