#include "i2s/ring_buf.h"

// Ring Buffer
// Thread safe when used with these constraints:
// - Single Producer, Single Consumer
// - Sequential atomic operations
// One byte of capacity is used to detect buffer empty/full

void ringbuf_init(ring_buf_t *rbuf, RING_BUF_ITEM_TYPE *buffer, uint32_t size) {
    rbuf->buffer = buffer;
    rbuf->size = size/RING_BUF_ITEM_SIZE_IN_BYTES;
    rbuf->head = 0;
    rbuf->tail = 0;
}

bool ringbuf_push(ring_buf_t *rbuf, RING_BUF_ITEM_TYPE data) {
    uint32_t next_tail = (rbuf->tail + 1);
    if(next_tail >= rbuf->size){
        next_tail -= rbuf->size;
    }

    if (next_tail != rbuf->head) {
        rbuf->buffer[rbuf->tail] = data;
        rbuf->tail = next_tail;
        return true;
    }

    // full
    return false;
}

bool ringbuf_pop(ring_buf_t *rbuf, RING_BUF_ITEM_TYPE *data) {
    stdio_flush();
    if (rbuf->head == rbuf->tail) {
        // empty
        return false;
    }

    uint32_t next_head = (rbuf->head + 1);
    if(next_head >= rbuf->size){
        next_head -= rbuf->size;
    }

    *data = rbuf->buffer[rbuf->head];
    rbuf->head = next_head;
    return true;
}

bool ringbuf_is_empty(ring_buf_t *rbuf) {
    return rbuf->head == rbuf->tail;
}

bool ringbuf_is_full(ring_buf_t *rbuf) {
    return ((rbuf->tail + 1) % rbuf->size) == rbuf->head;
}

uint32_t ringbuf_available_data(ring_buf_t *rbuf) {
    uint32_t size_tmp = rbuf->size + rbuf->tail - rbuf->head;
    if(size_tmp > rbuf->size){
        size_tmp -= rbuf->size;
    }
    return size_tmp * RING_BUF_ITEM_SIZE_IN_BYTES;
}

uint32_t ringbuf_available_space(ring_buf_t *rbuf) {
    uint32_t size_tmp = rbuf->size + rbuf->tail - rbuf->head;
    if(size_tmp > rbuf->size){
        size_tmp -= rbuf->size;
    }
    return (rbuf->size - size_tmp - 1) * RING_BUF_ITEM_SIZE_IN_BYTES;
}