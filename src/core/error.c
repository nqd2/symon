#include "error.h"

#include <stddef.h>

void symon_error_clear(SymonError *error)
{
    if (error == NULL) {
        return;
    }

    error->code = SYMON_ERROR_NONE;
    error->system_errno = 0;
    error->message[0] = '\0';
}

bool symon_error_is_set(const SymonError *error)
{
    return error != NULL && error->code != SYMON_ERROR_NONE;
}

void symon_error_set(SymonError *error, SymonErrorCode code, int system_errno, const char *message)
{
    size_t index = 0;

    if (error == NULL) {
        return;
    }

    error->code = code;
    error->system_errno = system_errno;
    if (message == NULL) {
        error->message[0] = '\0';
        return;
    }

    while (index < sizeof(error->message) - 1U && message[index] != '\0') {
        error->message[index] = message[index];
        index++;
    }
    error->message[index] = '\0';
}

const char *symon_error_code_name(SymonErrorCode code)
{
    switch (code) {
    case SYMON_ERROR_NONE:
        return "none";
    case SYMON_ERROR_INVALID_ARGUMENT:
        return "invalid_argument";
    case SYMON_ERROR_OUT_OF_MEMORY:
        return "out_of_memory";
    case SYMON_ERROR_IO:
        return "io";
    case SYMON_ERROR_CLOCK:
        return "clock";
    case SYMON_ERROR_COLLECTOR:
        return "collector";
    case SYMON_ERROR_THREAD:
        return "thread";
    case SYMON_ERROR_CAPACITY:
        return "capacity";
    }

    return "unknown";
}
