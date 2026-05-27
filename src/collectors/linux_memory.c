#include "linux_memory.h"

#include "linux_file.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    uint64_t value_kib;
    bool found;
} MemoryField;

static bool parse_field(const char *contents, MemoryField *field, SymonError *error)
{
    size_t name_length = strlen(field->name);
    const char *cursor = contents;

    while (*cursor != '\0') {
        if (strncmp(cursor, field->name, name_length) == 0 && cursor[name_length] == ':') {
            char *end = NULL;
            unsigned long long value;

            cursor += name_length + 1U;
            errno = 0;
            value = strtoull(cursor, &end, 10);
            if (end == cursor || errno != 0) {
                symon_error_set(error, SYMON_ERROR_IO, 0, "invalid memory value");
                return false;
            }
            field->value_kib = (uint64_t)value;
            field->found = true;
            return true;
        }

        while (*cursor != '\0' && *cursor != '\n') {
            cursor++;
        }
        if (*cursor == '\n') {
            cursor++;
        }
    }

    return true;
}

static bool kib_to_bytes(uint64_t kib, uint64_t *bytes, SymonError *error)
{
    if (kib > UINT64_MAX / UINT64_C(1024)) {
        symon_error_set(error, SYMON_ERROR_CAPACITY, 0, "memory value exceeds byte range");
        return false;
    }

    *bytes = kib * UINT64_C(1024);
    return true;
}

void symon_linux_memory_collector_init(SymonLinuxMemoryCollector *collector, const char *path)
{
    if (collector == NULL) {
        return;
    }

    collector->meminfo_path = path;
}

bool symon_linux_memory_parse(const char *contents, SymonMemoryMetrics *metrics, SymonError *error)
{
    enum {
        MEM_TOTAL,
        MEM_FREE,
        MEM_AVAILABLE,
        BUFFERS,
        CACHED,
        S_RECLAIMABLE,
        SWAP_TOTAL,
        SWAP_FREE,
        FIELD_COUNT
    };
    MemoryField fields[FIELD_COUNT] = {
        [MEM_TOTAL] = {.name = "MemTotal"},
        [MEM_FREE] = {.name = "MemFree"},
        [MEM_AVAILABLE] = {.name = "MemAvailable"},
        [BUFFERS] = {.name = "Buffers"},
        [CACHED] = {.name = "Cached"},
        [S_RECLAIMABLE] = {.name = "SReclaimable"},
        [SWAP_TOTAL] = {.name = "SwapTotal"},
        [SWAP_FREE] = {.name = "SwapFree"},
    };
    uint64_t cached_kib;
    SymonMemoryMetrics parsed = {.available = true};

    if (contents == NULL || metrics == NULL) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "invalid memory parse output");
        return false;
    }

    for (size_t index = 0; index < FIELD_COUNT; index++) {
        if (!parse_field(contents, &fields[index], error)) {
            return false;
        }
    }
    for (size_t index = 0; index < FIELD_COUNT; index++) {
        if (index != S_RECLAIMABLE && !fields[index].found) {
            symon_error_set(error, SYMON_ERROR_IO, 0, "required memory value missing");
            return false;
        }
    }
    if (fields[MEM_AVAILABLE].value_kib > fields[MEM_TOTAL].value_kib ||
        fields[SWAP_FREE].value_kib > fields[SWAP_TOTAL].value_kib ||
        fields[CACHED].value_kib > UINT64_MAX - fields[S_RECLAIMABLE].value_kib) {
        symon_error_set(error, SYMON_ERROR_IO, 0, "invalid memory relationship");
        return false;
    }

    cached_kib = fields[CACHED].value_kib + fields[S_RECLAIMABLE].value_kib;
    if (!kib_to_bytes(fields[MEM_TOTAL].value_kib, &parsed.total_bytes, error) ||
        !kib_to_bytes(fields[MEM_FREE].value_kib, &parsed.free_bytes, error) ||
        !kib_to_bytes(fields[MEM_AVAILABLE].value_kib, &parsed.available_bytes, error) ||
        !kib_to_bytes(fields[MEM_TOTAL].value_kib - fields[MEM_AVAILABLE].value_kib,
                      &parsed.used_bytes, error) ||
        !kib_to_bytes(fields[BUFFERS].value_kib, &parsed.buffers_bytes, error) ||
        !kib_to_bytes(cached_kib, &parsed.cached_bytes, error) ||
        !kib_to_bytes(fields[SWAP_TOTAL].value_kib, &parsed.swap_total_bytes, error) ||
        !kib_to_bytes(fields[SWAP_FREE].value_kib, &parsed.swap_free_bytes, error) ||
        !kib_to_bytes(fields[SWAP_TOTAL].value_kib - fields[SWAP_FREE].value_kib,
                      &parsed.swap_used_bytes, error)) {
        return false;
    }

    *metrics = parsed;
    return true;
}

bool symon_linux_memory_collect(SymonLinuxMemoryCollector *collector, SymonSnapshot *snapshot,
                                SymonError *error)
{
    char *contents = NULL;
    size_t length = 0;
    bool succeeded;

    if (collector == NULL || snapshot == NULL || collector->meminfo_path == NULL) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "invalid memory collector");
        return false;
    }

    if (!symon_linux_read_text_file(collector->meminfo_path, &contents, &length, error)) {
        return false;
    }

    succeeded = symon_linux_memory_parse(contents, &snapshot->memory, error);
    free(contents);
    return succeeded;
}

static bool collect_memory_adapter(void *context, SymonSnapshot *snapshot, SymonError *error)
{
    return symon_linux_memory_collect(context, snapshot, error);
}

SymonCollector symon_linux_memory_as_collector(SymonLinuxMemoryCollector *collector)
{
    return (SymonCollector){
        .name = "memory",
        .collect = collect_memory_adapter,
        .context = collector,
    };
}
