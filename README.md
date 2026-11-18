# Wireless Signal Scanner
 
A 2.4 GHz carrier-presence monitor built with an ESP32, an nRF24L01 radio module, and an SSD1306 OLED. The sketch iterates register values 0–127 and displays channels that cross an RPD hit threshold. The nRF24L01 documented RF channel range is 0–125 (2400–2525 MHz); the current sketch still attempts values 126 and 127, so those last two iterations should not be treated as supported channels. The on-screen bar is a relative count of carrier-detect hits, not calibrated RSSI.
 
---
 
## What the Device Does
 
The nRF24L01 chip includes a hardware-level Received Power Detector (RPD) register. When the
radio is tuned to a channel and placed in receive mode, that register reads 1 if it senses any
carrier signal above -64 dBm, regardless of whether the signal is a valid nRF24L01 packet.
This means it can detect Wi-Fi, Bluetooth, Zigbee, RC controllers, baby monitors, wireless
keyboards, microwave-band interference, and anything else transmitting in the 2.4 GHz band.
 
The ESP32 samples the RPD register repeatedly at each configured register value and then displays entries that meet the hit threshold. This is carrier-energy detection only: it does not identify a protocol, decode packets, or measure calibrated signal strength. The current loop includes values 126 and 127 even though the documented nRF24L01 channel range ends at 125.
 
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
 
| Library              | Version guidance (not verified in this review) | Purpose                          |
|----------------------|----------------|----------------------------------|
| RF24                 | 1.4.x          | nRF24L01 driver and RPD access   |
| Adafruit GFX Library | 1.11.x         | Graphics primitives for the OLED |
| Adafruit SSD1306     | 2.5.x          | SSD1306 OLED driver              |
 
You also need the ESP32 board support package installed in Arduino IDE. Follow the official
Espressif guide at https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
 
---
 
## Building and Uploading
 
1. Clone or download this repository.
2. Open the root-level `wireless_scanner.ino` in the Arduino IDE. There is no `src/` directory or checked-in PlatformIO configuration in the current tree.
3. Install the RF24, Adafruit GFX, and Adafruit SSD1306 libraries using Library Manager, and install the ESP32 board support package.
4. Select the correct ESP32 board and serial port.
5. Confirm the nRF24L01 VCC is connected to **3.3 V, never 5 V**, and place a 10–100 µF capacitor close to the radio's VCC/GND pins before powering the circuit.
6. Upload the sketch and open Serial Monitor at 115200 baud.

No compile or hardware test was run during this documentation review. For PlatformIO, create a separate local project and configure the root sketch/libraries there; no PlatformIO build is claimed for this repository as committed.
 
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

The checked-in files are at the repository root; the folder layout below reflects the actual tree.

- `wireless_scanner.ino` — ESP32/nRF24L01/OLED firmware
- `index.html` — interactive browser visualization with simulated channel activity; it does not connect to the hardware
- `code_explanation.md` — source walkthrough
- `hardware_explanation.md` — component and hardware notes
- `schematic_notes.md` — pin-level wiring notes
- `wiring_diagram.txt` — ASCII breadboard wiring reference
- `LICENSE (5)` — file containing the MIT license text; its suffixed name is not the conventional `LICENSE` path
- `README.md` — this guide

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
- The device cannot decode or read the content of any detected signal. It only detects
  carrier presence above the radio's RPD threshold.
- The hit-count bar is not calibrated RSSI and cannot be compared as an absolute signal-strength measurement.
- The current loop attempts channel values 126 and 127 even though the documented nRF24L01 channel range is 0–125.
- Channels above 2483 MHz (channel 83) are outside the standard 2.4 GHz ISM band. The
  nRF24L01 can tune there, but regulatory rules in most countries restrict transmissions in
  that range.
---
 
## License
 
MIT License. See LICENSE file for full text.


---


## Maintenance & Audit

- **Audit date:** 18 November 2026
- **Documentation status:** README reviewed and audit record added on this date.
- **Scope and evidence:** This is a documentation-level maintenance pass. It does not establish that source code compiles, dependencies are current, security has been tested, or hardware/runtime behavior has been verified.
- **Code and tests:** No new functional code changes or test results are claimed by this README update. Run project-specific build, test, and hardware/runtime checks before relying on the project.
- **Maintenance baseline:** Documentation status recorded through 11 November 2026; revisit setup instructions, dependencies, known limitations, and tests when the project changes.
