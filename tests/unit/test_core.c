#include "error.h"
#include "ring_buffer.h"
#include "snapshot.h"

#include <stdint.h>
#include <string.h>

#define REQUIRE(condition)                                                                         \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

static int test_error_state(void)
{
    SymonError error;

    symon_error_clear(&error);
    REQUIRE(!symon_error_is_set(&error));

    symon_error_set(&error, SYMON_ERROR_INVALID_ARGUMENT, 0, "bad interval");
    REQUIRE(symon_error_is_set(&error));
    REQUIRE(error.code == SYMON_ERROR_INVALID_ARGUMENT);
    REQUIRE(strcmp(error.message, "bad interval") == 0);
    REQUIRE(strcmp(symon_error_code_name(error.code), "invalid_argument") == 0);

    return 0;
}

static int test_snapshot_reset(void)
{
    SymonSnapshot snapshot = {
        .sequence = 99,
        .collectors_completed = 2,
        .collectors_failed = 1,
    };

    symon_error_set(&snapshot.last_collector_error, SYMON_ERROR_COLLECTOR, 0, "failed");
    symon_snapshot_reset(&snapshot, 7, 1000, 900);

    REQUIRE(snapshot.sequence == 7);
    REQUIRE(snapshot.realtime_ms == 1000);
    REQUIRE(snapshot.monotonic_ms == 900);
    REQUIRE(snapshot.collectors_completed == 0);
    REQUIRE(snapshot.collectors_failed == 0);
    REQUIRE(!symon_error_is_set(&snapshot.last_collector_error));

    return 0;
}

static int test_ring_buffer_order_and_overwrite(void)
{
    SymonRingBuffer buffer;
    SymonError error;
    const int first = 10;
    const int second = 20;
    const int third = 30;
    int value = 0;

    symon_error_clear(&error);
    REQUIRE(symon_ring_buffer_init(&buffer, 2, sizeof(int), &error));
    REQUIRE(symon_ring_buffer_count(&buffer) == 0);
    REQUIRE(symon_ring_buffer_push(&buffer, &first, &error));
    REQUIRE(symon_ring_buffer_push(&buffer, &second, &error));
    REQUIRE(symon_ring_buffer_push(&buffer, &third, &error));
    REQUIRE(symon_ring_buffer_count(&buffer) == 2);

    REQUIRE(symon_ring_buffer_get(&buffer, 0, &value, &error));
    REQUIRE(value == second);
    REQUIRE(symon_ring_buffer_get(&buffer, 1, &value, &error));
    REQUIRE(value == third);
    REQUIRE(!symon_ring_buffer_get(&buffer, 2, &value, &error));
    REQUIRE(error.code == SYMON_ERROR_INVALID_ARGUMENT);

    symon_ring_buffer_destroy(&buffer);
    return 0;
}

static int test_ring_buffer_rejects_empty_capacity(void)
{
    SymonRingBuffer buffer;
    SymonError error;

    symon_error_clear(&error);
    REQUIRE(!symon_ring_buffer_init(&buffer, 0, sizeof(uint64_t), &error));
    REQUIRE(error.code == SYMON_ERROR_INVALID_ARGUMENT);

    return 0;
}

int main(void)
{
    REQUIRE(test_error_state() == 0);
    REQUIRE(test_snapshot_reset() == 0);
    REQUIRE(test_ring_buffer_order_and_overwrite() == 0);
    REQUIRE(test_ring_buffer_rejects_empty_capacity() == 0);
    return 0;
}
