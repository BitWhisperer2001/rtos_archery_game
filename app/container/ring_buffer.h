#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Storage is supplied by the caller, making ownership and RAM use explicit.
 * The container performs no hidden libc allocation and is safe before RTOS init.
 */
typedef struct
{
    int *buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
} ring_buffer_t;

bool ring_buffer_init(ring_buffer_t *ring, int *storage, size_t capacity);
bool ring_buffer_post(ring_buffer_t *ring, int value);
bool ring_buffer_get(ring_buffer_t *ring, int *value);
bool ring_buffer_is_full(const ring_buffer_t *ring);
bool ring_buffer_is_empty(const ring_buffer_t *ring);
size_t ring_buffer_count(const ring_buffer_t *ring);

#ifdef __cplusplus
}
#endif

#endif /* RING_BUFFER_H */
