#pragma once

#include "collector.h"
#include "error.h"
#include "snapshot.h"

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SYMON_SCHEDULER_MAX_COLLECTORS 32

typedef void (*SymonSnapshotCallback)(const SymonSnapshot *snapshot, void *user_data);

typedef struct {
    uint32_t interval_ms;
    SymonSnapshotCallback on_snapshot;
    void *user_data;
} SymonSchedulerConfig;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    pthread_t worker;
    SymonSchedulerConfig config;
    SymonCollector collectors[SYMON_SCHEDULER_MAX_COLLECTORS];
    size_t collector_count;
    uint64_t next_sequence;
    bool initialized;
    bool running;
    bool stop_requested;
} SymonScheduler;

bool symon_scheduler_init(SymonScheduler *scheduler, const SymonSchedulerConfig *config,
                          SymonError *error);
bool symon_scheduler_add_collector(SymonScheduler *scheduler, SymonCollector collector,
                                   SymonError *error);
bool symon_scheduler_collect_once(SymonScheduler *scheduler, SymonSnapshot *snapshot,
                                  SymonError *error);
bool symon_scheduler_start(SymonScheduler *scheduler, SymonError *error);
void symon_scheduler_stop(SymonScheduler *scheduler);
void symon_scheduler_destroy(SymonScheduler *scheduler);
