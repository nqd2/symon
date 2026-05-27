#include "linux_cpu.h"

#include "linux_file.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static void skip_spaces(const char **cursor)
{
    while (**cursor == ' ' || **cursor == '\t') {
        (*cursor)++;
    }
}

static bool read_uint64(const char **cursor, uint64_t *value)
{
    char *end = NULL;
    unsigned long long parsed;

    skip_spaces(cursor);
    errno = 0;
    parsed = strtoull(*cursor, &end, 10);
    if (end == *cursor || errno != 0) {
        return false;
    }

    *value = (uint64_t)parsed;
    *cursor = end;
    return true;
}

static bool parse_counters(const char *cursor, SymonCpuCounters *counters)
{
    return read_uint64(&cursor, &counters->user) && read_uint64(&cursor, &counters->nice) &&
           read_uint64(&cursor, &counters->system) && read_uint64(&cursor, &counters->idle) &&
           read_uint64(&cursor, &counters->iowait) && read_uint64(&cursor, &counters->irq) &&
           read_uint64(&cursor, &counters->softirq) && read_uint64(&cursor, &counters->steal);
}

static uint64_t total_ticks(const SymonCpuCounters *counters)
{
    return counters->user + counters->nice + counters->system + counters->idle + counters->iowait +
           counters->irq + counters->softirq + counters->steal;
}

static uint64_t idle_ticks(const SymonCpuCounters *counters)
{
    return counters->idle + counters->iowait;
}

static bool utilization(const SymonCpuCounters *previous, const SymonCpuCounters *current,
                        double *percent)
{
    uint64_t previous_total = total_ticks(previous);
    uint64_t current_total = total_ticks(current);
    uint64_t previous_idle = idle_ticks(previous);
    uint64_t current_idle = idle_ticks(current);
    uint64_t delta_total;
    uint64_t delta_idle;

    if (current_total < previous_total || current_idle < previous_idle) {
        return false;
    }

    delta_total = current_total - previous_total;
    delta_idle = current_idle - previous_idle;
    if (delta_total == 0 || delta_idle > delta_total) {
        return false;
    }

    *percent = ((double)(delta_total - delta_idle) * 100.0) / (double)delta_total;
    return true;
}

void symon_linux_cpu_collector_init(SymonLinuxCpuCollector *collector, const char *stat_path,
                                    const char *loadavg_path)
{
    if (collector == NULL) {
        return;
    }

    *collector = (SymonLinuxCpuCollector){
        .stat_path = stat_path,
        .loadavg_path = loadavg_path,
    };
}

bool symon_linux_cpu_parse_stat(const char *contents, SymonCpuCounters *total,
                                SymonCpuCounters *cores, size_t *core_count, SymonError *error)
{
    const char *cursor;
    bool parsed_total = false;

    if (contents == NULL || total == NULL || cores == NULL || core_count == NULL) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "invalid CPU parse output");
        return false;
    }

    cursor = contents;
    *core_count = 0;
    while (*cursor != '\0') {
        if (strncmp(cursor, "cpu", 3) != 0) {
            while (*cursor != '\0' && *cursor != '\n') {
                cursor++;
            }
            if (*cursor == '\n') {
                cursor++;
            }
            continue;
        }

        if (cursor[3] == ' ' || cursor[3] == '\t') {
            if (!parse_counters(cursor + 3, total)) {
                symon_error_set(error, SYMON_ERROR_IO, 0, "invalid aggregate CPU counters");
                return false;
            }
            parsed_total = true;
        } else if (isdigit((unsigned char)cursor[3])) {
            const char *values = cursor + 3;

            if (*core_count == SYMON_CPU_MAX_CORES) {
                symon_error_set(error, SYMON_ERROR_CAPACITY, 0, "CPU core count exceeds limit");
                return false;
            }
            while (isdigit((unsigned char)*values)) {
                values++;
            }
            if (!parse_counters(values, &cores[*core_count])) {
                symon_error_set(error, SYMON_ERROR_IO, 0, "invalid per-core CPU counters");
                return false;
            }
            (*core_count)++;
        }

        while (*cursor != '\0' && *cursor != '\n') {
            cursor++;
        }
        if (*cursor == '\n') {
            cursor++;
        }
    }

    if (!parsed_total) {
        symon_error_set(error, SYMON_ERROR_IO, 0, "aggregate CPU counters missing");
        return false;
    }

    return true;
}

bool symon_linux_cpu_parse_loadavg(const char *contents, double averages[3], SymonError *error)
{
    const char *cursor = contents;

    if (contents == NULL || averages == NULL) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "invalid load average output");
        return false;
    }

    for (size_t index = 0; index < 3; index++) {
        char *end = NULL;

        skip_spaces(&cursor);
        errno = 0;
        averages[index] = strtod(cursor, &end);
        if (end == cursor || errno != 0) {
            symon_error_set(error, SYMON_ERROR_IO, 0, "invalid load average input");
            return false;
        }
        cursor = end;
    }

    return true;
}

bool symon_linux_cpu_collect(SymonLinuxCpuCollector *collector, SymonSnapshot *snapshot,
                             SymonError *error)
{
    SymonCpuCounters total;
    SymonCpuCounters cores[SYMON_CPU_MAX_CORES];
    size_t core_count = 0;
    double averages[3];
    char *stat_contents = NULL;
    char *loadavg_contents = NULL;
    size_t length = 0;
    bool succeeded = false;

    if (collector == NULL || snapshot == NULL || collector->stat_path == NULL ||
        collector->loadavg_path == NULL) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "invalid CPU collector");
        return false;
    }

    if (!symon_linux_read_text_file(collector->stat_path, &stat_contents, &length, error) ||
        !symon_linux_cpu_parse_stat(stat_contents, &total, cores, &core_count, error) ||
        !symon_linux_read_text_file(collector->loadavg_path, &loadavg_contents, &length, error) ||
        !symon_linux_cpu_parse_loadavg(loadavg_contents, averages, error)) {
        goto cleanup;
    }

    snapshot->cpu.available = true;
    snapshot->cpu.core_count = core_count;
    snapshot->cpu.load_average_1m = averages[0];
    snapshot->cpu.load_average_5m = averages[1];
    snapshot->cpu.load_average_15m = averages[2];
    if (collector->has_previous) {
        snapshot->cpu.usage_available =
            utilization(&collector->previous_total, &total, &snapshot->cpu.total_usage_percent);
        if (collector->previous_core_count == core_count) {
            for (size_t index = 0; index < core_count; index++) {
                if (!utilization(&collector->previous_cores[index], &cores[index],
                                 &snapshot->cpu.core_usage_percent[index])) {
                    snapshot->cpu.usage_available = false;
                }
            }
        } else {
            snapshot->cpu.usage_available = false;
        }
    }

    collector->previous_total = total;
    collector->previous_core_count = core_count;
    for (size_t index = 0; index < core_count; index++) {
        collector->previous_cores[index] = cores[index];
    }
    collector->has_previous = true;
    succeeded = true;

cleanup:
    free(stat_contents);
    free(loadavg_contents);
    return succeeded;
}

static bool collect_cpu_adapter(void *context, SymonSnapshot *snapshot, SymonError *error)
{
    return symon_linux_cpu_collect(context, snapshot, error);
}

SymonCollector symon_linux_cpu_as_collector(SymonLinuxCpuCollector *collector)
{
    return (SymonCollector){
        .name = "cpu",
        .collect = collect_cpu_adapter,
        .context = collector,
    };
}
