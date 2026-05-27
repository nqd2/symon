#pragma once

#include "error.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    unsigned char *storage;
    size_t element_size;
    size_t capacity;
    size_t count;
    size_t head;
} SymonRingBuffer;

bool symon_ring_buffer_init(SymonRingBuffer *buffer, size_t capacity, size_t element_size,
                            SymonError *error);
void symon_ring_buffer_destroy(SymonRingBuffer *buffer);
size_t symon_ring_buffer_count(const SymonRingBuffer *buffer);
bool symon_ring_buffer_push(SymonRingBuffer *buffer, const void *value, SymonError *error);
bool symon_ring_buffer_get(const SymonRingBuffer *buffer, size_t index, void *value,
                           SymonError *error);
