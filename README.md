# Wireless Signal Scanner
 
A 2.4 GHz RF channel scanner built with an ESP32 microcontroller, an nRF24L01 radio module,
and a 0.96-inch SSD1306 OLED display. The device sweeps all 128 channels in the 2.4 GHz band
and shows each detected active signal on the OLED screen one at a time, displaying the channel
number, frequency, a visual strength bar, and a numeric hit count.
 
---
 
## What the Device Does
 
The nRF24L01 chip includes a hardware-level Received Power Detector (RPD) register. When the
radio is tuned to a channel and placed in receive mode, that register reads 1 if it senses any
carrier signal above -64 dBm, regardless of whether the signal is a valid nRF24L01 packet.
This means it can detect Wi-Fi, Bluetooth, Zigbee, RC controllers, baby monitors, wireless
keyboards, microwave-band interference, and anything else transmitting in the 2.4 GHz band.
 
The ESP32 rapidly iterates over all 128 channels (2400 MHz to 2527 MHz, one channel per MHz),
samples the RPD register multiple times per channel to reduce false positives, and then
displays each active channel individually on the OLED with a short pause between entries.
 
---
 
## Hardware Requirements
 
| Component           | Specification / Notes                               |
|---------------------|-----------------------------------------------------|
| Microcontroller     | ESP32 development board (any 38-pin or 30-pin variant) |
| RF Module           | nRF24L01+ (the + variant is recommended for better sensitivity) |
| OLED Display        | 0.96-inch SSD1306, 128x64 pixels, I2C interface     |
| Capacitor           | 10-100 uF electrolytic across nRF24L01 VCC and GND (prevents brownouts) |
| Breadboard + wires  | Standard dupont jumper wires                        |
| USB cable           | Micro-USB or USB-C depending on your ESP32 board    |
| Power supply        | USB power bank or 5V wall adapter                   |
 
---
 
## Wiring
 
### nRF24L01 to ESP32
 
The nRF24L01 operates at 3.3 V logic and supply. Most ESP32 breakout boards provide a 3.3 V pin
that can supply the module. Do NOT connect VCC to the 5 V pin; that will destroy the radio.
 
```
nRF24L01 Pin    ESP32 Pin
-----------     ---------
VCC             3.3V
GND             GND
CE              GPIO 4
CSN             GPIO 5
SCK             GPIO 18  (hardware SPI clock)
MOSI            GPIO 23  (hardware SPI data out)
MISO            GPIO 19  (hardware SPI data in)
IRQ             not connected
```
 
Solder or clip a 10-100 uF capacitor directly across the VCC and GND pins of the nRF24L01
module. RF modules draw short but heavy current spikes and without this capacitor you will see
random resets or failed initialisation.
 
### SSD1306 OLED to ESP32
 
```
OLED Pin    ESP32 Pin
--------    ---------
VCC         3.3V
GND         GND
SDA         GPIO 21  (hardware I2C data)
SCL         GPIO 22  (hardware I2C clock)
```
 
The I2C address on most SSD1306 breakout boards is 0x3C. A small number of boards use 0x3D.
If the display does not initialise, try changing OLED_I2C_ADDR in the source file.
 
---
 
## Software Dependencies
 
Install all of the following through the Arduino IDE Library Manager (Sketch > Include Library
> Manage Libraries) or through PlatformIO:
 
| Library              | Tested Version | Purpose                          |
|----------------------|----------------|----------------------------------|
| RF24                 | 1.4.x          | nRF24L01 driver and RPD access   |
| Adafruit GFX Library | 1.11.x         | Graphics primitives for the OLED |
| Adafruit SSD1306     | 2.5.x          | SSD1306 OLED driver              |
 
You also need the ESP32 board support package installed in Arduino IDE. Follow the official
Espressif guide at https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
 
---
 
## Building and Uploading
 
1. Clone or download this repository.
2. Open `src/wireless_scanner.ino` in the Arduino IDE.
3. Select your ESP32 board under Tools > Board > ESP32 Arduino.
4. Select the correct COM port under Tools > Port.
5. Click Upload.
6. Open the Serial Monitor at 115200 baud to watch scan results in text form.
If you prefer PlatformIO, rename the sketch to `main.cpp`, add the libraries to
`platformio.ini` under `lib_deps`, and set `board = esp32dev`.
 
---
 
## Adjustable Parameters
 
All tuneable values are defined as constants near the top of `wireless_scanner.ino`:
 
| Constant             | Default | Effect                                                       |
|----------------------|---------|--------------------------------------------------------------|
| NRF_CE_PIN           | 4       | ESP32 GPIO pin connected to CE on the radio                  |
| NRF_CSN_PIN          | 5       | ESP32 GPIO pin connected to CSN on the radio                 |
| SAMPLES_PER_CHANNEL  | 64      | How many RPD reads per channel. Higher = less false positives |
| DETECTION_THRESHOLD  | 2       | Minimum RPD hits for a channel to count as active            |
| DISPLAY_HOLD_MS      | 1800    | Milliseconds each detected signal stays on screen            |
| INTER_SCAN_DELAY_MS  | 300     | Pause between full scan cycles                               |
 
---
 
## OLED Display Layout
 
Each detected signal is shown with the following layout:
 
```
+---------------------------+
| SIGNAL DETECTED           |
+---------------------------+
| CH: 79                    |
| Freq: 2479 MHz            |
| Sig: [######....]         |
| Hits: 38/64               |
+---------------------------+
```
 
The bar graph uses 10 segments. Each filled segment (#) represents roughly 10 percent of the
maximum possible hit count. An empty dot (.) represents a quiet segment. A channel with 64
out of 64 hits would show all 10 segments filled; a just-threshold detection with 2 hits would
show 1 filled segment.
 
---
 
## Project Structure
 
```
wireless-scanner/
├── src/
│   └── wireless_scanner.ino       Main Arduino sketch
├── docs/
│   ├── code_explanation.md        Detailed walkthrough of how the code works
│   └── hardware_explanation.md    How each hardware component works
├── hardware/
│   └── wiring_diagram.txt         ASCII wiring reference
├── schematics/
│   └── schematic_notes.md         Pin-level schematic description
├── README.md                      This file
└── LICENSE                        MIT License
```
 
---
 
## Serial Monitor Output
 
While connected over USB you can watch the scan in real time:
 
```
Wireless Scanner starting...
OLED display initialised OK
nRF24L01 initialised OK
Channel: 1   Freq: 2401 MHz  Hits: 4
Channel: 6   Freq: 2406 MHz  Hits: 58
Channel: 11  Freq: 2411 MHz  Hits: 61
Channel: 79  Freq: 2479 MHz  Hits: 22
...
```
 
Wi-Fi channels 1, 6, and 11 are the three non-overlapping 2.4 GHz Wi-Fi channels used by
most routers in North America. You will almost certainly see high hit counts on those three
channels if any Wi-Fi networks are nearby.
 
---
 
## Known Limitations
 
- The nRF24L01 RPD threshold is fixed at -64 dBm by the silicon. Signals weaker than that
  will not be detected even if they are present.
- The scan sweeps one channel at a time and is not simultaneous. A very short burst
  transmission might be missed if it happens to occur on a channel while the radio is tuned
  elsewhere.
- The device cannot decode or read the content of any detected signal. It only detects the
  presence of RF energy.
- Channels above 2483 MHz (channel 83) are outside the standard 2.4 GHz ISM band. The
  nRF24L01 can tune there, but regulatory rules in most countries restrict transmissions in
  that range.
---
 
## License
 
MIT License. See LICENSE file for full text.
