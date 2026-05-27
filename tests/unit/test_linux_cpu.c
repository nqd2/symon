#include "linux_cpu.h"
#include "linux_memory.h"
#include "scheduler.h"
#include "snapshot.h"

#include <string.h>

#define REQUIRE(condition)                                                                         \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

static int test_parse_stat_and_load_average(void)
{
    static const char stat_text[] = "cpu  100 20 30 400 5 6 7 0 0 0\n"
                                    "cpu0 60 10 15 200 3 4 5 0 0 0\n"
                                    "cpu1 40 10 15 200 2 2 2 0 0 0\n";
    SymonCpuCounters total;
    SymonCpuCounters cores[SYMON_CPU_MAX_CORES];
    size_t core_count = 0;
    SymonError error;
    double averages[3] = {0};

    symon_error_clear(&error);
    REQUIRE(symon_linux_cpu_parse_stat(stat_text, &total, cores, &core_count, &error));
    REQUIRE(core_count == 2);
    REQUIRE(total.user == 100);
    REQUIRE(total.idle == 400);
    REQUIRE(cores[1].user == 40);
    REQUIRE(symon_linux_cpu_parse_loadavg("1.25 0.75 0.50 2/512 24680\n", averages, &error));
    REQUIRE(averages[0] > 1.24 && averages[0] < 1.26);
    REQUIRE(averages[2] > 0.49 && averages[2] < 0.51);

    return 0;
}

static int test_collector_calculates_second_sample_delta(void)
{
    SymonLinuxCpuCollector context;
    SymonSnapshot first;
    SymonSnapshot second;
    SymonError error;

    symon_linux_cpu_collector_init(&context, "tests/fixtures/linux/proc-stat.sample",
                                   "tests/fixtures/linux/proc-loadavg.sample");
    symon_snapshot_reset(&first, 1, 1, 1);
    symon_error_clear(&error);
    REQUIRE(symon_linux_cpu_collect(&context, &first, &error));
    REQUIRE(first.cpu.available);
    REQUIRE(!first.cpu.usage_available);
    REQUIRE(first.cpu.core_count == 2);
    REQUIRE(first.cpu.load_average_1m > 1.24 && first.cpu.load_average_1m < 1.26);

    context.stat_path = "tests/fixtures/linux/proc-stat-next.sample";
    symon_snapshot_reset(&second, 2, 2, 2);
    REQUIRE(symon_linux_cpu_collect(&context, &second, &error));
    REQUIRE(second.cpu.usage_available);
    REQUIRE(second.cpu.total_usage_percent > 62.0 && second.cpu.total_usage_percent < 62.2);
    REQUIRE(second.cpu.core_usage_percent[0] > 67.4 && second.cpu.core_usage_percent[0] < 67.7);
    REQUIRE(second.cpu.core_usage_percent[1] > 54.4 && second.cpu.core_usage_percent[1] < 54.7);

    return 0;
}

static int test_collector_descriptor_calls_cpu_collector(void)
{
    SymonLinuxCpuCollector context;
    SymonCollector collector;

    symon_linux_cpu_collector_init(&context, "tests/fixtures/linux/proc-stat.sample",
                                   "tests/fixtures/linux/proc-loadavg.sample");
    collector = symon_linux_cpu_as_collector(&context);
    REQUIRE(strcmp(collector.name, "cpu") == 0);
    REQUIRE(collector.context == &context);
    REQUIRE(collector.collect != NULL);

    return 0;
}

static int test_scheduler_collects_cpu_and_memory(void)
{
    SymonLinuxCpuCollector cpu;
    SymonLinuxMemoryCollector memory;
    SymonScheduler scheduler;
    SymonSchedulerConfig config = {.interval_ms = 500};
    SymonSnapshot snapshot;
    SymonError error;

    symon_linux_cpu_collector_init(&cpu, "tests/fixtures/linux/proc-stat.sample",
                                   "tests/fixtures/linux/proc-loadavg.sample");
    symon_linux_memory_collector_init(&memory, "tests/fixtures/linux/proc-meminfo.sample");
    symon_error_clear(&error);
    REQUIRE(symon_scheduler_init(&scheduler, &config, &error));
    REQUIRE(symon_scheduler_add_collector(&scheduler, symon_linux_cpu_as_collector(&cpu), &error));
    REQUIRE(symon_scheduler_add_collector(&scheduler, symon_linux_memory_as_collector(&memory),
                                          &error));
    REQUIRE(symon_scheduler_collect_once(&scheduler, &snapshot, &error));
    REQUIRE(snapshot.collectors_completed == 2);
    REQUIRE(snapshot.collectors_failed == 0);
    REQUIRE(snapshot.cpu.available);
    REQUIRE(snapshot.memory.available);
    symon_scheduler_destroy(&scheduler);

    return 0;
}

int main(void)
{
    REQUIRE(test_parse_stat_and_load_average() == 0);
    REQUIRE(test_collector_calculates_second_sample_delta() == 0);
    REQUIRE(test_collector_descriptor_calls_cpu_collector() == 0);
    REQUIRE(test_scheduler_collects_cpu_and_memory() == 0);
    return 0;
}
