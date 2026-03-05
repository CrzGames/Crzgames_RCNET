#pragma once

#include <cstdint>

// Counters (incrementés depuis différents .cpp)
extern uint64_t g_dbg_simTicks;
extern uint64_t g_dbg_netOutTicks;
extern uint64_t g_dbg_snapshotsBuilt;
extern uint64_t g_dbg_snapshotsSent;

// Helper: log 1 fois/sec depuis simulation + netout
void ServerDebugNetworkStats_OnSimulationTick();
void ServerDebugNetworkStats_OnNetworkOutTick();