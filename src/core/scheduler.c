#include "scheduler.h"

#include "linux_clock.h"

#include <errno.h>
#include <time.h>

static void add_milliseconds(struct timespec *deadline, uint32_t milliseconds)
{
    const long nanoseconds_per_second = 1000000000L;
    long added_nanoseconds = (long)(milliseconds % 1000U) * 1000000L;

    deadline->tv_sec += (time_t)(milliseconds / 1000U);
    deadline->tv_nsec += added_nanoseconds;
    if (deadline->tv_nsec >= nanoseconds_per_second) {
        deadline->tv_sec++;
        deadline->tv_nsec -= nanoseconds_per_second;
    }
}

static bool collect_snapshot(SymonScheduler *scheduler, SymonSnapshot *snapshot, SymonError *error)
{
    SymonTimestamp timestamp;

    if (!symon_linux_clock_now(&timestamp, error)) {
        return false;
    }

    symon_snapshot_reset(snapshot, scheduler->next_sequence++, timestamp.realtime_ms,
                         timestamp.monotonic_ms);
    for (size_t index = 0; index < scheduler->collector_count; index++) {
        SymonError collector_error;
        SymonCollector *collector = &scheduler->collectors[index];

        symon_error_clear(&collector_error);
        if (collector->collect(collector->context, snapshot, &collector_error)) {
            snapshot->collectors_completed++;
            continue;
        }

        snapshot->collectors_failed++;
        if (!symon_error_is_set(&collector_error)) {
            symon_error_set(&collector_error, SYMON_ERROR_COLLECTOR, 0,
                            "collector callback failed");
        }
        snapshot->last_collector_error = collector_error;
    }

    return true;
}

static void *scheduler_worker(void *context)
{
    SymonScheduler *scheduler = context;

    for (;;) {
        SymonSnapshot snapshot;
        SymonError error;
        struct timespec deadline;
        int wait_result = 0;

        pthread_mutex_lock(&scheduler->mutex);
        if (scheduler->stop_requested) {
            pthread_mutex_unlock(&scheduler->mutex);
            break;
        }
        pthread_mutex_unlock(&scheduler->mutex);

        symon_error_clear(&error);
        if (collect_snapshot(scheduler, &snapshot, &error)) {
            scheduler->config.on_snapshot(&snapshot, scheduler->config.user_data);
        }

        if (clock_gettime(CLOCK_REALTIME, &deadline) != 0) {
            pthread_mutex_lock(&scheduler->mutex);
            scheduler->stop_requested = true;
            pthread_mutex_unlock(&scheduler->mutex);
            break;
        }
        add_milliseconds(&deadline, scheduler->config.interval_ms);

        pthread_mutex_lock(&scheduler->mutex);
        while (!scheduler->stop_requested && wait_result != ETIMEDOUT) {
            wait_result =
                pthread_cond_timedwait(&scheduler->condition, &scheduler->mutex, &deadline);
            if (wait_result != 0 && wait_result != ETIMEDOUT) {
                scheduler->stop_requested = true;
            }
        }
        if (scheduler->stop_requested) {
            pthread_mutex_unlock(&scheduler->mutex);
            break;
        }
        pthread_mutex_unlock(&scheduler->mutex);
    }

    return NULL;
}

bool symon_scheduler_init(SymonScheduler *scheduler, const SymonSchedulerConfig *config,
                          SymonError *error)
{
    int result;

    if (scheduler == NULL || config == NULL || config->interval_ms == 0) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "invalid scheduler configuration");
        return false;
    }

    scheduler->config = *config;
    scheduler->collector_count = 0;
    scheduler->next_sequence = 1;
    scheduler->initialized = false;
    scheduler->running = false;
    scheduler->stop_requested = false;

    result = pthread_mutex_init(&scheduler->mutex, NULL);
    if (result != 0) {
        symon_error_set(error, SYMON_ERROR_THREAD, result, "cannot initialize scheduler mutex");
        return false;
    }

    result = pthread_cond_init(&scheduler->condition, NULL);
    if (result != 0) {
        pthread_mutex_destroy(&scheduler->mutex);
        symon_error_set(error, SYMON_ERROR_THREAD, result, "cannot initialize scheduler condition");
        return false;
    }

    scheduler->initialized = true;
    return true;
}

bool symon_scheduler_add_collector(SymonScheduler *scheduler, SymonCollector collector,
                                   SymonError *error)
{
    if (scheduler == NULL || !scheduler->initialized || collector.name == NULL ||
        collector.collect == NULL) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "invalid collector");
        return false;
    }
    if (scheduler->running) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0,
                        "cannot add collector while scheduler runs");
        return false;
    }
    if (scheduler->collector_count >= SYMON_SCHEDULER_MAX_COLLECTORS) {
        symon_error_set(error, SYMON_ERROR_CAPACITY, 0, "collector capacity reached");
        return false;
    }

    scheduler->collectors[scheduler->collector_count++] = collector;
    return true;
}

bool symon_scheduler_collect_once(SymonScheduler *scheduler, SymonSnapshot *snapshot,
                                  SymonError *error)
{
    if (scheduler == NULL || !scheduler->initialized || snapshot == NULL || scheduler->running) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "invalid synchronous collection");
        return false;
    }

    return collect_snapshot(scheduler, snapshot, error);
}

bool symon_scheduler_start(SymonScheduler *scheduler, SymonError *error)
{
    int result;

    if (scheduler == NULL || !scheduler->initialized || scheduler->config.on_snapshot == NULL ||
        scheduler->running) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "invalid scheduler start");
        return false;
    }

    scheduler->stop_requested = false;
    result = pthread_create(&scheduler->worker, NULL, scheduler_worker, scheduler);
    if (result != 0) {
        symon_error_set(error, SYMON_ERROR_THREAD, result, "cannot start scheduler worker");
        return false;
    }

    scheduler->running = true;
    return true;
}

void symon_scheduler_stop(SymonScheduler *scheduler)
{
    if (scheduler == NULL || !scheduler->initialized || !scheduler->running) {
        return;
    }

    pthread_mutex_lock(&scheduler->mutex);
    scheduler->stop_requested = true;
    pthread_cond_signal(&scheduler->condition);
    pthread_mutex_unlock(&scheduler->mutex);
    pthread_join(scheduler->worker, NULL);
    scheduler->running = false;
}

void symon_scheduler_destroy(SymonScheduler *scheduler)
{
    if (scheduler == NULL || !scheduler->initialized) {
        return;
    }

    symon_scheduler_stop(scheduler);
    pthread_cond_destroy(&scheduler->condition);
    pthread_mutex_destroy(&scheduler->mutex);
    scheduler->initialized = false;
}
