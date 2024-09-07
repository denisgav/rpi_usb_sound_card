#ifndef KEY_CMD_RING_BUFFER__H
#define KEY_CMD_RING_BUFFER__H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef KEY_CMD_RING_BUFFER_QUEUE_SIZE
	#define KEY_CMD_RING_BUFFER_QUEUE_SIZE 16
#endif //KEY_CMD_RING_BUFFER_QUEUE_SIZE

typedef uint8_t media_key_report_t;

// Key report
typedef struct {
	media_key_report_t media_key_report;
} key_cmd_ring_buffer_item_t;

// Ring buffer:
typedef struct {
    size_t head;
    size_t tail;
    key_cmd_ring_buffer_item_t data[KEY_CMD_RING_BUFFER_QUEUE_SIZE];
} key_cmd_ring_buffer_t;

int key_cmd_ring_buffer_has_item(key_cmd_ring_buffer_t *queue);
key_cmd_ring_buffer_item_t* key_cmd_ring_buffer_queue_read(key_cmd_ring_buffer_t *queue);
int key_cmd_ring_buffer_queue_write(key_cmd_ring_buffer_t *queue, key_cmd_ring_buffer_item_t* handle);

#endif //KEY_CMD_RING_BUFFER__H
