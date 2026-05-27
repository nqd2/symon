#include "snapshot.h"

void symon_snapshot_reset(SymonSnapshot *snapshot, uint64_t sequence, uint64_t realtime_ms,
                          uint64_t monotonic_ms)
{
    if (snapshot == NULL) {
        return;
    }

    snapshot->sequence = sequence;
    snapshot->realtime_ms = realtime_ms;
    snapshot->monotonic_ms = monotonic_ms;
    snapshot->collectors_completed = 0;
    snapshot->collectors_failed = 0;
    symon_error_clear(&snapshot->last_collector_error);
    snapshot->cpu = (SymonCpuMetrics){0};
    snapshot->memory = (SymonMemoryMetrics){0};
}
