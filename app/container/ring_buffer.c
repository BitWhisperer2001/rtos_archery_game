#include "ring_buffer.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

void ring_buff_init(ring_buff_t *rb, int size)
{
    rb->buff = NULL;
    rb->buff = (int*)malloc(size * sizeof(int));
    if(rb->buff == NULL){
        while(true);
    }
    rb->count = 0;
    rb->head = 0;
    rb->tail = 0;
    rb->size = size;
    for(uint8_t i = 0; i < size; i++){
        rb->buff[i] = -1;
    }
}

void ring_buff_post(ring_buff_t *rb, int data)
{
    rb->buff[rb->tail] = data;
    rb->tail = (rb->tail + 1) % rb->size;
    rb->count++;
}

int ring_buff_get(ring_buff_t *rb)
{
    int item;
    item = rb->buff[rb->head];
    rb->buff[rb->head] = -1;
    rb->head = (rb->head + 1) % rb->size;
    rb->count--;
    return item;
}

int ring_is_full(ring_buff_t *rb)
{
    return rb->count == rb->size;
}

int ring_is_empty(ring_buff_t *rb)
{
    return rb->count == 0;
}

int ring_get_element_count(ring_buff_t *rb)
{
    return rb->count;
}



