/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/time.h"
#include "ws2812.pio.h"

#include "ws2812/ws2812.h"

#define IS_RGBW true
#define NUM_PIXELS 1
#define WS2812_PIN 23

static PIO pio;
static int sm;

static inline void put_pixel(uint32_t pixel_grb) {
    // static void pio_sm_put_blocking (PIO pio, uint sm, uint32_t data)
    // Write a word of data to a state machine’s TX FIFO, blocking if the FIFO is full.
    //pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);

    pio_sm_put(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}

void pattern_streaming(uint t){
    uint8_t cntr = (t&0x1ff);
    uint8_t shade = (cntr >= 0x100 ) ? (0x1ff - cntr) : cntr;
    put_pixel(urgb_u32(shade, shade, shade));
}

void pattern_not_mounted(uint t){
    put_pixel(urgb_u32(0xff, 0x0, 0x0));
}

void pattern_mounted(uint t){
    put_pixel(urgb_u32(0x0, 0xff, 0x0));

}

void pattern_suspended(uint t){
    uint8_t cntr = (t&0x1ff);
    uint8_t shade = (cntr >= 0x100 ) ? (0x1ff - cntr) : cntr;
    put_pixel(urgb_u32(0x0, 0x0, shade));
}

static const PIO pio_instances[NUM_PIOS] = {pio0, pio1};

int ws2812_init(){

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

        if (pio_can_add_program(candidate_pio,  &ws2812_program)) {
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

    pio = candidate_pio;
    sm = pio_claim_unused_sm(pio, false);

    // todo get free sm
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    return 0;
}

void ws2812_task(ws2812_animation_e animation){
    static int t = 0;

    static uint32_t prev_status_update__ms = 0;

    uint32_t cur_time_ms = to_ms_since_boot(get_absolute_time());

    // Update status 2 times per second
    if (cur_time_ms - prev_status_update__ms < (1000/25))
        return;

    prev_status_update__ms = cur_time_ms;

    switch(animation){
        case WS2812_ANIMATION_STREAMING: { pattern_streaming(t); break; }
        case WS2812_ANIMATION_NOT_MOUNTED: { pattern_not_mounted(t); break; }
        case WS2812_ANIMATION_MOUNTED: { pattern_mounted(t); break; }
        case WS2812_ANIMATION_SUSPENDED: { pattern_suspended(t); break; }
    }

    t += 1;
}
