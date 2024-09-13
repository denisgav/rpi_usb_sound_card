# USB stereo microphone
Microphone rupports several formats:
  * 24 / 16 bit samples
  * 16000 / 32000 / 48000 Hz sample rates

## Hardware
 * RP2040 board
   * [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/)
 * Microphone
   * [inmp441]
 * LCD
   * SSD1306
 * Buttons
 * LEDs
   * Red
   * WS2812 
 * Resistors 

### Default Pinout

#### INMP441 Microphone

| Raspberry Pi Pico / RP2040 | inmp441 I2S microphone | Define name | 
| --- | --- | --- | 
| 3V3 | VDD | - |
| GND | GND | - |
| 14  | SD  | I2S_MIC_SD |
| GND | Mic#0 L/R | - |
| 3V3 | Mic#1 L/R | - |
| 16 | SW  | I2S_MIC_WS |
| 15 | SCK | I2S_MIC_SCK |

GPIO pins are configurable in API by updating defines.

#### LCD SSD1306
| Raspberry Pi Pico / RP2040 | SSD1306 | Define name | 
| --- | --- | --- |
| GND | GND | - | 
| 3V3 | VCC | - | 
| 3   | SCL   | I2C_SSD1306_SCL | 
| 2   | SDA   | I2C_SSD1306_SDA | 


#### LED
| Raspberry Pi Pico / RP2040 | LED color | Define name | 
| --- | --- | --- |
| 10 | RED  | LED_RED_PIN | 
| 23 | WS2812  | WS2812_PIN | 

#### BUTTON
| Raspberry Pi Pico / RP2040 | Button name | Define name | 
| --- | --- | --- |
| 13 | Mute  | BTN_MUTE_PIN | 