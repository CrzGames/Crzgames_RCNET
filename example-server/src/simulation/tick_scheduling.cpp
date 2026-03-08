#include "simulation/tick_scheduling.h"

#include "core/context.h"

bool ServerSimulationUpdate_IsNetworkOutgoingProductionTick(
    uint64_t currentTick,
    uint32_t targetRateHz)
{
    // Récupérer la fréquence de simulation.
    const uint32_t simulationRateHz = rcnet_engine_getSimulationTickRateHz();

    // Récupérer la fréquence du thread réseau sortant.
    const uint32_t networkOutgoingRateHz = rcnet_engine_getNetworkOutgoingTickRateHz();

    // Si la fréquence cible est invalide, ne jamais déclencher.
    if (targetRateHz == 0)
    {
        return false;
    }

    // Si une des fréquences système est invalide, ne jamais déclencher.
    if (simulationRateHz == 0 || networkOutgoingRateHz == 0)
    {
        return false;
    }

    // Ne jamais produire plus vite que le thread réseau sortant.
    const uint32_t effectiveRateHz =
        (targetRateHz < networkOutgoingRateHz)
            ? targetRateHz
            : networkOutgoingRateHz;

    // Si la fréquence effective atteint ou dépasse la fréquence de simulation,
    // alors on peut produire à chaque tick.
    if (effectiveRateHz >= simulationRateHz)
    {
        return true;
    }

    // Le tick 0 peut être considéré comme un tick de production.
    if (currentTick == 0)
    {
        return true;
    }

    // Utiliser une comparaison par "bucket temporel franchi"
    // pour mieux répartir les fréquences non divisibles exactement.
    const uint64_t previousBucket = ((currentTick - 1) * effectiveRateHz) / simulationRateHz;
    const uint64_t currentBucket  = (currentTick * effectiveRateHz) / simulationRateHz;

    // Déclencher uniquement lorsqu'on change de bucket.
    return currentBucket != previousBucket;
}