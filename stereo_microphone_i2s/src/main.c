/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Jerzy Kasenberg
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

#include <stdio.h>
#include <string.h>

#include <pico/stdlib.h>

#include "usb_microphone.h"

#include "main.h"
#include "i2s/machine_i2s.h"
#include "volume_ctrl.h"

#include "ssd1306/ssd1306.h"

#include "ws2812/ws2812.h"


// Pointer to I2S handler
machine_i2s_obj_t* i2s0 = NULL;

microphone_settings_t microphone_settings;


// Audio test data, 2 channels muxed together, buffer[0] for CH0, buffer[1] for CH1
usb_audio_4b_sample i2s_24b_dummy_buffer[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX*CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE/1000];
usb_audio_2b_sample i2s_16b_dummy_buffer[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX*CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE/1000];

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

void refresh_i2s_connections()
{
  microphone_settings.samples_in_i2s_frame_min = (microphone_settings.sample_rate)    /1000;
  microphone_settings.samples_in_i2s_frame_max = (microphone_settings.sample_rate+999)/1000;

  i2s0 = create_machine_i2s(0, I2S_MIC_SCK, I2S_MIC_WS, I2S_MIC_SD, RX, I2S_MIC_BPS, /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, I2S_MIC_RATE_DEF);
  
  // update_pio_frequency(speaker_i2s0, microphone_settings.usb_sample_rate);
}


//---------------------------------------
//           SSD1306
//---------------------------------------
ssd1306_t disp;
void setup_ssd1306();
void display_ssd1306_info();
//---------------------------------------

//---------------------------------------
//           LED and button
//---------------------------------------
void setup_led_and_button();
void button_mute_ISR(uint gpio, uint32_t events);
//---------------------------------------

int32_t i2s_to_usb_32b_sample_convert(int32_t sample, uint16_t volume_db);
int16_t i2s_to_usb_16b_sample_convert(int16_t sample, uint16_t volume_db);

/*------------- MAIN -------------*/
int main(void)
{
  microphone_settings.sample_rate  = I2S_MIC_RATE_DEF;
  microphone_settings.resolution = CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX;
  microphone_settings.blink_interval_ms = BLINK_NOT_MOUNTED;
  microphone_settings.status_updated = false;
  microphone_settings.user_mute = false;

  setup_led_and_button();
  setup_ssd1306();
  ws2812_init();

  usb_microphone_set_mute_set_handler(usb_microphone_mute_handler);
  usb_microphone_set_volume_set_handler(usb_microphone_volume_handler);
  usb_microphone_set_current_sample_rate_set_handler(usb_microphone_current_sample_rate_handler);
  usb_microphone_set_current_resolution_set_handler(usb_microphone_current_resolution_handler);
  usb_microphone_set_current_status_set_handler(usb_microphone_current_status_set_handler);

  usb_microphone_set_tx_pre_load_handler(on_usb_microphone_tx_pre_load);
  usb_microphone_set_tx_post_load_handler(on_usb_microphone_tx_post_load);

  usb_microphone_init();
  refresh_i2s_connections();

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1); i++)
  {
    microphone_settings.volume[i] = DEFAULT_VOLUME;
    microphone_settings.mute[i] = 0;
    microphone_settings.volume_db[i] = vol_to_db_convert(microphone_settings.mute[i], microphone_settings.volume[i]);
  }

  while (1)
  {
    usb_microphone_task(); // tinyusb device task

    led_blinking_task();

    status_update_task();

    ws2812_task(microphone_settings.blink_interval_ms);
  }
}

//-------------------------
// SSD1306 functions
//-------------------------
void setup_ssd1306(){
  i2c_init(I2C_SSD1306_INST, 400000);
  gpio_set_function(I2C_SSD1306_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SSD1306_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SSD1306_SDA);
  gpio_pull_up(I2C_SSD1306_SCL);

  disp.external_vcc=false;
  ssd1306_init(&disp, I2C_SSD1306_WIDTH, I2C_SSD1306_HEIGHT, I2C_SSD1306_ADDR, I2C_SSD1306_INST);
  ssd1306_clear(&disp);

  ssd1306_draw_string(&disp, 4, 0, 1, "Raspberry pi pico");
  ssd1306_draw_string(&disp, 4, 16, 1, "USB UAC2 microphone");
  ssd1306_show(&disp);
}

//-------------------------

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
  microphone_settings.status_updated = true;
}

void usb_microphone_current_sample_rate_handler(uint32_t current_sample_rate_in)
{
  microphone_settings.sample_rate = current_sample_rate_in;
  refresh_i2s_connections();
  microphone_settings.status_updated = true;
}

void usb_microphone_current_resolution_handler(uint8_t current_resolution_in)
{
  microphone_settings.resolution = current_resolution_in;
  refresh_i2s_connections();
  microphone_settings.status_updated = true;
}

void usb_microphone_current_status_set_handler(uint32_t blink_interval_ms_in)
{
  microphone_settings.blink_interval_ms = blink_interval_ms_in;
  microphone_settings.status_updated = true;
}

void on_usb_microphone_tx_pre_load(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  if(microphone_settings.resolution == 24){
    uint32_t buffer_size = microphone_settings.samples_in_i2s_frame_min * CFG_TUD_AUDIO_FUNC_1_FORMAT_2_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
    tud_audio_write(i2s_24b_dummy_buffer, buffer_size);
  } else {
    uint32_t buffer_size = microphone_settings.samples_in_i2s_frame_min * CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX * CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
    tud_audio_write(i2s_16b_dummy_buffer, buffer_size);
  }
}

void on_usb_microphone_tx_post_load(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
{
  if(i2s0) {
    i2s_32b_audio_sample buffer[USB_MIC_SAMPLE_BUFFER_SIZE];

    // Read data from microphone
    uint32_t buffer_size_read = microphone_settings.samples_in_i2s_frame_min * (4 * 2);
    int num_bytes_read = machine_i2s_read_stream(i2s0, (void*)&buffer[0], buffer_size_read);

    if(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX == 2){
      uint16_t volume_db_master = microphone_settings.volume_db[0];
      uint16_t volume_db_left = microphone_settings.volume_db[1];
      uint16_t volume_db_right = microphone_settings.volume_db[2];
      {
        int num_of_frames_read = num_bytes_read/(4 * 2);
        for(uint32_t i = 0; i < num_of_frames_read; i++){
          if(microphone_settings.resolution == 24){
            int32_t left_24b = (int32_t)buffer[i].left << FORMAT_24B_TO_24B_SHIFT_VAL; // Magic number
            int32_t right_24b = (int32_t)buffer[i].right << FORMAT_24B_TO_24B_SHIFT_VAL; // Magic number
            left_24b = i2s_to_usb_32b_sample_convert(left_24b, volume_db_left);
            left_24b = i2s_to_usb_32b_sample_convert(left_24b, volume_db_master);
            right_24b = i2s_to_usb_32b_sample_convert(right_24b, volume_db_right);
            right_24b = i2s_to_usb_32b_sample_convert(right_24b, volume_db_master);

            i2s_24b_dummy_buffer[i*2] = left_24b; // TODO: check this value
            i2s_24b_dummy_buffer[i*2+1] = right_24b; // TODO: check this value
          }
          else {
            int32_t left_16b = (int32_t)buffer[i].left >> FORMAT_24B_TO_16B_SHIFT_VAL; // Magic number
            int32_t right_16b = (int32_t)buffer[i].right >> FORMAT_24B_TO_16B_SHIFT_VAL; // Magic number
            left_16b = i2s_to_usb_16b_sample_convert(left_16b, volume_db_left);
            left_16b = i2s_to_usb_16b_sample_convert(left_16b, volume_db_master);
            right_16b = i2s_to_usb_16b_sample_convert(right_16b, volume_db_right);
            right_16b = i2s_to_usb_16b_sample_convert(right_16b, volume_db_master);

            i2s_16b_dummy_buffer[i*2]   = left_16b; // TODO: check this value
            i2s_16b_dummy_buffer[i*2+1] = right_16b; // TODO: check this value
          }
        }
      }
    }
    else{ //CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX is 1
      uint16_t volume_db_master = microphone_settings.volume_db[0];
      uint16_t volume_db_mono = microphone_settings.volume_db[1];
      {
        int num_of_frames_read = num_bytes_read/(4 * 2);
        for(uint32_t i = 0; i < num_of_frames_read; i++){
          if(microphone_settings.resolution == 24){
            int32_t mono_24b = (int32_t)buffer[i].left;
            mono_24b = i2s_to_usb_32b_sample_convert(mono_24b, volume_db_mono);
            mono_24b = i2s_to_usb_32b_sample_convert(mono_24b, volume_db_master);

            i2s_24b_dummy_buffer[i] = mono_24b; // TODO: check this value
          }
          else {
            int32_t mono_24b = (int32_t)buffer[i].left >> 8;
            mono_24b = i2s_to_usb_16b_sample_convert(mono_24b, volume_db_mono);
            mono_24b = i2s_to_usb_16b_sample_convert(mono_24b, volume_db_master);

            i2s_16b_dummy_buffer[i]   = mono_24b; // TODO: check this value
          }
        }
      }
    }
  }
}

int32_t i2s_to_usb_32b_sample_convert(int32_t sample, uint16_t volume_db){
  #ifdef APPLY_VOLUME_FEATURE
    if(microphone_settings.user_mute) 
      return 0;
    else
    {
      int64_t sample_tmp = (int64_t)sample * (int64_t)volume_db;
      sample_tmp = sample_tmp>>15;
      return (int32_t)sample_tmp;
      //return (int32_t)sample;
    }
  #else //APPLY_VOLUME_FEATURE
    if(microphone_settings.user_mute) 
      return 0;
    else
    {
      return (int32_t)sample;
    }
  #endif //APPLY_VOLUME_FEATURE
}

int16_t i2s_to_usb_16b_sample_convert(int16_t sample, uint16_t volume_db){
  #ifdef APPLY_VOLUME_FEATURE
    if(microphone_settings.user_mute) 
      return 0;
    else
    {
      int32_t sample_tmp = (int32_t)(sample) * (int32_t)volume_db;
      sample_tmp = sample_tmp>>15;
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
    display_ssd1306_info();
  }
}

void button_mute_ISR(uint gpio, uint32_t events){
  microphone_settings.user_mute = !microphone_settings.user_mute;
}

void display_ssd1306_info(){
  ssd1306_clear(&disp);

  switch(microphone_settings.blink_interval_ms){
    case BLINK_NOT_MOUNTED:{
      ssd1306_draw_string(&disp, 4, 0, 1, "Microphone");
      ssd1306_draw_string(&disp, 4, 16, 1, "not mounted");
      break;
    }
    case BLINK_MOUNTED:{
      ssd1306_draw_string(&disp, 4, 0, 1, "Microphone");
      ssd1306_draw_string(&disp, 4, 16, 1, "mounted");
      break;
    }
    case BLINK_SUSPENDED:{
      ssd1306_draw_string(&disp, 4, 0, 1, "Microphone");
      ssd1306_draw_string(&disp, 4, 16, 1, "suspended");
      break;
    }
    case BLINK_STREAMING:{
      char format_str[20] = "Fmt:";
      char format_tmp_str[20] = "";

      itoa((microphone_settings.sample_rate/1000), format_tmp_str, 10);
      strcat(format_str, format_tmp_str);
      strcat(format_str, " kHz, ");

      itoa(microphone_settings.resolution, format_tmp_str, 10);
      strcat(format_str, format_tmp_str);
      strcat(format_str, " bit");

      char vol_str[20] = "Vol M:";
      char vol_tmp_str[20] = "";

      itoa(microphone_settings.volume[0]/*>>ENC_NUM_OF_FP_BITS*/, vol_tmp_str, 10);
      strcat(vol_str, vol_tmp_str);

      strcat(vol_str, " L:");
      itoa(microphone_settings.volume[1]/*>>ENC_NUM_OF_FP_BITS*/, vol_tmp_str, 10);
      strcat(vol_str, vol_tmp_str);

      strcat(vol_str, " R:");
      itoa(microphone_settings.volume[2]/*>>ENC_NUM_OF_FP_BITS*/, vol_tmp_str, 10);
      strcat(vol_str, vol_tmp_str);

      char mute_str[20] = "Mute M:";
      strcat(mute_str, (microphone_settings.mute[0] ? "T" : "F"));

      strcat(mute_str, " L:");
      strcat(mute_str, (microphone_settings.mute[1] ? "T" : "F"));

      strcat(mute_str, " R:");
      strcat(mute_str, (microphone_settings.mute[2] ? "T" : "F"));

      ssd1306_draw_string(&disp, 4, 0, 1, format_str);
      ssd1306_draw_string(&disp, 4, 8, 1, vol_str);
      ssd1306_draw_string(&disp, 4, 16, 1, mute_str);
    }
  } 
  
  ssd1306_show(&disp);
}

