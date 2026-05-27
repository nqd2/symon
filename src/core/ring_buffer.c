#include "ring_buffer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

bool symon_ring_buffer_init(SymonRingBuffer *buffer, size_t capacity, size_t element_size,
                            SymonError *error)
{
    if (buffer == NULL || capacity == 0 || element_size == 0 ||
        capacity > SIZE_MAX / element_size) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "invalid ring buffer dimensions");
        return false;
    }

    buffer->storage = calloc(capacity, element_size);
    if (buffer->storage == NULL) {
        symon_error_set(error, SYMON_ERROR_OUT_OF_MEMORY, 0, "cannot allocate ring buffer");
        return false;
    }

    buffer->element_size = element_size;
    buffer->capacity = capacity;
    buffer->count = 0;
    buffer->head = 0;
    return true;
}

void symon_ring_buffer_destroy(SymonRingBuffer *buffer)
{
    if (buffer == NULL) {
        return;
    }

    free(buffer->storage);
    buffer->storage = NULL;
    buffer->element_size = 0;
    buffer->capacity = 0;
    buffer->count = 0;
    buffer->head = 0;
}

size_t symon_ring_buffer_count(const SymonRingBuffer *buffer)
{
    return buffer == NULL ? 0 : buffer->count;
}

bool symon_ring_buffer_push(SymonRingBuffer *buffer, const void *value, SymonError *error)
{
    size_t position;

    if (buffer == NULL || buffer->storage == NULL || value == NULL) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "invalid ring buffer push");
        return false;
    }

    position = (buffer->head + buffer->count) % buffer->capacity;
    if (buffer->count == buffer->capacity) {
        position = buffer->head;
        buffer->head = (buffer->head + 1) % buffer->capacity;
    } else {
        buffer->count++;
    }

    // Initialization validated destination dimensions and push copies one complete element.
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memcpy(buffer->storage + (position * buffer->element_size), value, buffer->element_size);
    return true;
}

bool symon_ring_buffer_get(const SymonRingBuffer *buffer, size_t index, void *value,
                           SymonError *error)
{
    size_t position;

    if (buffer == NULL || buffer->storage == NULL || value == NULL || index >= buffer->count) {
        symon_error_set(error, SYMON_ERROR_INVALID_ARGUMENT, 0, "invalid ring buffer index");
        return false;
    }

    position = (buffer->head + index) % buffer->capacity;
    // Caller supplies one output element; source dimensions were validated at initialization.
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memcpy(value, buffer->storage + (position * buffer->element_size), buffer->element_size);
    return true;
}
