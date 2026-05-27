#include "linux_clock.h"

#include <errno.h>
#include <time.h>

static uint64_t timespec_to_milliseconds(const struct timespec *value)
{
    return ((uint64_t)value->tv_sec * 1000U) + ((uint64_t)value->tv_nsec / 1000000U);
}

bool symon_linux_clock_now(SymonTimestamp *timestamp, SymonError *error)
{
    struct timespec realtime;
    struct timespec monotonic;

    if (timestamp == NULL) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "timestamp output is required");
        return false;
    }

    if (clock_gettime(CLOCK_REALTIME, &realtime) != 0 ||
        clock_gettime(CLOCK_MONOTONIC, &monotonic) != 0) {
        symon_error_set(error, SYMON_ERROR_CLOCK, errno, "failed to read Linux clock");
        return false;
    }

    timestamp->realtime_ms = timespec_to_milliseconds(&realtime);
    timestamp->monotonic_ms = timespec_to_milliseconds(&monotonic);
    return true;
}
