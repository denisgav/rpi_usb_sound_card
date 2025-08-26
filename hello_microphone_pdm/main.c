/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * This examples captures data from a PDM microphone using a sample
 * rate of 8 kHz and prints the sample values over the USB serial
 * connection.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pdm/pdm_microphone.h"
#include "tusb.h"

#define AUDIO_SAMPLE_RATE 16000
#define SAMPLE_BUFFER_SIZE  (AUDIO_SAMPLE_RATE/1000)

// cat /dev/ttyACM0 | xxd -r -p | aplay -r16000 -c1 -fS16_BE

// Pointer to PDM handler
machine_pdm_obj_t* pdm0 = NULL;

// variables
int16_t sample_buffer[SAMPLE_BUFFER_SIZE];

int main( void )
{
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();
    while (!tud_cdc_connected()) {
        tight_loop_contents();
    }

    printf("hello PDM microphone\n");

    // initialize the PDM microphone
    pdm0 = create_machine_pdm(0, 18, 19);

    if (pdm0 == NULL) {
        printf("PDM microphone initialization failed!\n");
        while (1) { tight_loop_contents(); }
    }

    while (1) {
        int samples_read = machine_pdm_read_stream(pdm0, sample_buffer, SAMPLE_BUFFER_SIZE);
        if(samples_read == SAMPLE_BUFFER_SIZE){
            for(uint32_t i = 0; i < SAMPLE_BUFFER_SIZE; i++){
                printf("Sample Buffer [%0d] -> %d\n", i, sample_buffer[i]);
                //printf("%.4x\n", sample_buffer[i]);
            }
        } else {
            tight_loop_contents();
        }
    }

    return 0;
}
