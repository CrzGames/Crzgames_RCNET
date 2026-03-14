#include "services/nats/process/simulation/publish_message_for_subject_gameserver_simulation_etat_metrics.h"

#include <cstring> // strlen

#include "cJSON.h"

void ServerNats_ProcessSimulationDispatcher_HandlePublishMessageForSubjectGameserverSimulationEtatMetrics(
    RCNET_NATSContext* natsContext,
    const SimulationToNatsMessage& message)
{
    // Verifier le contexte NATS: sans contexte valide, on ne peut pas publier.
    if (natsContext == nullptr)
    {
        RCNET_log(
            RCNET_LOG_ERROR,
            "[SERVER] [NATS] [GAMESERVER_SIMULATION_ETAT_METRICS] - natsContext is null");
        return;
    }

    // Verifier le sujet cible: il doit etre renseigne par la simulation.
    if (message.subject.empty())
    {
        RCNET_log(
            RCNET_LOG_ERROR,
            "[SERVER] [NATS] [GAMESERVER_SIMULATION_ETAT_METRICS] - subject is empty");
        return;
    }

    // Creer l'objet JSON racine qui contiendra toutes les metrics.
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr)
    {
        RCNET_log(
            RCNET_LOG_ERROR,
            "[SERVER] [NATS] [GAMESERVER_SIMULATION_ETAT_METRICS] - Failed to create cJSON root object");
        return;
    }

    // Alias local vers le payload enrichi recu depuis la simulation.
    const GameserverSimulationEtatMetricsPayload& payload =
        message.gameserverSimulationEtatMetricsPayload;

    // Alias local vers le snapshot moteur 1s transporte dans le payload.
    const RCNET_SimulationEtatMetrics& metrics = payload.simulation_etat_metrics;

    // Ajouter chaque propriete du payload dans le JSON.
    // Important: les cles JSON reprennent exactement les noms des champs C++
    // pour simplifier le mapping backend.
    bool jsonOk = true;

    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "server_id", (double)payload.server_id) != nullptr);

    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "temps_reel_traitement_network_incoming_dispatcher_sur_ce_tick_ms", payload.temps_reel_traitement_network_incoming_dispatcher_sur_ce_tick_ms) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "temps_reel_traitement_http_dispatcher_sur_ce_tick_ms", payload.temps_reel_traitement_http_dispatcher_sur_ce_tick_ms) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "temps_reel_traitement_nats_dispatcher_sur_ce_tick_ms", payload.temps_reel_traitement_nats_dispatcher_sur_ce_tick_ms) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "temps_reel_traitement_check_match_flow_sur_ce_tick_ms", payload.temps_reel_traitement_check_match_flow_sur_ce_tick_ms) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "temps_reel_traitement_world_simulate_sur_ce_tick_ms", payload.temps_reel_traitement_world_simulate_sur_ce_tick_ms) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "temps_reel_traitement_create_full_snapshots_sur_ce_tick_ms", payload.temps_reel_traitement_create_full_snapshots_sur_ce_tick_ms) != nullptr);

    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "frequence_cible_tick_simulation_hz", (double) metrics.frequence_cible_tick_simulation_hz) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "frequence_reelle_tick_simulation_hz_sur_derniere_seconde", metrics.frequence_reelle_tick_simulation_hz_sur_derniere_seconde) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "nombre_ticks_simulation_executes_sur_derniere_seconde", (double) metrics.nombre_ticks_simulation_executes_sur_derniere_seconde) != nullptr);

    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "temps_moyen_par_tick_sur_derniere_seconde_ms", metrics.temps_moyen_par_tick_sur_derniere_seconde_ms) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "temps_en_ms_sous_lequel_se_situent_95_pourcent_des_ticks_sur_les_deux_dernieres_secondes", metrics.temps_en_ms_sous_lequel_se_situent_95_pourcent_des_ticks_sur_les_deux_dernieres_secondes) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "temps_en_ms_sous_lequel_se_situent_99_pourcent_des_ticks_sur_les_deux_dernieres_secondes", metrics.temps_en_ms_sous_lequel_se_situent_99_pourcent_des_ticks_sur_les_deux_dernieres_secondes) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "temps_maximum_observe_pour_un_tick_sur_derniere_seconde_ms", metrics.temps_maximum_observe_pour_un_tick_sur_derniere_seconde_ms) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "identifiant_tick_du_temps_maximum_observe_sur_derniere_seconde", (double) metrics.identifiant_tick_du_temps_maximum_observe_sur_derniere_seconde) != nullptr);

    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "retard_moyen_de_reveil_du_thread_parmi_les_ticks_en_retard_sur_derniere_seconde_ms", metrics.retard_moyen_de_reveil_du_thread_parmi_les_ticks_en_retard_sur_derniere_seconde_ms) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "retard_maximum_observe_sur_derniere_seconde_ms", metrics.retard_maximum_observe_sur_derniere_seconde_ms) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "nombre_de_reveils_du_thread_apres_l_horaire_prevu_sur_derniere_seconde", (double) metrics.nombre_de_reveils_du_thread_apres_l_horaire_prevu_sur_derniere_seconde) != nullptr);

    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "temps_moyen_total_reel_du_tick_en_comptant_retard_de_reveil_plus_traitement_sur_derniere_seconde_ms", metrics.temps_moyen_total_reel_du_tick_en_comptant_retard_de_reveil_plus_traitement_sur_derniere_seconde_ms) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "marge_moyenne_restante_avant_de_deborder_sur_le_tick_suivant_sur_derniere_seconde_ms", metrics.marge_moyenne_restante_avant_de_deborder_sur_le_tick_suivant_sur_derniere_seconde_ms) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "budget_maximal_par_tick_avant_de_deborder_sur_le_tick_suivant_ms", metrics.budget_maximal_par_tick_avant_de_deborder_sur_le_tick_suivant_ms) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "nombre_ticks_de_rattrapage_executes_sur_derniere_seconde", (double) metrics.nombre_ticks_de_rattrapage_executes_sur_derniere_seconde) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "nombre_ticks_de_rattrapage_executes_depuis_le_lancement_du_serveur", (double) metrics.nombre_ticks_de_rattrapage_executes_depuis_le_lancement_du_serveur) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "nombre_abandons_de_backlog_simulation_sur_derniere_seconde", (double) metrics.nombre_abandons_de_backlog_simulation_sur_derniere_seconde) != nullptr);
    jsonOk = jsonOk && (cJSON_AddNumberToObject(root, "nombre_abandons_de_backlog_simulation_depuis_le_lancement_du_serveur", (double) metrics.nombre_abandons_de_backlog_simulation_depuis_le_lancement_du_serveur) != nullptr);

    // Si au moins une insertion JSON a echoue, on nettoie et on sort.
    if (!jsonOk)
    {
        RCNET_log(
            RCNET_LOG_ERROR,
            "[SERVER] [NATS] [GAMESERVER_SIMULATION_ETAT_METRICS] - Failed to add one or more metric fields to JSON");
        cJSON_Delete(root);
        return;
    }

    // Serialiser l'objet JSON en string compacte.
    char* jsonString = cJSON_PrintUnformatted(root);
    if (jsonString == nullptr)
    {
        RCNET_log(
            RCNET_LOG_ERROR,
            "[SERVER] [NATS] [GAMESERVER_SIMULATION_ETAT_METRICS] - Failed to serialize JSON payload");
        cJSON_Delete(root);
        return;
    }

    // Publier la payload JSON vers le sujet NATS cible.
    // La longueur publiee n'inclut pas le '\0' final.
    rcnet_nats_publish(
        natsContext,
        message.subject.c_str(),
        jsonString,
        static_cast<int>(strlen(jsonString)));

    // Liberer la string allouee par cJSON_PrintUnformatted.
    cJSON_free(jsonString);

    // Liberer l'objet JSON racine.
    cJSON_Delete(root);
}
