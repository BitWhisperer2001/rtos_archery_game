#include "ring_buffer.h"

bool ring_buffer_init(ring_buffer_t *ring, int *storage, size_t capacity)
{
    /* Reject invalid ownership up front; a zero-capacity modulo is undefined. */
    if ((ring == NULL) || (storage == NULL) || (capacity == 0U)) {
        return false;
    }

    ring->buffer = storage;
    ring->capacity = capacity;
    ring->head = 0U;
    ring->tail = 0U;
    ring->count = 0U;
    return true;
}

bool ring_buffer_post(ring_buffer_t *ring, int value)
{
    /*
     * Never overwrite unread data silently. The caller explicitly chooses how
     * to handle back-pressure when the fixed-capacity container is full.
     */
    if ((ring == NULL) || (ring->buffer == NULL) ||
        ring_buffer_is_full(ring)) {
        return false;
    }

    ring->buffer[ring->tail] = value;
    ring->tail = (ring->tail + 1U) % ring->capacity;
    ring->count++;
    return true;
}

bool ring_buffer_get(ring_buffer_t *ring, int *value)
{
    /* An output parameter separates empty-state from every valid int value. */
    if ((ring == NULL) || (ring->buffer == NULL) || (value == NULL) ||
        ring_buffer_is_empty(ring)) {
        return false;
    }

    *value = ring->buffer[ring->head];
    ring->head = (ring->head + 1U) % ring->capacity;
    ring->count--;
    return true;
}

bool ring_buffer_is_full(const ring_buffer_t *ring)
{
    return (ring != NULL) && (ring->capacity != 0U) &&
           (ring->count == ring->capacity);
}

bool ring_buffer_is_empty(const ring_buffer_t *ring)
{
    return (ring == NULL) || (ring->count == 0U);
}

size_t ring_buffer_count(const ring_buffer_t *ring)
{
    return (ring != NULL) ? ring->count : 0U;
}
