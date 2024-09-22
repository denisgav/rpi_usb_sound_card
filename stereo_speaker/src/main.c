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

#include "usb_speaker.h"

#include "main.h"

#include "i2s/machine_i2s.h"
#include "volume_ctrl.h"

#include "ssd1306/ssd1306.h"

#include "ws2812/ws2812.h"

// Pointer to I2S handler
machine_i2s_obj_t* speaker_i2s0 = NULL;

speaker_settings_t speaker_settings;

// Buffer for speaker data
//i2s_32b_audio_sample spk_i2s_buffer[SAMPLE_BUFFER_SIZE];
i2s_32b_audio_sample spk_32b_i2s_buffer[SAMPLE_BUFFER_SIZE];
i2s_16b_audio_sample spk_16b_i2s_buffer[SAMPLE_BUFFER_SIZE];
int32_t spk_buf[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / 4];

void led_blinking_task(void);
void status_update_task(void);

int32_t usb_to_i2s_32b_sample_convert(int32_t sample, uint16_t volume_db);

int16_t usb_to_i2s_16b_sample_convert(int16_t sample, uint16_t volume_db);

void refresh_i2s_connections()
{
  speaker_settings.samples_in_i2s_frame_min = (speaker_settings.sample_rate)    /1000;
  speaker_settings.samples_in_i2s_frame_max = (speaker_settings.sample_rate+999)/1000;

  speaker_i2s0 = create_machine_i2s(0, I2S_SPK_SCK, I2S_SPK_WS, I2S_SPK_SD, TX, 
    ((speaker_settings.resolution == 16) ? 16 : 32), STEREO, /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, speaker_settings.sample_rate);
  
  // update_pio_frequency(speaker_i2s0, speaker_settings.usb_sample_rate);
}


void usb_speaker_mute_handler(int8_t bChannelNumber, int8_t mute_in);
void usb_speaker_volume_handler(int8_t bChannelNumber, int16_t volume_in);
void usb_speaker_current_sample_rate_handler(uint32_t current_sample_rate_in);
void usb_speaker_current_resolution_handler(uint8_t current_resolution_in);
void usb_speaker_current_status_set_handler(uint32_t blink_interval_ms_in);

void usb_speaker_tud_audio_rx_done_pre_read_handler(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting);


//---------------------------------------
//           SSD1306
//---------------------------------------
ssd1306_t disp;
void setup_ssd1306();
void display_ssd1306_info();
//---------------------------------------

/*------------- MAIN -------------*/
int main(void)
{
  speaker_settings.sample_rate  = I2S_SPK_RATE_DEF;
  speaker_settings.resolution = CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX;
  speaker_settings.blink_interval_ms = BLINK_NOT_MOUNTED;
  speaker_settings.status_updated = false;

  setup_ssd1306();
  ws2812_init();

  usb_speaker_set_mute_set_handler(usb_speaker_mute_handler);
  usb_speaker_set_volume_set_handler(usb_speaker_volume_handler);
  usb_speaker_set_current_sample_rate_set_handler(usb_speaker_current_sample_rate_handler);
  usb_speaker_set_current_resolution_set_handler(usb_speaker_current_resolution_handler);
  usb_speaker_set_current_status_set_handler(usb_speaker_current_status_set_handler);

  usb_speaker_set_tud_audio_rx_done_pre_read_set_handler(usb_speaker_tud_audio_rx_done_pre_read_handler);
  
  usb_speaker_init();
  refresh_i2s_connections();

  for(int i=0; i<(CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1); i++)
  {
    speaker_settings.volume[i] = DEFAULT_VOLUME;
    speaker_settings.mute[i] = 0;
    speaker_settings.volume_db[i] = vol_to_db_convert_enc(speaker_settings.mute[i], speaker_settings.volume[i]);
  }

  while (1) {
    usb_speaker_task();

    led_blinking_task();

    status_update_task();

    ws2812_task(speaker_settings.blink_interval_ms);
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

  ssd1306_draw_string(&disp, 4, 16, 1, "Raspberry pi pico");
  ssd1306_draw_string(&disp, 8, 32, 1, "USB UAC2 speaker");
  ssd1306_show(&disp);
}
//-------------------------

//-------------------------

//-------------------------
// callback functions
//-------------------------


void usb_speaker_mute_handler(int8_t bChannelNumber, int8_t mute_in)
{
  speaker_settings.mute[bChannelNumber] = mute_in;
  speaker_settings.volume_db[bChannelNumber] = vol_to_db_convert_enc(speaker_settings.mute[bChannelNumber], speaker_settings.volume[bChannelNumber]);
  speaker_settings.status_updated = true;
}

void usb_speaker_volume_handler(int8_t bChannelNumber, int16_t volume_in)
{
  speaker_settings.volume[bChannelNumber] = volume_in;
  speaker_settings.volume_db[bChannelNumber] = vol_to_db_convert_enc(speaker_settings.mute[bChannelNumber], speaker_settings.volume[bChannelNumber]);
  speaker_settings.status_updated = true;
}

void usb_speaker_current_sample_rate_handler(uint32_t current_sample_rate_in)
{
  speaker_settings.sample_rate = current_sample_rate_in;
  refresh_i2s_connections();
  speaker_settings.status_updated = true;
}

void usb_speaker_current_resolution_handler(uint8_t current_resolution_in)
{
  speaker_settings.resolution = current_resolution_in;
  refresh_i2s_connections();
  speaker_settings.status_updated = true;
}

void usb_speaker_current_status_set_handler(uint32_t blink_interval_ms_in)
{
  speaker_settings.blink_interval_ms = blink_interval_ms_in;
  speaker_settings.status_updated = true;
}

void usb_speaker_tud_audio_rx_done_pre_read_handler(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting)
{
  uint16_t volume_db_master = speaker_settings.volume_db[0];
  uint16_t volume_db_left = speaker_settings.volume_db[1];
  uint16_t volume_db_right = speaker_settings.volume_db[2];

  if(speaker_i2s0 && (speaker_settings.blink_interval_ms == BLINK_STREAMING)){
    // Speaker data size received in the last frame
    uint16_t usb_spk_data_size = tud_audio_read(spk_buf, n_bytes_received);
    uint16_t usb_sample_count = 0;

    if (speaker_settings.resolution == 16)
    {
      int16_t *in = (int16_t *) spk_buf;
      usb_sample_count = usb_spk_data_size/4; // 4 bytes per sample 2b left, 2b right

      if(usb_sample_count >= speaker_settings.samples_in_i2s_frame_min){
        for (int i = 0; i < usb_sample_count; i++) {
          int16_t left = in[i*2 + 0];
          int16_t right = in[i*2 + 1];

          left = usb_to_i2s_16b_sample_convert(left, volume_db_left);
          left = usb_to_i2s_16b_sample_convert(left, volume_db_master);
          right = usb_to_i2s_16b_sample_convert(right, volume_db_right);
          right = usb_to_i2s_16b_sample_convert(right, volume_db_master);
          spk_16b_i2s_buffer[i].left  = left;
          spk_16b_i2s_buffer[i].right = right;
        }
        machine_i2s_write_stream(speaker_i2s0, (void*)&spk_16b_i2s_buffer[0], usb_sample_count*4);
      }
    }
    else if (speaker_settings.resolution == 24)
    {
      int32_t *in = (int32_t *) spk_buf;
      usb_sample_count = usb_spk_data_size/8; // 8 bytes per sample 4b left, 4b right

      if(usb_sample_count >= speaker_settings.samples_in_i2s_frame_min){
        for (int i = 0; i < usb_sample_count; i++) {
          int32_t left = in[i*2 + 0];
          int32_t right = in[i*2 + 1];

          left = usb_to_i2s_32b_sample_convert(left, volume_db_left);
          left = usb_to_i2s_32b_sample_convert(left, volume_db_master);
          right = usb_to_i2s_32b_sample_convert(right, volume_db_right);
          right = usb_to_i2s_32b_sample_convert(right, volume_db_master);
          spk_32b_i2s_buffer[i].left  = left;
          spk_32b_i2s_buffer[i].right = right;
        }
        machine_i2s_write_stream(speaker_i2s0, (void*)&spk_32b_i2s_buffer[0], usb_sample_count*8);
      }
    }
  }
}


int32_t usb_to_i2s_32b_sample_convert(int32_t sample, uint16_t volume_db)
{
  int64_t sample_tmp = (int64_t)sample * (int64_t)volume_db;
  sample_tmp = sample_tmp>>15;
  return (int32_t)sample_tmp;
  //return (int32_t)sample;
}

int16_t usb_to_i2s_16b_sample_convert(int16_t sample, uint16_t volume_db)
{
  int32_t sample_tmp = (int32_t)sample * (int32_t)volume_db;
  sample_tmp = sample_tmp>>15;
  return (int16_t)sample_tmp;
  //return (int16_t)sample;
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
  if (cur_time_ms - start_ms < speaker_settings.blink_interval_ms) return;
  start_ms += speaker_settings.blink_interval_ms;

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

  if(speaker_settings.status_updated == true){
    speaker_settings.status_updated = false;
    display_ssd1306_info();
  }
}

void display_ssd1306_info(){
  ssd1306_clear(&disp);

  switch(speaker_settings.blink_interval_ms){
    case BLINK_NOT_MOUNTED:{
      ssd1306_draw_string(&disp, 4, 0, 1, "Speaker");
      ssd1306_draw_string(&disp, 4, 16, 1, "not mounted");
      break;
    }
    case BLINK_SUSPENDED:{
      ssd1306_draw_string(&disp, 4, 0, 1, "Speaker");
      ssd1306_draw_string(&disp, 4, 16, 1, "suspended");
      break;
    }
    case BLINK_MOUNTED:{
      ssd1306_draw_string(&disp, 4, 0, 1, "Speaker");
      ssd1306_draw_string(&disp, 4, 16, 1, "mounted");
      break;
    }
    case BLINK_STREAMING:{
      char format_str[20] = "Fmt:";
      char format_tmp_str[20] = "";

      itoa((speaker_settings.sample_rate), format_tmp_str, 10);
      strcat(format_str, format_tmp_str);
      strcat(format_str, " Hz, ");

      itoa(speaker_settings.resolution, format_tmp_str, 10);
      strcat(format_str, format_tmp_str);
      strcat(format_str, " bit");

      char vol_str[20] = "Vol M:";
      char vol_tmp_str[20] = "";

      itoa((speaker_settings.volume[0]>>ENC_NUM_OF_FP_BITS), vol_tmp_str, 10);
      strcat(vol_str, vol_tmp_str);

      strcat(vol_str, " L:");
      itoa((speaker_settings.volume[1]>>ENC_NUM_OF_FP_BITS), vol_tmp_str, 10);
      strcat(vol_str, vol_tmp_str);

      strcat(vol_str, " R:");
      itoa((speaker_settings.volume[2]>>ENC_NUM_OF_FP_BITS), vol_tmp_str, 10);
      strcat(vol_str, vol_tmp_str);

      char mute_str[20] = "Mute M:";
      strcat(mute_str, (speaker_settings.mute[0] ? "T" : "F"));

      strcat(mute_str, " L:");
      strcat(mute_str, (speaker_settings.mute[1] ? "T" : "F"));

      strcat(mute_str, " R:");
      strcat(mute_str, (speaker_settings.mute[2] ? "T" : "F"));

      ssd1306_draw_string(&disp, 4, 0, 1, format_str);
      ssd1306_draw_string(&disp, 4, 12, 1, vol_str);
      ssd1306_draw_string(&disp, 4, 24, 1, mute_str);
    }
  } 
  
  ssd1306_show(&disp);
}