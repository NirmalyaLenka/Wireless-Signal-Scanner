/*
  wireless_scanner.ino

  A wireless signal scanner built on ESP32 with an nRF24L01 radio module
  and a 0.96-inch SSD1306 OLED display. The device continuously scans
  all 128 RF channels in the 2.4 GHz band using the nRF24L01 carrier
  detect feature, then displays each detected signal one by one on the
  OLED screen with its channel number and signal strength indicator.

  Author  : Your Name
  License : MIT
  Date    : 2024
*/

#include <SPI.h>
#include <Wire.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------------------------------------------------------------------
// Hardware pin definitions
// ---------------------------------------------------------------------------

// nRF24L01 chip-select and chip-enable pins connected to the ESP32
#define NRF_CE_PIN   4
#define NRF_CSN_PIN  5

// SSD1306 OLED display dimensions (change to 128x64 if you use the bigger
// variant; nothing else in the code needs updating)
#define OLED_WIDTH   128
#define OLED_HEIGHT   64

// I2C address of the SSD1306. Most breakout boards use 0x3C; a few use 0x3D.
#define OLED_I2C_ADDR 0x3C

// ---------------------------------------------------------------------------
// RF scan parameters
// ---------------------------------------------------------------------------

// The nRF24L01 supports channels 0-127, each 1 MHz apart starting at 2.400 GHz
#define RF_CHANNEL_COUNT 128

// How many times we sample each channel before declaring it "active".
// Higher values reduce false positives but slow down the full scan cycle.
#define SAMPLES_PER_CHANNEL 64

// A channel is considered active when at least this many samples return a
// carrier-detect hit. Tune this threshold for your RF environment.
#define DETECTION_THRESHOLD 2

// Delay in milliseconds between showing each detected signal on the OLED
#define DISPLAY_HOLD_MS 1800

// Delay after a full scan cycle before starting the next one
#define INTER_SCAN_DELAY_MS 300

// ---------------------------------------------------------------------------
// Global objects
// ---------------------------------------------------------------------------

// RF24 object using hardware SPI bus (MOSI/MISO/SCK wired per board notes)
RF24 radio(NRF_CE_PIN, NRF_CSN_PIN);

// OLED display object; -1 means we are not using a hardware reset pin
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// Stores the hit count for each of the 128 channels after one scan sweep
uint8_t channelHits[RF_CHANNEL_COUNT];

// ---------------------------------------------------------------------------
// Function declarations
// ---------------------------------------------------------------------------

void setupRadio();
void setupDisplay();
void scanAllChannels();
void displaySignal(uint8_t channel, uint8_t hits);
void displayIdleScreen(uint8_t totalFound);
uint8_t mapHitsToBar(uint8_t hits);

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  Serial.println("Wireless Scanner starting...");

  setupDisplay();
  setupRadio();
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------

void loop() {
  // Zero out all channel counters before a fresh sweep
  memset(channelHits, 0, sizeof(channelHits));

  // Perform the full carrier-detect sweep across all 128 channels
  scanAllChannels();

  // Count how many distinct channels crossed the detection threshold
  uint8_t totalFound = 0;
  for (uint8_t ch = 0; ch < RF_CHANNEL_COUNT; ch++) {
    if (channelHits[ch] >= DETECTION_THRESHOLD) {
      totalFound++;
    }
  }

  // If nothing was found this round, show a brief "no signals" message and
  // go back to scanning immediately
  if (totalFound == 0) {
    displayIdleScreen(0);
    delay(INTER_SCAN_DELAY_MS);
    return;
  }

  // Walk through every channel and display each active one individually
  for (uint8_t ch = 0; ch < RF_CHANNEL_COUNT; ch++) {
    if (channelHits[ch] >= DETECTION_THRESHOLD) {
      displaySignal(ch, channelHits[ch]);
      delay(DISPLAY_HOLD_MS);
    }
  }

  // Brief pause at the end of a full display cycle, then scan again
  delay(INTER_SCAN_DELAY_MS);
}

// ---------------------------------------------------------------------------
// setupRadio()
//
// Initialises the nRF24L01 and puts it into constant carrier monitoring mode.
// We do not want to receive actual packet data here; we only need the
// hardware-level carrier-detect bit (rpd register) that the chip exposes.
// ---------------------------------------------------------------------------

void setupRadio() {
  if (!radio.begin()) {
    Serial.println("ERROR: nRF24L01 not found. Check wiring.");
    // Show a visible error on the OLED so the user knows immediately
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 20);
    display.println("nRF24L01 not found");
    display.println("Check wiring!");
    display.display();
    // Halt here; there is no point continuing without the radio
    while (true) { delay(1000); }
  }

  // Low power mode is sufficient for passive scanning
  radio.setPALevel(RF24_PA_MIN);

  // Widest possible bandwidth to maximise carrier-detect sensitivity
  radio.setDataRate(RF24_2MBPS);

  // Auto-acknowledgement off; we are not exchanging packets
  radio.setAutoAck(false);

  // Disable dynamic payload and all pipe-level features we do not need
  radio.disableDynamicPayloads();

  Serial.println("nRF24L01 initialised OK");
}

// ---------------------------------------------------------------------------
// setupDisplay()
//
// Initialises the SSD1306 OLED over I2C and shows a short splash screen.
// ---------------------------------------------------------------------------

void setupDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println("ERROR: SSD1306 OLED not found. Check I2C wiring.");
    // Cannot show anything on screen; just halt and log to serial
    while (true) { delay(1000); }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // --- Splash screen ---
  display.setTextSize(1);
  display.setCursor(20, 10);
  display.println("Wireless Scanner");

  display.setTextSize(1);
  display.setCursor(10, 30);
  display.println("2.4 GHz Band Monitor");

  display.setCursor(28, 50);
  display.println("Initialising...");

  display.display();
  delay(2000);

  Serial.println("OLED display initialised OK");
}

// ---------------------------------------------------------------------------
// scanAllChannels()
//
// Iterates over every RF channel from 0 to 127. For each channel the radio
// is tuned and then sampled SAMPLES_PER_CHANNEL times. The nRF24L01 RPD
// (Received Power Detector) register reads 1 when it senses a carrier above
// -64 dBm. We count how many of those SAMPLES_PER_CHANNEL reads return a hit
// and store the count in channelHits[].
// ---------------------------------------------------------------------------

void scanAllChannels() {
  // Switch the radio to RX mode so the RPD register is active
  radio.startListening();

  for (uint8_t ch = 0; ch < RF_CHANNEL_COUNT; ch++) {
    radio.setChannel(ch);

    uint8_t hits = 0;
    for (uint8_t s = 0; s < SAMPLES_PER_CHANNEL; s++) {
      // A tiny dwell time lets the LNA settle on the new frequency
      delayMicroseconds(130);

      if (radio.testRPD()) {
        hits++;
      }
    }
    channelHits[ch] = hits;
  }

  // Return the radio to standby so it does not consume power needlessly
  radio.stopListening();
}

// ---------------------------------------------------------------------------
// displaySignal()
//
// Renders a single detected channel on the OLED. The layout is:
//
//   Line 1:  "SIGNAL DETECTED"      (header, text size 1)
//   Line 2:  "CH: <channel>"        (channel number, text size 2)
//   Line 3:  "Freq: <MHz>"          (human-readable frequency)
//   Line 4:  bar graph              (visual strength indicator)
//   Line 5:  "Strength: X/64"       (numeric hit count out of max samples)
//
// ---------------------------------------------------------------------------

void displaySignal(uint8_t channel, uint8_t hits) {
  // Calculate frequency: nRF24L01 channel 0 = 2400 MHz, step = 1 MHz
  uint16_t freqMHz = 2400 + channel;

  display.clearDisplay();

  // Header
  display.setTextSize(1);
  display.setCursor(22, 0);
  display.println("SIGNAL DETECTED");

  // Divider line under the header
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Channel number in large text
  display.setTextSize(2);
  display.setCursor(0, 14);
  display.print("CH: ");
  display.println(channel);

  // Frequency in small text
  display.setTextSize(1);
  display.setCursor(0, 34);
  display.print("Freq: ");
  display.print(freqMHz);
  display.println(" MHz");

  // Strength bar: 10 segments, each segment = roughly 6-7 hits out of 64
  display.setCursor(0, 46);
  display.print("Sig: [");
  uint8_t bars = mapHitsToBar(hits);
  for (uint8_t b = 0; b < 10; b++) {
    if (b < bars) {
      display.print("#");
    } else {
      display.print(".");
    }
  }
  display.print("]");

  // Numeric hit count for precision
  display.setCursor(0, 56);
  display.print("Hits: ");
  display.print(hits);
  display.print("/");
  display.print(SAMPLES_PER_CHANNEL);

  display.display();

  // Also log to serial so anyone with a USB cable can follow along
  Serial.print("Channel: ");
  Serial.print(channel);
  Serial.print("  Freq: ");
  Serial.print(freqMHz);
  Serial.print(" MHz  Hits: ");
  Serial.println(hits);
}

// ---------------------------------------------------------------------------
// displayIdleScreen()
//
// Shown when no channels exceeded the detection threshold in a full sweep.
// ---------------------------------------------------------------------------

void displayIdleScreen(uint8_t totalFound) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(22, 0);
  display.println("SIGNAL DETECTED");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(10, 22);
  display.println("Scanning 2.4 GHz...");

  display.setCursor(20, 38);
  display.println("No signals found");

  display.setCursor(28, 52);
  display.println("Retrying...");

  display.display();

  Serial.println("Scan complete: no active channels detected");
}

// ---------------------------------------------------------------------------
// mapHitsToBar()
//
// Converts a raw hit count (0-64) into a bar graph segment count (0-10).
// Uses a simple linear mapping: bar = (hits / SAMPLES_PER_CHANNEL) * 10
// clamped to [0, 10].
// ---------------------------------------------------------------------------

uint8_t mapHitsToBar(uint8_t hits) {
  if (hits == 0) return 0;
  uint8_t bar = (uint8_t)(((uint16_t)hits * 10) / SAMPLES_PER_CHANNEL);
  if (bar > 10) bar = 10;
  if (bar < 1) bar = 1; // Always show at least one bar for detected signals
  return bar;
}
