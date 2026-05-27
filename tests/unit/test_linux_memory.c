#include "linux_memory.h"
#include "snapshot.h"

#include <string.h>

#define REQUIRE(condition)                                                                         \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

static int test_memory_collector_reads_fixture(void)
{
    SymonLinuxMemoryCollector context;
    SymonSnapshot snapshot;
    SymonError error;

    symon_linux_memory_collector_init(&context, "tests/fixtures/linux/proc-meminfo.sample");
    symon_snapshot_reset(&snapshot, 1, 1, 1);
    symon_error_clear(&error);
    REQUIRE(symon_linux_memory_collect(&context, &snapshot, &error));
    REQUIRE(snapshot.memory.available);
    REQUIRE(snapshot.memory.total_bytes == UINT64_C(16384000000));
    REQUIRE(snapshot.memory.free_bytes == UINT64_C(1024000000));
    REQUIRE(snapshot.memory.available_bytes == UINT64_C(6144000000));
    REQUIRE(snapshot.memory.used_bytes == UINT64_C(10240000000));
    REQUIRE(snapshot.memory.buffers_bytes == UINT64_C(102400000));
    REQUIRE(snapshot.memory.cached_bytes == UINT64_C(2764800000));
    REQUIRE(snapshot.memory.swap_total_bytes == UINT64_C(2048000000));
    REQUIRE(snapshot.memory.swap_used_bytes == UINT64_C(1536000000));

    return 0;
}

static int test_memory_descriptor(void)
{
    SymonLinuxMemoryCollector context;
    SymonCollector collector;

    symon_linux_memory_collector_init(&context, "tests/fixtures/linux/proc-meminfo.sample");
    collector = symon_linux_memory_as_collector(&context);
    REQUIRE(strcmp(collector.name, "memory") == 0);
    REQUIRE(collector.context == &context);
    REQUIRE(collector.collect != NULL);

    return 0;
}

int main(void)
{
    REQUIRE(test_memory_collector_reads_fixture() == 0);
    REQUIRE(test_memory_descriptor() == 0);
    return 0;
}
