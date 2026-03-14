#include "simulation/process/nats/publish_gameserver_simulation_etat_metrics.h"

#include <chrono> // std::chrono::steady_clock, std::chrono::duration

#include <RCNET/RCNET.h>

void ServerSimulation_PublishGameserverSimulationEtatMetricsToNatsIfNeeded(
    uint64_t serverTimeNs,
    const GameserverSimulationEtatMetricsPayload& payloadFromCurrentTick,
    SimulationToNatsQueue& simToNatsQueue)
{
    // Intervalle de publication vise: 1 seconde (en nanosecondes).
    static constexpr uint64_t kPublishIntervalNs = 1'000'000'000ull;

    // Prochain instant (temps monotone serveur) ou l'on autorise une publication.
    // Cette variable est locale statique car:
    // - la fonction est appelee sur le thread simulation uniquement,
    // - on veut conserver l'etat entre les ticks.
    static uint64_t nextPublishAtNs = 0;

    // Initialisation au premier passage:
    // on planifie la premiere publication 1 seconde plus tard.
    if (nextPublishAtNs == 0)
    {
        nextPublishAtNs = serverTimeNs + kPublishIntervalNs;
        return;
    }

    // Tant que l'echeance n'est pas atteinte, on ne publie pas.
    if (serverTimeNs < nextPublishAtNs)
    {
        return;
    }

    // Echeance atteinte:
    // avancer la prochaine echeance d'au moins 1 intervalle.
    // Si le serveur etait en retard, on saute les fenetres passees
    // pour eviter de publier plusieurs messages d'un coup.
    do
    {
        nextPublishAtNs += kPublishIntervalNs;
    }
    while (nextPublishAtNs <= serverTimeNs);

    // Preparer la structure locale qui recevra le snapshot moteur.
    RCNET_SimulationEtatMetrics metrics{};

    // Recuperer le dernier snapshot publie par le moteur.
    // Si aucun snapshot n'est disponible (ex: tout debut), on sort.
    if (!rcnet_engine_getLastSimulationEtatMetrics(&metrics))
    {
        return;
    }

    // Copier le payload construit sur le tick courant.
    GameserverSimulationEtatMetricsPayload payloadToPublish = payloadFromCurrentTick;

    // Injecter le snapshot moteur 1s dans le payload publie.
    payloadToPublish.simulation_etat_metrics = metrics;

    // Construire le message type que le thread NATS saura traiter.
    SimulationToNatsMessage simToNatsMessage{};

    // Renseigner le type logique du message simulation -> NATS.
    simToNatsMessage.type =
        SimulationToNatsMessageType::PUBLISH_MESSAGE_FOR_SUBJECT_GAMESERVER_SIMULATION_ETAT_METRICS;

    // Renseigner le sujet NATS cible.
    simToNatsMessage.subject = "gameserver.simulation.etat.metrics";

    // Copier le payload enrichi dans le message.
    simToNatsMessage.gameserverSimulationEtatMetricsPayload = payloadToPublish;

    // Enqueue vers le thread NATS pour publication effective.
    simToNatsQueue.push(simToNatsMessage);
}
