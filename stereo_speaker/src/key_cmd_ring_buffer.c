#include "key_cmd_ring_buffer.h"

int key_cmd_ring_buffer_has_item(key_cmd_ring_buffer_t *queue){
	return (queue->tail != queue->head);
}

key_cmd_ring_buffer_item_t* key_cmd_ring_buffer_queue_read(key_cmd_ring_buffer_t *queue) {
	key_cmd_ring_buffer_item_t* handle;
    if (queue->tail == queue->head) {
        return NULL;
    }
    else{
		handle = &(queue->data[queue->tail]);
		queue->tail = (queue->tail + 1);
		if(queue->tail >= KEY_CMD_RING_BUFFER_QUEUE_SIZE)
			queue->tail = 0;
		return handle;
    }
}

int key_cmd_ring_buffer_queue_write(key_cmd_ring_buffer_t *queue, key_cmd_ring_buffer_item_t* handle) {
    if (((queue->head + 1) % KEY_CMD_RING_BUFFER_QUEUE_SIZE) == queue->tail) {
        return -1;
    }

    queue->data[queue->head] = *handle;

    queue->head = (queue->head + 1);
    if(queue->head >= KEY_CMD_RING_BUFFER_QUEUE_SIZE)
        queue->head = 0;
    return 0;
}
