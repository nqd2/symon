#include "linux_file.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define SYMON_LINUX_TEXT_INITIAL_BYTES ((size_t)4096)
#define SYMON_LINUX_TEXT_MAX_BYTES ((size_t)1024 * (size_t)1024)

static bool grow_buffer(char **buffer, size_t *capacity, SymonError *error)
{
    size_t next_capacity = *capacity * 2U;
    char *larger_buffer;

    if (next_capacity > SYMON_LINUX_TEXT_MAX_BYTES) {
        next_capacity = SYMON_LINUX_TEXT_MAX_BYTES;
    }
    if (next_capacity <= *capacity) {
        symon_error_set(error, SYMON_ERROR_CAPACITY, 0, "Linux text input exceeds limit");
        return false;
    }

    larger_buffer = realloc(*buffer, next_capacity + 1U);
    if (larger_buffer == NULL) {
        symon_error_set(error, SYMON_ERROR_OUT_OF_MEMORY, 0, "cannot grow Linux text buffer");
        return false;
    }

    *buffer = larger_buffer;
    *capacity = next_capacity;
    return true;
}

bool symon_linux_read_text_file(const char *path, char **contents, size_t *length,
                                SymonError *error)
{
    int descriptor;
    char *buffer;
    size_t used = 0;
    size_t capacity = SYMON_LINUX_TEXT_INITIAL_BYTES;

    if (path == NULL || contents == NULL || length == NULL) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "text reader output is required");
        return false;
    }

    *contents = NULL;
    *length = 0;
    descriptor = open(path, O_RDONLY | O_CLOEXEC);
    if (descriptor < 0) {
        symon_error_set(error, SYMON_ERROR_IO, errno, "cannot open Linux text input");
        return false;
    }

    buffer = malloc(capacity + 1U);
    if (buffer == NULL) {
        (void)close(descriptor);
        symon_error_set(error, SYMON_ERROR_OUT_OF_MEMORY, 0, "cannot allocate Linux text buffer");
        return false;
    }

    for (;;) {
        ssize_t result;

        if (used == capacity) {
            if (capacity == SYMON_LINUX_TEXT_MAX_BYTES) {
                char extra_byte;

                result = read(descriptor, &extra_byte, 1);
                if (result < 0 && errno == EINTR) {
                    continue;
                }
                if (result > 0) {
                    free(buffer);
                    (void)close(descriptor);
                    symon_error_set(error, SYMON_ERROR_CAPACITY, 0,
                                    "Linux text input exceeds limit");
                    return false;
                }
                if (result < 0) {
                    free(buffer);
                    (void)close(descriptor);
                    symon_error_set(error, SYMON_ERROR_IO, errno, "cannot read Linux text input");
                    return false;
                }
                break;
            }
            if (!grow_buffer(&buffer, &capacity, error)) {
                free(buffer);
                (void)close(descriptor);
                return false;
            }
        }

        result = read(descriptor, buffer + used, capacity - used);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            free(buffer);
            (void)close(descriptor);
            symon_error_set(error, SYMON_ERROR_IO, errno, "cannot read Linux text input");
            return false;
        }
        if (result == 0) {
            break;
        }

        used += (size_t)result;
    }

    (void)close(descriptor);
    buffer[used] = '\0';
    *contents = buffer;
    *length = used;
    return true;
}
