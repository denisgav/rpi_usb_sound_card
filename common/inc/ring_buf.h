#ifndef RING_BUF__H
#define RING_BUF__H

#include <stdio.h>
#include <stdbool.h>

#define RING_BUF_ITEM_TYPE uint32_t
#define RING_BUF_ITEM_SIZE_IN_BYTES (sizeof(RING_BUF_ITEM_TYPE))

typedef struct _ring_buf_t {
    RING_BUF_ITEM_TYPE *buffer;
    uint32_t head;
    uint32_t tail;
    uint32_t size;
} ring_buf_t;

void ringbuf_init(ring_buf_t *rbuf, RING_BUF_ITEM_TYPE *buffer, uint32_t size);

bool ringbuf_push(ring_buf_t *rbuf, RING_BUF_ITEM_TYPE data);

bool ringbuf_pop(ring_buf_t *rbuf, RING_BUF_ITEM_TYPE *data);

bool ringbuf_is_empty(ring_buf_t *rbuf);

bool ringbuf_is_full(ring_buf_t *rbuf);

uint32_t ringbuf_available_data(ring_buf_t *rbuf);

uint32_t ringbuf_available_space(ring_buf_t *rbuf);


#endif //RING_BUF__H