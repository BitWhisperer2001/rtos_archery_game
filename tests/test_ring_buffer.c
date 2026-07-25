#include <assert.h>
#include <stddef.h>

#include "ring_buffer.h"

int main(void)
{
    ring_buffer_t ring = {0};
    int storage[3] = {0};
    int value = 0;

    /* Invalid ownership is rejected without changing external memory. */
    assert(!ring_buffer_init(NULL, storage, 3U));
    assert(!ring_buffer_init(&ring, NULL, 3U));
    assert(!ring_buffer_init(&ring, storage, 0U));

    assert(ring_buffer_init(&ring, storage, 3U));
    assert(ring_buffer_is_empty(&ring));
    assert(!ring_buffer_is_full(&ring));
    assert(ring_buffer_count(&ring) == 0U);
    assert(!ring_buffer_get(&ring, &value));

    /* Fill, reject overflow, then verify FIFO order across index wraparound. */
    assert(ring_buffer_post(&ring, 10));
    assert(ring_buffer_post(&ring, -1));
    assert(ring_buffer_post(&ring, 30));
    assert(ring_buffer_is_full(&ring));
    assert(!ring_buffer_post(&ring, 40));

    assert(ring_buffer_get(&ring, &value) && (value == 10));
    assert(ring_buffer_post(&ring, 40));
    assert(ring_buffer_get(&ring, &value) && (value == -1));
    assert(ring_buffer_get(&ring, &value) && (value == 30));
    assert(ring_buffer_get(&ring, &value) && (value == 40));
    assert(ring_buffer_is_empty(&ring));
    return 0;
}
