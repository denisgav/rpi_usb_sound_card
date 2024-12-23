#ifndef MACHINE_I2S__H
#define MACHINE_I2S__H

#include <stdlib.h>
#include <string.h>

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "ring_buf.h"

// Notes on this port's specific implementation of I2S:
// - the DMA IRQ handler is used to implement the asynchronous background operations, for non-blocking mode
// - the PIO is used to drive the I2S bus signals
// - all sample data transfers use non-blocking DMA
// - the DMA controller is configured with 2 DMA channels in chained mode

#define MAX_I2S_RP2 (2)

// The DMA buffer size was empirically determined.  It is a tradeoff between:
// 1. memory use (smaller buffer size desirable to reduce memory footprint)
// 2. interrupt frequency (larger buffer size desirable to reduce interrupt frequency)
#define SIZEOF_DMA_BUFFER_IN_BYTES (96*8*3) // Max frequency is 96000. in worst case. 1ms contains 96 samples. Each sample is 8 bytes. Need to hold 2 buffers of this size
#define I2S_NUM_DMA_CHANNELS (2)

#define SAMPLES_PER_FRAME (2)
#define PIO_INSTRUCTIONS_PER_BIT (2)


#define STATIC static
#define mp_hal_pin_obj_t uint

#ifndef m_new
    #define m_new(type, num) ((type *)(malloc(sizeof(type) * (num))))
#endif //m_new

#ifndef m_new_obj
    #define m_new_obj(type) (m_new(type, 1))
#endif //m_new_obj


typedef enum {
    RX,
    TX
} i2s_mode_t;

typedef enum {
    BLOCKING,
    NON_BLOCKING,
    UASYNCIO
} io_mode_t;

typedef enum {
    GP_INPUT = 0,
    GP_OUTPUT = 1
} gpio_dir_t;



// Buffer protocol

typedef struct _mp_buffer_info_t {
    void *buf;      // can be NULL if len == 0
    size_t len;     // in bytes
    int typecode;   // as per binary.h
} mp_buffer_info_t;

typedef struct _machine_i2s_obj_t {
    uint8_t i2s_id;
    mp_hal_pin_obj_t sck;
    mp_hal_pin_obj_t ws;
    mp_hal_pin_obj_t sd;
    i2s_mode_t mode;
    int8_t bits;
    int32_t rate;
    PIO pio;
    uint8_t sm;
    const pio_program_t *pio_program;
    uint prog_offset;
    int dma_channel[I2S_NUM_DMA_CHANNELS];
    uint8_t dma_buffer[SIZEOF_DMA_BUFFER_IN_BYTES];
    ring_buf_t ring_buffer;
    uint8_t *ring_buffer_storage;
    uint32_t sizeof_half_dma_buffer_in_bytes;
} machine_i2s_obj_t;

typedef struct  {
    int32_t left;
    int32_t right;
} i2s_32b_audio_sample;

typedef struct  {
    int16_t left;
    int16_t right;
} i2s_16b_audio_sample;


machine_i2s_obj_t* create_machine_i2s(uint8_t i2s_id,
              mp_hal_pin_obj_t sck, mp_hal_pin_obj_t ws, mp_hal_pin_obj_t sd,
              i2s_mode_t i2s_mode, int8_t i2s_bits, int32_t ring_buffer_len, int32_t i2s_rate);


int machine_i2s_read_stream(machine_i2s_obj_t *self, void *buf_in, size_t size);
int machine_i2s_write_stream(machine_i2s_obj_t *self, void *buf_in, size_t size);

//void update_pio_frequency(machine_i2s_obj_t *self, uint32_t sample_freq);

#endif //MACHINE_I2S__H