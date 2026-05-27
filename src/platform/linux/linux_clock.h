#pragma once

#include "error.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint64_t realtime_ms;
    uint64_t monotonic_ms;
} SymonTimestamp;

bool symon_linux_clock_now(SymonTimestamp *timestamp, SymonError *error);
