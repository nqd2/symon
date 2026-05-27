#pragma once

#include "error.h"
#include "snapshot.h"

#include <stdbool.h>

typedef bool (*SymonCollectorCollectFn)(void *context, SymonSnapshot *snapshot, SymonError *error);

typedef struct {
    const char *name;
    SymonCollectorCollectFn collect;
    void *context;
} SymonCollector;
