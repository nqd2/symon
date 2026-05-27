#pragma once

#include "collector.h"
#include "error.h"
#include "snapshot.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
} SymonCpuCounters;

typedef struct {
    const char *stat_path;
    const char *loadavg_path;
    SymonCpuCounters previous_total;
    SymonCpuCounters previous_cores[SYMON_CPU_MAX_CORES];
    size_t previous_core_count;
    bool has_previous;
} SymonLinuxCpuCollector;

void symon_linux_cpu_collector_init(SymonLinuxCpuCollector *collector, const char *stat_path,
                                    const char *loadavg_path);
bool symon_linux_cpu_parse_stat(const char *contents, SymonCpuCounters *total,
                                SymonCpuCounters *cores, size_t *core_count, SymonError *error);
bool symon_linux_cpu_parse_loadavg(const char *contents, double averages[3], SymonError *error);
bool symon_linux_cpu_collect(SymonLinuxCpuCollector *collector, SymonSnapshot *snapshot,
                             SymonError *error);
SymonCollector symon_linux_cpu_as_collector(SymonLinuxCpuCollector *collector);
