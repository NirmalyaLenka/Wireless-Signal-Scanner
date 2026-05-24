# Hardware Explanation

This document explains what each hardware component does, how it works internally, and why
it was chosen for this project. Understanding the hardware makes it much easier to debug
wiring problems and to make informed modifications.

---

## The ESP32 Microcontroller

The ESP32 is a 32-bit microcontroller made by Espressif Systems. It runs at up to 240 MHz,
has 520 KB of internal SRAM, and includes hardware support for SPI, I2C, UART, PWM, and
many other peripherals. For this project the important capabilities are:

Hardware SPI: The ESP32 has dedicated SPI controller hardware that can transfer data to the
nRF24L01 much faster and with less CPU overhead than bit-banged (software) SPI. The default
hardware SPI pins on most ESP32 development boards are GPIO 18 (clock), GPIO 19 (MISO),
and GPIO 23 (MOSI).

Hardware I2C: Similarly, the ESP32 has a hardware I2C controller on GPIO 21 (SDA) and GPIO
22 (SCL) by default. The OLED uses these pins.

3.3 V output: The ESP32's 3.3 V regulator output is sufficient to power both the nRF24L01
and the SSD1306. Both modules require 3.3 V logic and supply voltage, so connecting them
directly to the ESP32's 3.3 V pin works correctly. If you use a 5 V Arduino instead you
would need level shifters for every signal line.

GPIO flexibility: CE and CSN for the nRF24L01 can be assigned to almost any GPIO pin. We use
GPIO 4 and GPIO 5 in this project because they are available on virtually all ESP32 development
boards and are not used by any of the built-in peripheral hardware.

---

## The nRF24L01+ Radio Module

The nRF24L01+ is a single-chip radio transceiver made by Nordic Semiconductor. It operates
in the 2.4 GHz ISM band, which is the same unlicensed spectrum used by Wi-Fi, Bluetooth,
Zigbee, and many other consumer wireless devices.

The chip communicates with a host microcontroller over SPI. Commands and register values are
sent over SPI; data payloads are also transferred over SPI. The chip supports up to 2 Mbps
data rate in the 2.4 GHz band, auto-acknowledgement, six independent receive pipes, and a
number of power-saving modes.

For this project we use none of those communication features. We only use one register: RPD.

The RPD Register (Received Power Detector):

RPD stands for Received Power Detector. It is a 1-bit register that the nRF24L01 hardware
sets to 1 automatically whenever it detects an RF carrier above approximately -64 dBm on the
currently tuned channel while in receive mode. The bit clears when you change channel or
when the radio exits receive mode.

-64 dBm is a moderate signal level. For reference, a Wi-Fi access point 3 meters away might
produce -50 to -60 dBm at your location, which is comfortably above the threshold. A device
10 or 15 meters away through walls might produce -75 to -80 dBm, which would be below the
threshold and therefore not detected. The RPD is a proximity detector, not a long-range
spectrum analyser.

Channel Spacing:

The nRF24L01+ channel register maps each integer value (0 through 127) to a 1 MHz step
above 2400 MHz. Channel 0 = 2400 MHz, channel 1 = 2401 MHz, and so on up to channel 127 =
2527 MHz. This gives coverage of the entire 2.4 GHz ISM band (2400-2483.5 MHz) plus some
margin above it.

The + Variant:

If you have a choice, use the nRF24L01+ (with a plus sign) rather than the original
nRF24L01. The + version has improved sensitivity and a wider RPD detection range. Both are
electrically pin-compatible and use the same library.

Power Supply Sensitivity:

The nRF24L01 is sensitive to power supply noise. When it transmits or switches modes it draws
brief current spikes that can cause the 3.3 V rail to droop momentarily. This confuses the
SPI logic and causes the radio.begin() call to fail or causes random errors during operation.
The fix is to solder a 10 to 100 uF electrolytic capacitor directly across the VCC and GND
pins of the module, as close to the module as possible. This capacitor acts as a local charge
reservoir that supplies those current spikes without drooping the rail. This is the single
most common reason nRF24L01 modules misbehave on breadboards.

---

## The SSD1306 OLED Display

The SSD1306 is a controller chip made by Solomon Systech. It drives a matrix of organic LED
pixels. At 0.96 inches diagonal with 128 columns and 64 rows of pixels, each pixel is about
0.1 mm across. The display is monochrome (each pixel is either on or off at full brightness).

The SSD1306 can communicate over either SPI or I2C depending on how the breakout board is
configured. Most small breakout boards with 4 pins (GND, VCC, SCL, SDA) use I2C. We use I2C
here because it requires only 2 signal wires shared with other I2C devices, whereas SPI would
need 4 wires dedicated to the display alone.

The Adafruit SSD1306 library maintains a pixel frame buffer in the ESP32's RAM. You draw
into this buffer using drawing functions from the Adafruit_GFX library (lines, rectangles,
circles, text at multiple sizes). None of those drawing operations affect the actual display
hardware. Only when you call `display.display()` does the driver send the entire frame buffer
over I2C to the SSD1306 controller, which then lights the appropriate pixels.

This buffered approach has two consequences. First, you will never see partial or flickering
updates because the transfer to hardware happens in one complete block. Second, if you forget
to call `display.display()` nothing changes on screen no matter how much you draw.

Pixel Coordinate System:

The origin (0, 0) is at the top-left corner of the display. X increases to the right. Y
increases downward. The bottom-right pixel is at (127, 63). Text drawn with setTextSize(1)
uses a 6x8 pixel font; each character is 6 pixels wide and 8 pixels tall, so a full row of
size-1 text can fit 21 characters. Size-2 text doubles both dimensions to 12x16 pixels,
fitting about 10 characters per row.

I2C Address:

The SSD1306 has a configurable I2C address. Most breakout boards pull the address select pin
to ground, giving address 0x3C. If your display does not respond, probe it with an I2C
scanner sketch to find its actual address and update OLED_I2C_ADDR in the source code.

---

## How the Three Components Work Together

The ESP32 acts as the master controlling both peripherals. The RF24 library handles the SPI
communication protocol to the nRF24L01: it sends commands to tune the channel, enables
receive mode, reads the RPD register, and disables receive mode, all transparently through
SPI calls. The Adafruit_SSD1306 library handles the I2C communication to the display: it
sends command bytes to control the display hardware and data bytes to fill the pixel buffer.

From the firmware's perspective the hardware is invisible. The code calls
`radio.setChannel(ch)` and `radio.testRPD()` without caring that these involve SPI byte
exchanges; it calls `display.setCursor()` and `display.println()` without caring that these
write into a RAM buffer that will later be pushed over I2C. The libraries abstract the
hardware communication completely.

---

## Power Budget

Typical current draw at 3.3 V:

- ESP32 active (running at 240 MHz): 60-80 mA
- nRF24L01+ in RX mode: 12 mA
- SSD1306 OLED (half pixels on): 15-20 mA

Total estimated draw: approximately 90-110 mA at 3.3 V.

When powered from a USB port the ESP32's onboard 3.3 V LDO regulator is supplied from the
USB 5 V line. A standard USB port provides 500 mA, so the power budget is well within limits.
A USB power bank works fine for portable use.

---

## Breadboard Assembly Tips

1. Wire the nRF24L01 first, before connecting power. Double-check that VCC goes to 3.3 V and
   not 5 V before applying power. Overvoltage will permanently destroy the module.

2. Keep the nRF24L01 wires short. Long SPI wires act as antennas and pick up noise, causing
   communication errors at higher SPI speeds. On a breadboard try to place the module close
   to the ESP32.

3. The OLED can share the I2C bus with other devices if needed. I2C is a multi-device bus;
   the SSD1306 is addressed by its 7-bit address (0x3C) and ignores traffic addressed to
   other devices.

4. If you use a bare nRF24L01 chip without an antenna module, the chip has a tiny onboard
   PCB trace antenna. Orient it away from other components and away from the ESP32's RF
   circuitry for best results. The PA+LNA breakout modules with an external SMA antenna
   have significantly longer range but draw more current.

---

## Schematic Reference

See `schematics/schematic_notes.md` for a detailed text description of every connection.
See `hardware/wiring_diagram.txt` for an ASCII diagram of the physical breadboard layout.
