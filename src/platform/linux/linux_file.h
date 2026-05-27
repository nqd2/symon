#pragma once

#include "error.h"

#include <stdbool.h>
#include <stddef.h>

bool symon_linux_read_text_file(const char *path, char **contents, size_t *length,
                                SymonError *error);
