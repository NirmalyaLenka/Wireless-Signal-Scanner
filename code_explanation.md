# Code Explanation

This document walks through every section of `wireless_scanner.ino` in plain language. You do
not need to be an expert Arduino programmer to follow this; the goal is that after reading this
you understand not just what each line does but why it is there.

---

## High-Level Flow

When power is applied the firmware runs `setup()` once, then calls `loop()` forever. Inside
`loop()` there are three phases in every iteration:

1. Scan all 128 RF channels and count how many times each one shows a carrier signal.
2. Walk through the results and display each active channel on the OLED one at a time.
3. Pause briefly and then start the next scan.

That is the entire program at a conceptual level. Everything else is implementation detail.

---

## Header Includes

```cpp
#include <SPI.h>
#include <Wire.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
```

SPI.h is part of the Arduino core and gives us access to the hardware SPI bus, which is how
the ESP32 talks to the nRF24L01 module. Wire.h is the I2C library; the SSD1306 OLED uses I2C.
RF24.h is the community-maintained driver for the nRF24L01 family. Adafruit_GFX.h provides
drawing primitives (lines, rectangles, text) that are hardware-independent. Adafruit_SSD1306.h
is the hardware-specific driver that sits on top of Adafruit_GFX and knows how to push pixels
to a SSD1306 chip.

---

## Pin and Configuration Constants

```cpp
#define NRF_CE_PIN   4
#define NRF_CSN_PIN  5
#define OLED_WIDTH   128
#define OLED_HEIGHT   64
#define OLED_I2C_ADDR 0x3C
#define RF_CHANNEL_COUNT 128
#define SAMPLES_PER_CHANNEL 64
#define DETECTION_THRESHOLD 2
#define DISPLAY_HOLD_MS 1800
#define INTER_SCAN_DELAY_MS 300
```

Using `#define` constants instead of hardcoded numbers throughout the code means you can
change behaviour by editing one line. CE (Chip Enable) on the nRF24L01 is the pin that puts
the radio into active RX or TX mode; CSN (Chip Select Not) is the SPI chip-select line that
tells the radio when the ESP32 is addressing it rather than some other SPI device.

SAMPLES_PER_CHANNEL and DETECTION_THRESHOLD work together to filter noise. With 64 samples
per channel and a threshold of 2, a channel must show a carrier at least twice (3 percent of
samples) to be reported. In a quiet room with no transmitters nearby you should see close to
zero hits on most channels. In a home with Wi-Fi you will see near-maximum hits on channels
6 and 11 (the most commonly used Wi-Fi channels in the 2.4 GHz band).

---

## Global Object Creation

```cpp
RF24 radio(NRF_CE_PIN, NRF_CSN_PIN);
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
uint8_t channelHits[RF_CHANNEL_COUNT];
```

radio is the RF24 driver object. Passing the pin numbers here tells the driver which GPIO
pins control the radio; it handles all the SPI communication internally.

display is the OLED driver. The -1 as the last argument means we have no hardware reset pin
connected from the ESP32 to the display; the display resets itself over the I2C bus.

channelHits is a plain array of 128 bytes. After each scan sweep, channelHits[n] holds the
number of times channel n showed a carrier signal out of SAMPLES_PER_CHANNEL attempts.

---

## setup()

```cpp
void setup() {
  Serial.begin(115200);
  setupDisplay();
  setupRadio();
}
```

The order matters here. We initialise the display before the radio because if the radio
initialisation fails we want to be able to show an error message on screen. Serial is started
first so that debug messages from inside setupDisplay() and setupRadio() appear in the monitor.

---

## setupDisplay()

This function calls `display.begin()`. If the SSD1306 is not found on the I2C bus that call
returns false and we halt the program. If it is found we clear the frame buffer, write a
three-line splash screen to it, and call `display.display()` to push the buffer to the actual
hardware. Everything drawn to an Adafruit_SSD1306 object is first written to a RAM buffer on
the ESP32; nothing appears on screen until you call `display.display()`. This is an important
concept: draw everything you want, then commit once.

The two-second delay after the splash screen is purely cosmetic. It gives the user time to
read the startup message before scanning begins.

---

## setupRadio()

```cpp
if (!radio.begin()) { ... halt ... }
radio.setPALevel(RF24_PA_MIN);
radio.setDataRate(RF24_2MBPS);
radio.setAutoAck(false);
radio.disableDynamicPayloads();
```

`radio.begin()` initialises SPI communication and checks that the radio responds. If it
returns false the radio is not connected, is wired incorrectly, or does not have stable power
(the capacitor on VCC is critical here).

PA level is set to minimum because we are only receiving, never transmitting. There is no
reason to run the power amplifier at high output.

Data rate is set to 2 Mbps (the highest available) because at higher data rates the nRF24L01
uses a wider channel bandwidth, which makes the RPD register slightly more sensitive to
nearby signals. For scanning purposes this is what you want.

Auto-acknowledgement is disabled because we have no intention of receiving packets; we are
only looking at the carrier detect register. Leaving ACK enabled would cause the chip to
transmit ACK packets if it ever decoded something that looked like a valid address, which would
interfere with our passive monitoring goal.

---

## loop()

```cpp
memset(channelHits, 0, sizeof(channelHits));
scanAllChannels();

uint8_t totalFound = 0;
for (uint8_t ch = 0; ch < RF_CHANNEL_COUNT; ch++) {
    if (channelHits[ch] >= DETECTION_THRESHOLD) {
        totalFound++;
    }
}
```

`memset` zeroes the entire channelHits array before each scan cycle. Without this, counts
from the previous cycle would still be present and corrupt the new results.

After scanning, we count how many channels crossed the threshold. If nothing was found we call
displayIdleScreen() and exit the loop iteration early. The `return` in the idle branch skips
the display loop below, which means we go straight back to scanning. This keeps the device
responsive in quiet RF environments.

```cpp
for (uint8_t ch = 0; ch < RF_CHANNEL_COUNT; ch++) {
    if (channelHits[ch] >= DETECTION_THRESHOLD) {
        displaySignal(ch, channelHits[ch]);
        delay(DISPLAY_HOLD_MS);
    }
}
```

This inner loop shows each active channel one at a time. The delay gives the user time to
read the screen before it advances to the next channel. Channels are displayed in ascending
order (lowest channel number first). A full Wi-Fi environment might show 5-10 active channels;
the whole display cycle for those would take about 9 to 18 seconds before the next scan begins.

---

## scanAllChannels()

```cpp
radio.startListening();

for (uint8_t ch = 0; ch < RF_CHANNEL_COUNT; ch++) {
    radio.setChannel(ch);
    uint8_t hits = 0;
    for (uint8_t s = 0; s < SAMPLES_PER_CHANNEL; s++) {
        delayMicroseconds(130);
        if (radio.testRPD()) {
            hits++;
        }
    }
    channelHits[ch] = hits;
}

radio.stopListening();
```

`radio.startListening()` puts the nRF24L01 into active RX mode. Only in this mode does the
RPD register update.

For each channel we call `radio.setChannel(ch)` to retune the synthesiser, wait 130
microseconds for the LNA (low-noise amplifier) and PLL (phase-locked loop) to settle on the
new frequency, and then read the RPD register via `radio.testRPD()`. The 130 microsecond
value was chosen empirically; it matches the nRF24L01 datasheet's Tpd2stby specification for
settling time. Going faster risks reading the RPD before the synthesiser has locked.

The inner loop runs SAMPLES_PER_CHANNEL times per channel. Because the `delayMicroseconds`
call consumes most of the time, the full scan of all 128 channels takes approximately:

  128 channels x 64 samples x 130 microseconds = 1.07 seconds per full sweep

This is fast enough to catch most persistent transmitters and short enough that the OLED
display does not feel sluggish.

---

## displaySignal()

```cpp
uint16_t freqMHz = 2400 + channel;
```

The nRF24L01 defines channel 0 as 2400 MHz and each subsequent channel adds 1 MHz. This
single arithmetic line converts the channel number to a human-readable frequency.

The function then builds the display layout in layers: header text, a horizontal divider
line, the large channel number, the frequency string, the bar graph, and the hit count. Each
element is positioned with `display.setCursor(x, y)` where x and y are pixel coordinates
with the origin at the top-left corner of the 128x64 display.

The bar graph string is built character by character inside a loop:

```cpp
for (uint8_t b = 0; b < 10; b++) {
    if (b < bars) { display.print("#"); }
    else          { display.print("."); }
}
```

This prints exactly 10 characters: filled # characters up to the bar count, then dot
characters for the remainder. It is a simple but effective ASCII-style progress bar.

---

## mapHitsToBar()

```cpp
uint8_t bar = (uint8_t)(((uint16_t)hits * 10) / SAMPLES_PER_CHANNEL);
```

This scales the hit count from the range [0, SAMPLES_PER_CHANNEL] to [0, 10]. The cast to
uint16_t before the multiplication prevents overflow: hits (max 64) times 10 equals 640,
which does not fit in a uint8_t (max 255) but fits easily in a uint16_t. This is a subtle
but important detail; leaving it as uint8_t arithmetic would cause the result to wrap around
incorrectly for any hits value above 25.

The function also guarantees a minimum of 1 bar for any channel that crossed the detection
threshold. Without this guarantee a channel with exactly 2 hits out of 64 would compute a
bar count of 0 (since 2 * 10 / 64 = 0 in integer arithmetic), showing an empty bar even
though it was detected. The minimum-1 rule makes it visually clear that something was found.

---

## Memory Usage Notes

The channelHits array is 128 bytes. The Adafruit_SSD1306 driver allocates a frame buffer
equal to OLED_WIDTH * OLED_HEIGHT / 8 = 1024 bytes. Both of these live in the ESP32's heap.
The ESP32 has 320 KB of RAM, so memory is not a concern for this project. On an Arduino Uno
(2 KB RAM) this code would not fit without significant modification.

---

## Extending the Code

To add Wi-Fi band annotations you could add a function that maps known channel numbers to
protocol labels:

- Channels 1, 6, 11 are the standard non-overlapping Wi-Fi channels (IEEE 802.11 b/g/n)
- Channels 0-26 in the Bluetooth frequency hopping range
- Channels 10-26 are used by Zigbee

You could then display an extra line on the OLED such as "Likely: Wi-Fi Ch 6" when channel
6 shows a high hit count. That logic would go inside displaySignal() after the frequency line.
