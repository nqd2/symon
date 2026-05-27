#pragma once

#include "error.h"

#include <stddef.h>
#include <stdint.h>

#define SYMON_CPU_MAX_CORES 256

typedef struct {
    bool available;
    bool usage_available;
    double total_usage_percent;
    size_t core_count;
    double core_usage_percent[SYMON_CPU_MAX_CORES];
    double load_average_1m;
    double load_average_5m;
    double load_average_15m;
} SymonCpuMetrics;

typedef struct {
    bool available;
    uint64_t total_bytes;
    uint64_t free_bytes;
    uint64_t available_bytes;
    uint64_t used_bytes;
    uint64_t buffers_bytes;
    uint64_t cached_bytes;
    uint64_t swap_total_bytes;
    uint64_t swap_free_bytes;
    uint64_t swap_used_bytes;
} SymonMemoryMetrics;

typedef struct {
    uint64_t sequence;
    uint64_t realtime_ms;
    uint64_t monotonic_ms;
    size_t collectors_completed;
    size_t collectors_failed;
    SymonError last_collector_error;
    SymonCpuMetrics cpu;
    SymonMemoryMetrics memory;
} SymonSnapshot;

void symon_snapshot_reset(SymonSnapshot *snapshot, uint64_t sequence, uint64_t realtime_ms,
                          uint64_t monotonic_ms);
