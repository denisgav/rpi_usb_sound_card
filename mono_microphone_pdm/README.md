# USB stereo microphone
Microphone rupports several formats:
  * 16 bit samples
  * 16000 Hz sample rates

## Hardware
 * RP2040 board
   * [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/)
 * Microphone
   * [PDM mic] (https://learn.adafruit.com/adafruit-pdm-microphone-breakout/overview)
 * LCD
   * SSD1306
 * Buttons
 * LEDs
 * Resistors 

### Default Pinout

#### PDM Microphone 0

| Raspberry Pi Pico / RP2040 | PDM microphone | Define name | 
| --- | --- | --- | 
| 3V3 | 3V | - |
| GND | GND | - |
| GND | SEL  | - |
| 19 | CLK | PDM_MIC_0_CLK |
| 18 | DAT | PDM_MIC_0_DATA |

#### PDM Microphone 1

| Raspberry Pi Pico / RP2040 | PDM microphone | Define name | 
| --- | --- | --- | 
| 3V3 | 3V | - |
| GND | GND | - |
| GND | SEL  | - |
| 21 | CLK | PDM_MIC_1_CLK |
| 20 | DAT | PDM_MIC_1_DATA |

GPIO pins are configurable in API by updating defines.

#### LED
| Raspberry Pi Pico / RP2040 | LED color | Define name | 
| --- | --- | --- |
| 10 | RED  | LED_RED_PIN | 
| 23	| WS2812 |	WS2812_PIN |

#### BUTTON
| Raspberry Pi Pico / RP2040 | Button name | Define name | 
| --- | --- | --- |
| 13 | Mute  | BTN_MUTE_PIN | 
