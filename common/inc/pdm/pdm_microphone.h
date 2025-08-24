#ifndef _PICO_PDM_MICROPHONE_H_
#define _PICO_PDM_MICROPHONE_H_

#include <stdlib.h>
#include <string.h>

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "ring_buf.h"

#include "OpenPDMFilter.h"

// Notes on this port's specific implementation of PDM:
// - the DMA IRQ handler is used to implement the asynchronous background operations, for non-blocking mode
// - the PIO is used to drive the PDM bus signals
// - all sample data transfers use non-blocking DMA
// - the DMA controller is configured with 2 DMA channels in chained mode

#define MAX_PDM_RP2 (2)

// The DMA buffer size was empirically determined.  It is a tradeoff between:
// 1. memory use (smaller buffer size desirable to reduce memory footprint)
// 2. interrupt frequency (larger buffer size desirable to reduce interrupt frequency)
#define PDM_DECIMATION       64
#define PDM_SIZEOF_DMA_BUFFER_IN_SAMPLES 16
#define PDM_SIZEOF_DMA_BUFFER_IN_BYTES (PDM_SIZEOF_DMA_BUFFER_IN_SAMPLES * (PDM_DECIMATION/8) * 2 * 2) // Max frequency is 16000. in worst case. 1ms contains 16 samples. Each sample is 2 bytes. Need to hold 2 buffers of this size
#define PDM_SIZEOF_DMA_READ_BUFFER_IN_BYTES (PDM_SIZEOF_DMA_BUFFER_IN_SAMPLES * (PDM_DECIMATION/8))
#define PDM_NUM_DMA_CHANNELS (2)

#define STATIC static
#define mp_hal_pin_obj_t uint

#ifndef m_new
    #define m_new(type, num) ((type *)(malloc(sizeof(type) * (num))))
#endif //m_new

#ifndef m_new_obj
    #define m_new_obj(type) (m_new(type, 1))
#endif //m_new_obj

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

typedef struct _machine_pdm_obj_t {
    uint8_t pdm_id;
    mp_hal_pin_obj_t gpio_data;
    mp_hal_pin_obj_t gpio_clk;
    int32_t rate;
    PIO pio;
    uint8_t sm;
    const pio_program_t *pio_program;
    uint prog_offset;
    int dma_channel[PDM_NUM_DMA_CHANNELS];
    uint8_t dma_buffer[PDM_NUM_DMA_CHANNELS];
    ring_buf_t ring_buffer;
    uint8_t *ring_buffer_storage;
    uint32_t sizeof_half_dma_buffer_in_bytes;

    uint32_t raw_buffer_size;
    uint8_t read_raw_buffer[PDM_SIZEOF_DMA_READ_BUFFER_IN_BYTES];

    TPDMFilter_InitStruct filter;
    uint16_t filter_volume;
} machine_pdm_obj_t;

machine_pdm_obj_t* create_machine_pdm(uint8_t pdm_id,
              mp_hal_pin_obj_t gpio_data, mp_hal_pin_obj_t gpio_clk);


int machine_pdm_read_stream(machine_pdm_obj_t *self, int16_t* buffer, size_t samples);

#endif //_PICO_PDM_MICROPHONE_H_
