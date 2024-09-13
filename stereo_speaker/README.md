# USB stereo speaker
Speaker rupports several formats:
  * 24 / 16 bit samples
  * 44100 / 48000 Hz sample rates

## Hardware
 * RP2040 board
   * [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/)
 * DAC
   * [PCM5102A] (http://www.kosmodrom.com.ua/el.php?name=PCM5102A-Modul)
   * [UDA1334A] (https://learn.adafruit.com/adafruit-i2s-stereo-decoder-uda1334a)
* LCD
   * SSD1306
 * LEDs
 * Resistors 

### Default Pinout

#### PCM5102a DAC

 * PCM5102A configuration inputs:
    * FLT - Filter select : Normal latency (Low) / Low latency (High)
    * DEMP - De-emphasis control for 44.1kHz sampling rate: Off (Low) / On (High)
    * XSMT - Soft mute control(1): Soft mute (Low) / soft un-mute (High)
    * FMT - Audio format selection : I2S (Low) / Left justified (High)
 * PCM5102A I2S interface:
    * SCK - System clock input
    * BCK - Audio data bit clock input
    * DIN - Audio data input
    * LRCK - Audio data word clock input

| Raspberry Pi Pico / RP2040 | PCM5102A I2S DAC | Define name | 
| --- | --- | --- | 
| 5 V | VCC | - |
| -   | 3v3 | - |
| GND | GND | - |
| FLT | GND | - |
| DMP | GND | - |
| SCL | GND | - |
| BCK | 15  | I2S_SPK_SCK |
| DIN | 14  | I2S_SPK_SD |
| LCK | 16  | I2S_SPK_WS |
| FMT | GND | - |
| XMT | 3v3 | - |


GPIO pins are configurable in API by updating defines.


#### UDA1334A DAC

  * UDA1334A configuration inputs:
    * SCLK (Sys Clock) - Optional 27 MHz 'video mode' ssytem clock input - by default we generate the sysclock from the WS clock in 'audio mode' But the UDA can also take a oscillator input on this pin
    * Mute - Setting this pin High will mute the output 
    * De-Emphasis - In audio mode (which is the default), can be used to add a de-emphasis filter. In video mode, where the system clock is generated from an oscillator, this is the clock output.
    * PLL - sets the PLL mode, by default pulled low for Audio. Can be pulled high or set to ~1.6V to set PAL or NTSC video frequency
    * SF0 and SF1 are used to set the input data format. By default both are pulled Low for I2S but you can change them around for alternate formats.

  * UDA1334A I2S interface:
    * WSEL (Word Select or Left/Right Clock) - this is the pin that tells the DAC when the data is for the left
    * DIN (Data In) - This is the pin that has the actual data coming in, both left and right data are sent on this pin, the WSEL pin indicates when left or right is being transmitted
    * BCLK (Bit Clock) - This is the pin that tells the amplifier when to read data on the data pin.

| Raspberry Pi Pico / RP2040 | UDA1334A I2S DAC | Define name | 
|  --- | --- | --- |
| 5 V  | Vin | - | 
| -    | 3v0 | - | 
| GND  | GND | - | 
| WSEL | 16 | I2S_SPK_WS |
| DIN  | 14 | I2S_SPK_SD |
| BCLK | 15 | I2S_SPK_SCK|
| SCLK | - | - |
| SF1  | - | - |
| MUTE | - | - |
| SF0  | - | - |
| PLL  | - | - |
| DEEM | - | - |


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
| 10 | WHITE  | LED_WHITE_PIN | 
| 11 | YELLOW | LED_YELLOW_PIN | 
| 12 | GREEN  | LED_GREEN_PIN | 

