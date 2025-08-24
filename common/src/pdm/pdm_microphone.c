/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "pdm_microphone.pio.h"

#include "OpenPDMFilter.h"
#include "pdm/pdm_microphone.h"

#include "board_defines.h"

STATIC machine_pdm_obj_t* machine_pdm_obj[MAX_PDM_RP2] = {NULL, NULL};

STATIC const PIO pio_instances[NUM_PIOS] = {pio0, pio1};

STATIC void dma_irq0_handler(void);
STATIC void dma_irq1_handler(void);
STATIC void machine_pdm_deinit(machine_pdm_obj_t *self);

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

// function is used in IRQ context
STATIC void empty_dma(machine_pdm_obj_t *self, uint8_t *dma_buffer_p) {
    uint32_t available_space = ringbuf_available_space(&self->ring_buffer);

    if(available_space >= self->sizeof_half_dma_buffer_in_bytes){
        // when space exists, copy samples into ring buffer
        if (ringbuf_available_space(&self->ring_buffer) >= self->sizeof_half_dma_buffer_in_bytes) {
            RING_BUF_ITEM_TYPE* data = (RING_BUF_ITEM_TYPE*)(dma_buffer_p);
            uint32_t num_of_items = (self->sizeof_half_dma_buffer_in_bytes/ RING_BUF_ITEM_SIZE_IN_BYTES);
            for (uint32_t i = 0; i < num_of_items; i++) {
                if(ringbuf_push(&self->ring_buffer, data[i]) == false){
                    return;
                }
            }
        }
    } else {
        return;
    }
}

STATIC void irq_configure(machine_pdm_obj_t *self) {
    if (self->pdm_id == 0) {
        irq_add_shared_handler(DMA_IRQ_0, dma_irq0_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
        irq_set_enabled(DMA_IRQ_0, true);
    } else {
        irq_add_shared_handler(DMA_IRQ_1, dma_irq1_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
        irq_set_enabled(DMA_IRQ_1, true);
    }
}

STATIC void irq_deinit(machine_pdm_obj_t *self) {
    if (self->pdm_id == 0) {
        irq_remove_handler(DMA_IRQ_0, dma_irq0_handler);
    } else {
        irq_remove_handler(DMA_IRQ_1, dma_irq1_handler);
    }
}

STATIC void gpio_init_pdm(PIO pio, uint8_t sm, mp_hal_pin_obj_t pin_num, uint8_t pin_val, gpio_dir_t pin_dir) {
    uint32_t pinmask = 1 << pin_num;
    pio_sm_set_pins_with_mask(pio, sm, pin_val << pin_num, pinmask);
    pio_sm_set_pindirs_with_mask(pio, sm, pin_dir << pin_num, pinmask);
    pio_gpio_init(pio, pin_num);
}

STATIC void gpio_configure(machine_pdm_obj_t *self) {
    gpio_init_pdm(self->pio, self->sm, self->gpio_clk, 0, GP_OUTPUT);
    gpio_init_pdm(self->pio, self->sm, self->gpio_data, 0, GP_INPUT);
}

STATIC int pio_configure(machine_pdm_obj_t *self) {
    self->pio_program = &pdm_microphone_data_program;

    // find a PIO with a free state machine and adequate program space
    PIO candidate_pio;
    bool is_free_sm;
    bool can_add_program;
    for (uint8_t p = 0; p < NUM_PIOS; p++) {
        candidate_pio = pio_instances[p];
        is_free_sm = false;
        can_add_program = false;

        for (uint8_t sm = 0; sm < NUM_PIO_STATE_MACHINES; sm++) {
            if (!pio_sm_is_claimed(candidate_pio, sm)) {
                is_free_sm = true;
                break;
            }
        }

        if (pio_can_add_program(candidate_pio,  self->pio_program)) {
            can_add_program = true;
        }

        if (is_free_sm && can_add_program) {
            break;
        }
    }

    if (!is_free_sm) {
        //mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("no free state machines"));
        return -1;
    }

    if (!can_add_program) {
        //mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("not enough PIO program space"));
        return -2;
    }

    self->pio = candidate_pio;
    self->sm = pio_claim_unused_sm(self->pio, false);
    self->prog_offset = pio_add_program(self->pio, self->pio_program);
    

    pio_sm_config config = pio_get_default_sm_config();

    float clk_div = clock_get_hz(clk_sys) / (PDM_SIZEOF_DMA_BUFFER_IN_SAMPLES*1000 * PDM_DECIMATION * 4.0);
    sm_config_set_clkdiv(&config, clk_div);

    sm_config_set_in_pins(&config, self->gpio_data);
    sm_config_set_sideset(&config, 1, false, false);
    sm_config_set_sideset_pins(&config, self->gpio_clk);
    sm_config_set_in_shift(&config, false, false, 8);
    sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_RX);  // double RX FIFO size

    //sm_config_set_wrap(&config, self->prog_offset, self->prog_offset + self->pio_program->length - 1);
    pio_sm_set_config(self->pio, self->sm, &config);

    pio_sm_init(self->pio, self->sm, self->prog_offset, &config);

    // float clk_div = clock_get_hz(clk_sys) / (16000 * PDM_DECIMATION * 4.0);
    // pdm_microphone_data_init(self->pio, self->sm, self->prog_offset,
    //     clk_div, self->gpio_data, self->gpio_clk);

    // pio_sm_set_enabled(
    //     self->pio,
    //     self->sm,
    //     true
    // );

    return 0;
}

STATIC void pio_deinit(machine_pdm_obj_t *self) {
    if (self->pio) {
        pio_sm_set_enabled(self->pio, self->sm, false);
        pio_sm_unclaim(self->pio, self->sm);
        pio_remove_program(self->pio, self->pio_program, self->prog_offset);
    }
}

// determine which DMA channel is associated to this IRQ
STATIC uint dma_map_irq_to_channel(uint irq_index) {
    for (uint ch = 0; ch < NUM_DMA_CHANNELS; ch++) {
        if ((dma_irqn_get_channel_status(irq_index, ch))) {
            return ch;
        }
    }
    // This should never happen
    return -1;
}

// note:  first DMA channel is mapped to the top half of buffer, second is mapped to the bottom half
STATIC uint8_t *dma_get_buffer(machine_pdm_obj_t *self, uint channel) {
    for (uint8_t ch = 0; ch < PDM_NUM_DMA_CHANNELS; ch++) {
        if (self->dma_channel[ch] == channel) {
            return self->dma_buffer + (self->sizeof_half_dma_buffer_in_bytes * ch);
        }
    }
    // This should never happen
    return NULL;
}

STATIC int dma_configure(machine_pdm_obj_t *self) {
    uint8_t num_free_dma_channels = 0;
    for (uint8_t ch = 0; ch < PDM_NUM_DMA_CHANNELS; ch++) {
        if (!dma_channel_is_claimed(ch)) {
            num_free_dma_channels++;
        }
    }
    if (num_free_dma_channels < PDM_NUM_DMA_CHANNELS) {
        //mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("cannot claim 2 DMA channels"));
        return -1;
    }

    for (uint8_t ch = 0; ch < PDM_NUM_DMA_CHANNELS; ch++) {
        self->dma_channel[ch] = dma_claim_unused_channel(false);
    }

    // The DMA channels are chained together.  The first DMA channel is used to access
    // the top half of the DMA buffer.  The second DMA channel accesses the bottom half of the DMA buffer.
    // With chaining, when one DMA channel has completed a data transfer, the other
    // DMA channel automatically starts a new data transfer.
    enum dma_channel_transfer_size dma_size = DMA_SIZE_8;
    for (uint8_t ch = 0; ch < PDM_NUM_DMA_CHANNELS; ch++) {
        dma_channel_config dma_config = dma_channel_get_default_config(self->dma_channel[ch]);
        channel_config_set_transfer_data_size(&dma_config, dma_size);
        channel_config_set_chain_to(&dma_config, self->dma_channel[(ch + 1) % PDM_NUM_DMA_CHANNELS]);

        uint8_t *dma_buffer = self->dma_buffer + (self->sizeof_half_dma_buffer_in_bytes * ch);
        channel_config_set_dreq(&dma_config, pio_get_dreq(self->pio, self->sm, false));
        channel_config_set_read_increment(&dma_config, false);
        channel_config_set_write_increment(&dma_config, true);
        dma_channel_configure(self->dma_channel[ch],
            &dma_config,
            dma_buffer,                                             // dest = DMA buffer
            (void *)&self->pio->rxf[self->sm],                      // src = PIO RX FIFO
            self->sizeof_half_dma_buffer_in_bytes,
            false);
    }

    for (uint8_t ch = 0; ch < PDM_NUM_DMA_CHANNELS; ch++) {
        dma_irqn_acknowledge_channel(self->pdm_id, self->dma_channel[ch]);  // clear pending.  e.g. from SPI
        dma_irqn_set_channel_enabled(self->pdm_id, self->dma_channel[ch], true);
    }
    return 0;
}

STATIC void dma_deinit(machine_pdm_obj_t *self) {
    for (uint8_t ch = 0; ch < PDM_NUM_DMA_CHANNELS; ch++) {
        int channel = self->dma_channel[ch];

        // unchain the channel to prevent triggering a transfer in the chained-to channel
        dma_channel_config dma_config = dma_get_channel_config(channel);
        channel_config_set_chain_to(&dma_config, channel);
        dma_channel_set_config(channel, &dma_config, false);

        dma_irqn_set_channel_enabled(self->pdm_id, channel, false);
        dma_channel_abort(channel);  // in case a transfer is in flight
        dma_channel_unclaim(channel);
    }
}

STATIC void dma_irq_handler(uint8_t irq_index) {
    int dma_channel = dma_map_irq_to_channel(irq_index);
    if (dma_channel == -1) {
        // This should never happen
        return;
    }

    machine_pdm_obj_t *self = machine_pdm_obj[irq_index];
    if (self == NULL) {
        // This should never happen
        return;
    }

    uint8_t *dma_buffer = dma_get_buffer(self, dma_channel);
    if (dma_buffer == NULL) {
        // This should never happen
        return;
    }
    
    empty_dma(self, dma_buffer);
    dma_irqn_acknowledge_channel(irq_index, dma_channel);
    dma_channel_set_write_addr(dma_channel, dma_buffer, false);
    
}

STATIC void dma_irq0_handler(void) {
    dma_irq_handler(0);
}

STATIC void dma_irq1_handler(void) {
    dma_irq_handler(1);
}

STATIC int machine_pdm_init_helper(machine_pdm_obj_t *self,
              mp_hal_pin_obj_t gpio_data, mp_hal_pin_obj_t gpio_clk) {
    //
    // ---- Check validity of arguments ----
    //

    // does gpio_clk pin follow gpio_data pin?
    // note:  gpio_data and WS are implemented as PIO sideset pins.  Sideset pins must be sequential.
    if (gpio_clk != (gpio_data + 1)) {
        //mp_raise_ValueError(MP_ERROR_TEXT("invalid gpio_clk (must be gpio_data+1)"));
        return -1;
    }

    // is Ibuf valid?
    uint32_t ring_buffer_len = PDM_SIZEOF_DMA_BUFFER_IN_BYTES;
    self->ring_buffer_storage = m_new(uint8_t, ring_buffer_len);
    ringbuf_init(&self->ring_buffer, self->ring_buffer_storage, ring_buffer_len);

    self->gpio_data = gpio_data;
    self->gpio_clk = gpio_clk;
    self->rate = PDM_SIZEOF_DMA_BUFFER_IN_SAMPLES*1000;
    self->raw_buffer_size = PDM_SIZEOF_DMA_READ_BUFFER_IN_BYTES;

    memset(self->dma_buffer, 0, PDM_SIZEOF_DMA_BUFFER_IN_BYTES);

    //self->sizeof_half_dma_buffer_in_bytes = ((self->rate+999)/1000) * ((i2s_bits == 32) ? 8 : 4);
    self->sizeof_half_dma_buffer_in_bytes = self->raw_buffer_size / 2;

    self->filter.Fs = self->rate;
    self->filter.LP_HZ = self->rate / 2;
    self->filter.HP_HZ = 10;
    self->filter.In_MicChannels = 1;
    self->filter.Out_MicChannels = 1;
    self->filter.Decimation = PDM_DECIMATION;
    self->filter.MaxVolume = 64;
    self->filter.Gain = 16;

    self->filter_volume = self->filter.MaxVolume;

    Open_PDM_Filter_Init(&self->filter);

    irq_configure(self);
    int err = pio_configure(self);
    if (err != 0) {
        return err;
    }
    gpio_configure(self);
    err = dma_configure(self);
    if (err != 0) {
        return err;
    }

    pio_sm_set_enabled(self->pio, self->sm, true);
    dma_channel_start(self->dma_channel[0]);

    return 0;
}

STATIC machine_pdm_obj_t* machine_pdm_make_new(uint8_t pdm_id,
              mp_hal_pin_obj_t gpio_data, mp_hal_pin_obj_t gpio_clk) {
    if (pdm_id >= MAX_PDM_RP2) {
        return NULL;
    }

    machine_pdm_obj_t *self;
    // Deinit a machine if it already created
    if (machine_pdm_obj[pdm_id] != NULL) { 
        self = machine_pdm_obj[pdm_id];
        machine_pdm_deinit(self);
    }

    self = m_new_obj(machine_pdm_obj_t);
    machine_pdm_obj[pdm_id] = self;
    self->pdm_id = pdm_id;

    if (machine_pdm_init_helper(self, gpio_data, gpio_clk) != 0) {
        return NULL;
    }
    return self;
}

STATIC void machine_pdm_deinit(machine_pdm_obj_t *self){
    // use self->pio as in indication that I2S object has already been de-initialized
    if (self != NULL) {
        pio_deinit(self);
        dma_deinit(self);
        irq_deinit(self);

        self->pio = NULL;  // flag object as de-initialized
        machine_pdm_obj[self->pdm_id] == NULL;

        free(self->ring_buffer_storage);
        free(self);
    }
}

int machine_pdm_read_stream(machine_pdm_obj_t *self, int16_t* buffer, size_t samples) {
    int filter_stride = (self->filter.Fs / 1000);
    samples = (samples / filter_stride) * filter_stride;

    if (samples > PDM_SIZEOF_DMA_BUFFER_IN_SAMPLES) {
        samples = PDM_SIZEOF_DMA_BUFFER_IN_SAMPLES;
    }

    memset(self->read_raw_buffer, 0x0, self->raw_buffer_size);

    uint32_t available_data_bytes = ringbuf_available_data(&self->ring_buffer);
    if(available_data_bytes < self->raw_buffer_size)
        return 0;

    // Read data from ring buffer
    RING_BUF_ITEM_TYPE* data = (RING_BUF_ITEM_TYPE*)(self->read_raw_buffer);
    uint32_t num_bytes_needed_from_ringbuf = self->raw_buffer_size;
    uint32_t num_of_items = (num_bytes_needed_from_ringbuf / RING_BUF_ITEM_SIZE_IN_BYTES);

    for(uint32_t a_index = 0; a_index < num_of_items; a_index++){
        if(ringbuf_pop(&self->ring_buffer, &(data[a_index])) == false) {
            break;
        }
    }

    uint8_t* in = self->read_raw_buffer;
    int16_t* out = buffer;

    for (int i = 0; i < samples; i += filter_stride) {
#if PDM_DECIMATION == 64
        Open_PDM_Filter_64(in, out, self->filter_volume, &self->filter);
#elif PDM_DECIMATION == 128
        Open_PDM_Filter_128(in, out, self->filter_volume, &self->filter);
#else
        #error "Unsupported PDM_DECIMATION value!"
#endif

        in += filter_stride * (PDM_DECIMATION / 8);
        out += filter_stride;
    }
    return samples;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


machine_pdm_obj_t* create_machine_pdm(uint8_t pdm_id,
              mp_hal_pin_obj_t gpio_data, mp_hal_pin_obj_t gpio_clk)
{
    return machine_pdm_make_new(pdm_id, gpio_data, gpio_clk);
}
