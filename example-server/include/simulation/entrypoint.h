#pragma once

#include <cstdint> // uint64_t

void ServerSimulation_DrainNetworkIncomingAndHttpAndNatsMessages_AndRunSimulationForCurrentTick(uint64_t currentTick, uint64_t serverTimeNs, uint64_t dtNs, double dt);