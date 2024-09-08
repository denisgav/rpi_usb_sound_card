# Making USB sound card using Raspberry pi pico

Capture audio from a microphone on your [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/) or any [RP2040](https://www.raspberrypi.org/products/rp2040/) based board. 🎤


## Hardware

 * RP2040 board
   * [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/)
 * Microphones
   * I2S
     * [inmp441]
   * PDM
     * [PDM mic](https://learn.adafruit.com/adafruit-pdm-microphone-breakout/overview)
 * DAC
   * [PCM5102A](http://www.kosmodrom.com.ua/el.php?name=PCM5102A-Modul)
   * [UDA1334A](https://learn.adafruit.com/adafruit-i2s-stereo-decoder-uda1334a)
 * LCD
   * SSD1306
 * Buttons
 * LEDs
 * Resistors 


#### INMP441 Microphone

| Raspberry Pi Pico / RP2040 | I2S Microphone |
| --- | --- |
| 3.3V | VCC |
| GND | GND |
| GPIO 14 | SD |
| GPIO 15 | SCK |
| GPIO 16 | WS |


## Examples

See [applications]:
 * [stereo_microphone_i2s]( stereo_microphone_i2s )
 * [stereo_microphone_pdm]( stereo_microphone_pdm )
 * [stereo_speaker]( stereo_speaker )


## Cloning

```sh
git clone https://github.com/denisgav/rpi_usb_sound_card.git
```

## Building

1. [Set up the Pico C/C++ SDK](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
2. Set `PICO_SDK_PATH`
```sh
export PICO_SDK_PATH=/path/to/pico-sdk
```
3. Create `build` dir, run `cmake` and `make`:
```
mkdir build
cd build
cmake .. -DPICO_BOARD=pico
make
```
4. Copy example `.uf2` to Pico when in BOOT mode.


## Acknowledgements

To create this project, following references were used:
 * The [TinyUSB](https://github.com/hathach/tinyusb) library.
 * Machine I2S  https://github.com/sfera-labs/arduino-pico-i2s-audio
 * Microphone library for pico https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico.git 
 * SSD1306 library for pico https://github.com/daschr/pico-ssd1306.git
---


