#ifndef RING_BUFF_H
#define RING_BUFF_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    int *buff;
    uint8_t tail;
    uint8_t head;
    uint8_t count;
    int size;
}ring_buff_t;

extern void ring_buff_init(ring_buff_t *rb, int size);
extern void ring_buff_post(ring_buff_t *rb, int data);
extern int ring_buff_get(ring_buff_t *rb);
int ring_is_full(ring_buff_t *rb);
int ring_is_empty(ring_buff_t *rb);
int ring_get_element_count(ring_buff_t *rb);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // RING_BUFF_H