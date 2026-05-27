#include "collector.h"
#include "error.h"
#include "scheduler.h"

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#define REQUIRE(condition)                                                                         \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

typedef struct {
    unsigned int calls;
    bool fail;
} FakeCollectorState;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    unsigned int callbacks;
    SymonSnapshot snapshot;
} PublishState;

static bool fake_collect(void *context, SymonSnapshot *snapshot, SymonError *error)
{
    FakeCollectorState *state = context;

    (void)snapshot;
    state->calls++;
    if (state->fail) {
        symon_error_set(error, SYMON_ERROR_COLLECTOR, 0, "synthetic collector failure");
        return false;
    }

    return true;
}

static void record_snapshot(const SymonSnapshot *snapshot, void *user_data)
{
    PublishState *state = user_data;

    pthread_mutex_lock(&state->mutex);
    state->snapshot = *snapshot;
    state->callbacks++;
    pthread_cond_signal(&state->condition);
    pthread_mutex_unlock(&state->mutex);
}

static int test_collect_once_counts_collector_outcomes(void)
{
    FakeCollectorState healthy = {0};
    FakeCollectorState failing = {.fail = true};
    SymonCollector first = {.name = "healthy", .collect = fake_collect, .context = &healthy};
    SymonCollector second = {.name = "failing", .collect = fake_collect, .context = &failing};
    SymonScheduler scheduler;
    SymonSchedulerConfig config = {.interval_ms = 500};
    SymonSnapshot snapshot;
    SymonError error;

    symon_error_clear(&error);
    REQUIRE(symon_scheduler_init(&scheduler, &config, &error));
    REQUIRE(symon_scheduler_add_collector(&scheduler, first, &error));
    REQUIRE(symon_scheduler_add_collector(&scheduler, second, &error));
    REQUIRE(symon_scheduler_collect_once(&scheduler, &snapshot, &error));
    REQUIRE(snapshot.sequence == 1);
    REQUIRE(snapshot.collectors_completed == 1);
    REQUIRE(snapshot.collectors_failed == 1);
    REQUIRE(snapshot.last_collector_error.code == SYMON_ERROR_COLLECTOR);
    REQUIRE(healthy.calls == 1);
    REQUIRE(failing.calls == 1);
    symon_scheduler_destroy(&scheduler);

    return 0;
}

static int test_worker_publishes_snapshot(void)
{
    FakeCollectorState collector_state = {0};
    SymonCollector collector = {
        .name = "worker",
        .collect = fake_collect,
        .context = &collector_state,
    };
    PublishState publish = {
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .condition = PTHREAD_COND_INITIALIZER,
    };
    SymonScheduler scheduler;
    SymonSchedulerConfig config = {
        .interval_ms = 10,
        .on_snapshot = record_snapshot,
        .user_data = &publish,
    };
    SymonError error;
    struct timespec deadline;
    int wait_result = 0;

    symon_error_clear(&error);
    REQUIRE(symon_scheduler_init(&scheduler, &config, &error));
    REQUIRE(symon_scheduler_add_collector(&scheduler, collector, &error));
    REQUIRE(symon_scheduler_start(&scheduler, &error));

    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 2;
    pthread_mutex_lock(&publish.mutex);
    while (publish.callbacks == 0 && wait_result == 0) {
        wait_result = pthread_cond_timedwait(&publish.condition, &publish.mutex, &deadline);
    }
    pthread_mutex_unlock(&publish.mutex);

    symon_scheduler_stop(&scheduler);
    REQUIRE(wait_result != ETIMEDOUT);
    REQUIRE(publish.callbacks > 0);
    REQUIRE(publish.snapshot.collectors_completed == 1);
    REQUIRE(collector_state.calls > 0);

    symon_scheduler_destroy(&scheduler);
    pthread_cond_destroy(&publish.condition);
    pthread_mutex_destroy(&publish.mutex);

    return 0;
}

int main(void)
{
    REQUIRE(test_collect_once_counts_collector_outcomes() == 0);
    REQUIRE(test_worker_publishes_snapshot() == 0);
    return 0;
}
