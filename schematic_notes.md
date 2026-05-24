# Schematic Notes

This file describes every electrical connection in the project at the pin level.
Use this as a reference when drawing a schematic in KiCad, EasyEDA, or Fritzing.

---

## Component List

U1: ESP32 development board (38-pin or 30-pin, any variant with exposed GPIO 4, 5, 18, 19, 21, 22, 23)
U2: nRF24L01+ module (8-pin, 2x4 header, 2.54mm pitch)
U3: SSD1306 OLED module (4-pin I2C variant, 2.54mm pitch)
C1: Electrolytic capacitor, 10-100 uF, 6.3V or higher rating

---

## Power Net

3V3 net: U1 pin 3V3 --> U2 pin 2 (VCC) --> U3 pin 2 (VCC) --> C1 positive leg
GND net: U1 pin GND --> U2 pin 1 (GND) --> U3 pin 1 (GND) --> C1 negative leg

Note: C1 should be placed physically close to U2. It is a bypass capacitor, not a bulk filter;
its purpose is to supply the current spikes from the nRF24L01 without drooping the 3V3 net.

---

## SPI Bus (U1 to U2)

Signal   U1 pin     U2 pin    Note
------   ------     ------    ----
SCK      GPIO 18    pin 5     SPI clock, driven by ESP32
MOSI     GPIO 23    pin 6     Master Out Slave In
MISO     GPIO 19    pin 7     Master In Slave Out
CE       GPIO 4     pin 3     Chip Enable (RX/TX control), driven by ESP32
CSN      GPIO 5     pin 4     Chip Select Not (active low), driven by ESP32
IRQ      not used   pin 8     Interrupt output from nRF24L01, left floating

There are no pull-up or pull-down resistors needed on these lines for normal operation.
The nRF24L01 SPI interface supports mode 0 (CPOL=0, CPHA=0) at up to 10 MHz.
The RF24 library defaults to a lower clock speed that works reliably on breadboard wiring.

---

## I2C Bus (U1 to U3)

Signal   U1 pin     U3 pin    Note
------   ------     ------    ----
SDA      GPIO 21    pin 4     I2C data, bidirectional
SCL      GPIO 22    pin 3     I2C clock, driven by ESP32

The SSD1306 breakout board includes its own 4.7k pull-up resistors on SDA and SCL.
If you are building a custom PCB without a breakout board, add 4.7k pull-ups from SDA
and SCL to the 3V3 net.

I2C address: 0x3C (default on most breakout boards)
If the address select resistor on the board is moved to the other pad, address becomes 0x3D.

---

## Net Summary Table

Net name    Connected pins
--------    --------------
3V3         U1:3V3, U2:VCC(2), U3:VCC(2), C1:+
GND         U1:GND, U2:GND(1), U3:GND(1), C1:-
SCK         U1:GPIO18, U2:SCK(5)
MOSI        U1:GPIO23, U2:MOSI(6)
MISO        U1:GPIO19, U2:MISO(7)
NRF_CE      U1:GPIO4, U2:CE(3)
NRF_CSN     U1:GPIO5, U2:CSN(4)
I2C_SDA     U1:GPIO21, U3:SDA(4)
I2C_SCL     U1:GPIO22, U3:SCL(3)

---

## Electrical Characteristics

nRF24L01+ supply voltage: 1.9V to 3.6V (3.3V nominal)
nRF24L01+ IO voltage: 0V to VCC + 0.3V (3.3V logic only, not 5V tolerant)
SSD1306 supply voltage: 1.65V to 3.3V (3.3V typical with internal charge pump for OLED bias)
ESP32 IO voltage: 3.3V; all IO pins are 3.3V; not 5V tolerant

All three devices are 3.3V native. No level shifters are required.

---

## PCB Layout Recommendations (for custom board)

1. Place C1 as close to U2 VCC and GND pins as possible. Use a short trace, not a long run.
2. Route SCK, MOSI, MISO as a matched-length group with ground pours on both sides.
3. Keep the nRF24L01 antenna area clear of ground planes on both sides of the PCB within 5mm.
4. Place a ground via beside each SPI signal pin on U2 to reduce return path inductance.
5. The SDA and SCL traces can be longer without issues; I2C at 400 kHz is not sensitive to trace length.
