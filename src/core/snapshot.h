#pragma once

#include "error.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t sequence;
    uint64_t realtime_ms;
    uint64_t monotonic_ms;
    size_t collectors_completed;
    size_t collectors_failed;
    SymonError last_collector_error;
} SymonSnapshot;

void symon_snapshot_reset(SymonSnapshot *snapshot, uint64_t sequence, uint64_t realtime_ms,
                          uint64_t monotonic_ms);
