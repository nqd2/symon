#pragma once

#include <stdbool.h>

#define SYMON_ERROR_MESSAGE_SIZE 192

typedef enum {
    SYMON_ERROR_NONE = 0,
    SYMON_ERROR_INVALID_ARGUMENT,
    SYMON_ERROR_OUT_OF_MEMORY,
    SYMON_ERROR_IO,
    SYMON_ERROR_CLOCK,
    SYMON_ERROR_COLLECTOR,
    SYMON_ERROR_THREAD,
    SYMON_ERROR_CAPACITY
} SymonErrorCode;

typedef struct {
    SymonErrorCode code;
    int system_errno;
    char message[SYMON_ERROR_MESSAGE_SIZE];
} SymonError;

void symon_error_clear(SymonError *error);
bool symon_error_is_set(const SymonError *error);
void symon_error_set(SymonError *error, SymonErrorCode code, int system_errno, const char *message);
const char *symon_error_code_name(SymonErrorCode code);
