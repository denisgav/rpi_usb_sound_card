/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Reinhard Panhuber
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

/* plot_audio_samples.py requires following modules:
 * $ sudo apt install libportaudio
 * $ pip3 install sounddevice matplotlib
 *
 * Then run
 * $ python3 plot_audio_samples.py
 */

#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include "usb_microphone.h"

#include "pdm/pdm_microphone.h"
#include "volume_ctrl.h"

#include "board_defines.h"
#include "pdm_board_defines.h"
#include "microphone_settings.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+
#define SAMPLE_BUFFER_SIZE  (PDM_MIC_RATE_DEF/1000)


// Pointer to PDM handler
machine_pdm_obj_t* pdm0 = NULL;

// variables
int16_t sample_buffer_l[SAMPLE_BUFFER_SIZE];

typedef int16_t usb_audio_sample;

microphone_settings_t microphone_settings;

// Audio test data, 4 channels muxed together, buffer[0] for CH0, buffer[1] for CH1, buffer[2] for CH2, buffer[3] for CH3
usb_audio_sample usb_dummy_buffer[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX*CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE/1000];

//-------------------------
// callback functions
//-------------------------
void usb_microphone_mute_handler(int8_t bChannelNumber, int8_t mute_in);
void usb_microphone_volume_handler(int8_t bChannelNumber, int16_t volume_in);
void usb_microphone_current_sample_rate_handler(uint32_t current_sample_rate_in);
void usb_microphone_current_resolution_handler(uint8_t current_resolution_in);
void usb_microphone_current_status_set_handler(uint32_t blink_interval_ms_in);

void on_usb_microphone_tx_pre_load(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
void on_usb_microphone_tx_post_load(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting);
//-------------------------

void led_blinking_task(void);
void status_update_task(void);

void refresh_pdm_connections()
{
  microphone_settings.samples_in_i2s_frame_min = (microphone_settings.sample_rate)    /1000;
  microphone_settings.samples_in_i2s_frame_max = (microphone_settings.sample_rate+999)/1000;

  pdm0 = create_machine_pdm(0, PDM_MIC_0_DATA, PDM_MIC_0_CLK);
}

//---------------------------------------
//           LED and button
//---------------------------------------
void setup_led_and_button();
void button_mute_ISR(uint gpio, uint32_t events);
//---------------------------------------

usb_audio_sample pdm_to_usb_sample_convert(int16_t sample, uint32_t volume_db);

/*------------- MAIN -------------*/
int main(void)
{
  microphone_settings.sample_rate  = PDM_MIC_RATE_DEF;
  microphone_settings.resolution = CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_TX;
  microphone_settings.blink_interval_ms = BLINK_NOT_MOUNTED;
  microphone_settings.status_updated = false;
  microphone_settings.user_mute = false;

  setup_led_and_button();
  
  usb_microphone_set_mute_set_handler(usb_microphone_mute_handler);
  usb_microphone_set_volume_set_handler(usb_microphone_volume_handler);
  usb_microphone_set_current_sample_rate_set_handler(usb_microphone_current_sample_rate_handler);
  usb_microphone_set_current_resolution_set_handler(usb_microphone_current_resolution_handler);
  usb_microphone_set_current_status_set_handler(usb_microphone_current_status_set_handler);

  usb_microphone_set_tx_pre_load_handler(on_usb_microphone_tx_pre_load);
  usb_microphone_set_tx_post_load_handler(on_usb_microphone_tx_post_load);

  usb_microphone_init();

  refresh_pdm_connections();

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1); i++)
  {
    microphone_settings.volume[i] = DEFAULT_VOLUME;
    microphone_settings.mute[i] = 0;
    microphone_settings.volume_db[i] = vol_to_db_convert(microphone_settings.mute[i], microphone_settings.volume[i]);
  }

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX); i++) {
    microphone_settings.volume_mul_db[0] = microphone_settings.volume_db[0]
      * microphone_settings.volume_db[i+1];
  }

  while (1)
  {
    usb_microphone_task(); // tinyusb device task

    led_blinking_task();

    status_update_task();
  }
}


//---------------------------------------
//           LED and button
//---------------------------------------
void setup_led_and_button(){
  gpio_init(LED_RED_PIN);
  gpio_set_dir(LED_RED_PIN, GPIO_OUT);

  gpio_init(BTN_MUTE_PIN);
  gpio_set_dir(BTN_MUTE_PIN, GPIO_IN);

  gpio_set_irq_enabled_with_callback(BTN_MUTE_PIN, GPIO_IRQ_EDGE_RISE, 1, button_mute_ISR);
}
//---------------------------------------


//-------------------------
// callback functions
//-------------------------

void usb_microphone_mute_handler(int8_t bChannelNumber, int8_t mute_in)
{
  microphone_settings.mute[bChannelNumber] = mute_in;
  microphone_settings.volume_db[bChannelNumber] = vol_to_db_convert(microphone_settings.mute[bChannelNumber], microphone_settings.volume[bChannelNumber]);

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX); i++) {
    microphone_settings.volume_mul_db[0] = microphone_settings.volume_db[0]
      * microphone_settings.volume_db[i+1];
  }

  microphone_settings.status_updated = true;
}

void usb_microphone_volume_handler(int8_t bChannelNumber, int16_t volume_in)
{
  // If value in range -91 to 0, apply as is
  if((volume_in >= -91) && (volume_in <= 0))
    microphone_settings.volume[bChannelNumber] = volume_in;
   else { // Need to convert the value
     int16_t volume_tmp = volume_in >> ENC_NUM_OF_FP_BITS; // Value in range -128 to 127
     volume_tmp = volume_tmp - 127; // Value in range -255 to 0. Need to have -91 to 0
     volume_tmp = (volume_tmp*91)/255;
     microphone_settings.volume[bChannelNumber] = volume_tmp;    
  }
  microphone_settings.volume_db[bChannelNumber] = vol_to_db_convert(microphone_settings.mute[bChannelNumber], microphone_settings.volume[bChannelNumber]);

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX); i++) {
    microphone_settings.volume_mul_db[0] = microphone_settings.volume_db[0]
      * microphone_settings.volume_db[i+1];
  }

  microphone_settings.status_updated = true;
}

void usb_microphone_current_sample_rate_handler(uint32_t current_sample_rate_in)
{
  microphone_settings.sample_rate = current_sample_rate_in;
  //refresh_pdm_connections();
  microphone_settings.status_updated = true;
}

void usb_microphone_current_resolution_handler(uint8_t current_resolution_in)
{
  microphone_settings.resolution = current_resolution_in;
  //refresh_pdm_connections();
  microphone_settings.status_updated = true;
}

void usb_microphone_current_status_set_handler(uint32_t blink_interval_ms_in)
{
  microphone_settings.blink_interval_ms = blink_interval_ms_in;
  microphone_settings.status_updated = true;
}

void on_usb_microphone_tx_pre_load(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  tud_audio_write(usb_dummy_buffer, PDM_MIC_RATE_DEF/1000 * CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX);
}

void on_usb_microphone_tx_post_load(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  uint32_t volume_db_left = microphone_settings.volume_mul_db[0];

  // reading left microphone
  int samples_read = machine_pdm_read_stream(pdm0, sample_buffer_l, SAMPLE_BUFFER_SIZE);
  if(samples_read == SAMPLE_BUFFER_SIZE){
    for(uint32_t i = 0; i < SAMPLE_BUFFER_SIZE; i++){
      usb_dummy_buffer[i] = pdm_to_usb_sample_convert(sample_buffer_l[i], volume_db_left);
    }
  } else {
    for(uint32_t i = 0; i < SAMPLE_BUFFER_SIZE; i++){
      usb_dummy_buffer[i] = 0;
    }
  }

    static bool dma_stat_led = 0;
    gpio_put(LED_RED_PIN, dma_stat_led);
    dma_stat_led = 1-dma_stat_led;
}

usb_audio_sample pdm_to_usb_sample_convert(int16_t sample, uint32_t volume_db)
{
  #ifdef APPLY_VOLUME_FEATURE
    if(microphone_settings.user_mute) 
      return 0;
    else
    {
      int64_t sample_tmp = (int64_t)(sample) * (int64_t)volume_db;
      sample_tmp = sample_tmp>>(15+15);
      return (int16_t)sample_tmp;
      //return (int16_t)sample;
    }
  #else //APPLY_VOLUME_FEATURE
    if(microphone_settings.user_mute) 
      return 0;
    else
    {
      return (int16_t)(sample);
    }
  #endif //APPLY_VOLUME_FEATURE
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  uint32_t cur_time_ms = board_millis();

  // Blink every interval ms
  if (cur_time_ms - start_ms < microphone_settings.blink_interval_ms) return;
  start_ms += microphone_settings.blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state;
}

//--------------------------------------------------------------------+
// STATUS UPDATE TASK
//--------------------------------------------------------------------+
void status_update_task(void){
  static uint32_t prev_status_update__ms = 0;

  uint32_t cur_time_ms = board_millis();

  // Update status 2 times per second
  if (cur_time_ms - prev_status_update__ms < 500) 
    return;

  prev_status_update__ms = cur_time_ms;

  gpio_put(LED_RED_PIN, microphone_settings.user_mute);


  if(microphone_settings.status_updated == true){
    microphone_settings.status_updated = false;
    //display_ssd1306_info();
  }
}

void button_mute_ISR(uint gpio, uint32_t events){
  microphone_settings.user_mute = !microphone_settings.user_mute;
}