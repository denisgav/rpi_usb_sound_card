#ifndef _USB_MICROPHONE_H_
#define _USB_MICROPHONE_H_

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "bsp/board_api.h"
#include "tusb.h"
//#include "usb_descriptors.h"

/* Blink pattern
 * - 25 ms   : streaming data
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum
{
  BLINK_STREAMING = 25,
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

#define AUDIO_SAMPLE_RATE   CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE

void usb_microphone_init();
void usb_microphone_task();

// Callback functions:
typedef void (*usb_microphone_mute_set_cb_t)(int8_t bChannelNumber, int8_t mute);
typedef void (*usb_microphone_volume_set_cb_t)(int8_t bChannelNumber, int16_t volume);

typedef void (*usb_microphone_current_sample_rate_set_cb_t)(uint32_t current_sample_rate);
typedef void (*usb_microphone_current_resolution_set_cb_t)(uint8_t current_resolution);

typedef void (*usb_microphone_current_status_set_cb_t)(uint32_t blink_interval_ms);

typedef void (*usb_microphone_tx_pre_load_cb_t)(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
typedef void (*usb_microphone_tx_post_load_cb_t)(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);

void usb_microphone_set_mute_set_handler(usb_microphone_mute_set_cb_t handler);
void usb_microphone_set_volume_set_handler(usb_microphone_volume_set_cb_t handler);

void usb_microphone_set_current_sample_rate_set_handler(usb_microphone_current_sample_rate_set_cb_t handler);
void usb_microphone_set_current_resolution_set_handler(usb_microphone_current_resolution_set_cb_t handler);

void usb_microphone_set_current_status_set_handler(usb_microphone_current_status_set_cb_t handler);

void usb_microphone_set_tx_pre_load_handler(usb_microphone_tx_pre_load_cb_t handler);
void usb_microphone_set_tx_post_load_handler(usb_microphone_tx_post_load_cb_t handler);

#endif
