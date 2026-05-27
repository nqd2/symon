#pragma once

#include "collector.h"
#include "error.h"
#include "snapshot.h"

#include <stdbool.h>

typedef struct {
    const char *meminfo_path;
} SymonLinuxMemoryCollector;

void symon_linux_memory_collector_init(SymonLinuxMemoryCollector *collector, const char *path);
bool symon_linux_memory_parse(const char *contents, SymonMemoryMetrics *metrics, SymonError *error);
bool symon_linux_memory_collect(SymonLinuxMemoryCollector *collector, SymonSnapshot *snapshot,
                                SymonError *error);
SymonCollector symon_linux_memory_as_collector(SymonLinuxMemoryCollector *collector);
