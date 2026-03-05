#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct SnapshotHeader
{
    uint32_t snapshotId;
    uint64_t serverTick;
    uint64_t serverTimeNs;
};
#pragma pack(pop)