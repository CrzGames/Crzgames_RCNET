#pragma once

#include <cstdint> // uint64_t, uint32_t

/**
 * @brief Indique si le tick courant doit produire un flux destiné au réseau sortant.
 *
 * Cette fonction sert à cadencer la production des messages que la simulation
 * pousse dans `SimulationToNetworkOUTQueue`.
 *
 * La fréquence réellement utilisée ne dépasse jamais la fréquence du thread
 * réseau sortant. Autrement dit, si `targetRateHz` est supérieur à la fréquence
 * de `network out`, la cadence est automatiquement plafonnée à celle de
 * `network out`.
 *
 * Exemple avec :
 * - simulation = 128 Hz
 * - network out = 32 Hz
 *
 * Alors :
 * - `targetRateHz = 32` déclenche à 32 Hz ;
 * - `targetRateHz = 3` déclenche à environ 3 Hz ;
 * - `targetRateHz = 60` reste plafonné à 32 Hz.
 *
 * @param currentTick Tick courant de simulation.
 * @param targetRateHz Fréquence cible souhaitée pour ce flux sortant.
 *
 * @return `true` si la simulation doit produire ce flux à ce tick,
 *         `false` sinon.
 */
bool ServerSimulationUpdate_IsNetworkOutgoingProductionTick(
    uint64_t currentTick,
    uint32_t targetRateHz);