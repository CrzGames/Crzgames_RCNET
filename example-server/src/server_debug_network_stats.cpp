#include "server_debug_network_stats.h"

#include <RCNET/RCNET.h>

#include <chrono>

uint64_t g_dbg_simTicks = 0;
uint64_t g_dbg_netOutTicks = 0;
uint64_t g_dbg_snapshotsBuilt = 0;
uint64_t g_dbg_snapshotsSent = 0;

static std::chrono::steady_clock::time_point g_lastLogSim = std::chrono::steady_clock::now();
static std::chrono::steady_clock::time_point g_lastLogOut = std::chrono::steady_clock::now();

// Baselines pour calculer les "par seconde"
static uint64_t g_lastSimTicks = 0;
static uint64_t g_lastNetOutTicks = 0;
static uint64_t g_lastSnapshotsBuilt = 0;
static uint64_t g_lastSnapshotsSent = 0;

void ServerDebugNetworkStats_OnSimulationTick()
{
    ++g_dbg_simTicks;

    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastLogSim).count();
    if (ms >= 1000)
    {
        const uint64_t simHz = g_dbg_simTicks - g_lastSimTicks;
        const uint64_t builtPerSec = g_dbg_snapshotsBuilt - g_lastSnapshotsBuilt;

        RCNET_log(RCNET_LOG_INFO,
                  "[DEBUG_SIMULATION] SIMULATION_UPDATE Hz=%llu | snapshotsBuilt/s=%llu | totalBuilt=%llu\n",
                  (unsigned long long)simHz,
                  (unsigned long long)builtPerSec,
                  (unsigned long long)g_dbg_snapshotsBuilt);

        g_lastSimTicks = g_dbg_simTicks;
        g_lastSnapshotsBuilt = g_dbg_snapshotsBuilt;
        g_lastLogSim = now;
    }
}

void ServerDebugNetworkStats_OnNetworkOutTick()
{
    ++g_dbg_netOutTicks;

    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastLogOut).count();
    if (ms >= 1000)
    {
        const uint64_t outHz = g_dbg_netOutTicks - g_lastNetOutTicks;
        const uint64_t sentPerSec = g_dbg_snapshotsSent - g_lastSnapshotsSent;

        RCNET_log(RCNET_LOG_INFO,
                  "[DEBUG_NETWORK] NETWORK_OUT_UPDATE Hz=%llu | snapshotsSent/s=%llu | totalSent=%llu\n",
                  (unsigned long long)outHz,
                  (unsigned long long)sentPerSec,
                  (unsigned long long)g_dbg_snapshotsSent);

        g_lastNetOutTicks = g_dbg_netOutTicks;
        g_lastSnapshotsSent = g_dbg_snapshotsSent;
        g_lastLogOut = now;
    }
}