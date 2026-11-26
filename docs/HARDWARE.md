# Hardware

Bill of materials for the telemetry node and the phase 3 scanner. Ordered 30 Sep 2026 in three carts, all arrived by 5 Oct 2026: uamper.com for most parts, arduino.ua for the soldering kit and the USB cables, MiniComp for the Nucleo. Board markings are as printed on the silkscreen. The wiring gets filled in once the bench confirms it.

## Phase 1: telemetry node

| Part | Role | Bus | Qty | Status |
|------|------|-----|-----|--------|
| STM32 Nucleo-F411RE (MB1136 rev C) | MCU board with on-board ST-LINK, USB to the host | - | 1 | arrived |
| MPU-6050 breakout (GY-521, silk HW-123) | 6-axis IMU: 3-axis gyroscope and 3-axis accelerometer | I2C | 2, one spare | arrived |
| BMP280 breakout (GY-BM, 3 V, silk HW-611) | barometric pressure and temperature sensor | I2C | 2, one spare | arrived |
| Breadboard, 830 points | prototyping base | - | 1 | arrived |
| Dupont jumper wires, 20 cm, M-F | Nucleo pin headers to the breadboard | - | 40 | arrived |
| Dupont jumper wires, 20 cm, M-M | breadboard to breadboard, breadboard to the Nucleo's female headers | - | 30 | arrived |
| USB-A to mini-USB cable | one for the Nucleo, one for the breadboard power module; neither ships with a cable | - | 2 | arrived |

F-F wires were not bought: every run here starts at a pin header or at a breadboard hole.

## Phase 3: scanning rangefinder

| Part | Role | Bus | Qty | Status |
|------|------|-----|-----|--------|
| TF-Luna (Benewake), cable included | single-beam rangefinder, sweeps the room on the servo | UART or I2C, decided at M10 | 1 | arrived |
| MG90S servo, 180 degrees | pan sweep; metal gears for repeatable angles | PWM | 2, one spare | arrived |
| Pan-tilt bracket (PT Pan) | mounts the rangefinder on the servo | - | 1 | arrived |
| MB102 breadboard power module, mini-USB input | separate 5 V rail for the servo and the rangefinder, fed by a phone charger, so they do not load the Nucleo's USB; 500 mA max per the listing. Measured 5 Oct 2026: the left leg gives 5 V in both switch positions, the right leg gives 5 V or 3.3 V depending on the slide switch. The legs are wider than the breadboard's rail spacing, so the rail strips were slid apart to fit | - | 1 | arrived |

## Bench tools

| Tool | Role | Status |
|------|------|--------|
| Multimeter ZT98 | continuity, voltage | arrived |
| Soldering iron, 60 W with a power regulator, stand and five tips | pin headers on the breakouts | arrived |
| Solder 1.0 mm 60/40, 100 g, flux-cored; rosin 20 g | joints on the pin headers | arrived |
| USB logic analyzer, 24 MHz, 8 channels, 10 leads included | looking at the I2C and UART lines in PulseView when a bus goes quiet | arrived |

Host: a PC running the serial-to-MQTT bridge from phase 2, and a browser for the plots and, from phase 3, the live map.

## Wiring

Phase 1, the I2C bus. Confirmed on 16 Nov 2026, when the M3 bus scan found both sensors. The two breakouts sit on the breadboard and share four lines from the Nucleo:

| Line | Nucleo | MPU-6050 (HW-123) | BMP280 (HW-611) |
|------|--------|-------------------|-----------------|
| 3.3 V | CN6 pin 4, 3V3 | VCC | VCC |
| GND | CN6 pin 6 | GND | GND |
| SCL | CN5 pin 10, D15, PB8 (I2C1_SCL, AF4) | SCL | SCL |
| SDA | CN5 pin 9, D14, PB9 (I2C1_SDA, AF4) | SDA | SDA |

Both breakouts are powered from the Nucleo's 3V3 pin, not from 5 V. Measured with the multimeter: 3.31 to 3.32 V at the modules, SDA and SCL idle at about 3.31 V, no short between any two of the four lines.

The pull-ups are the ones on the breakouts; the MCU's internal pulls are off. The MPU-6050 board has 2.19 kΩ per line to its regulated rail, the BMP280 board about 9.97 kΩ per line to VCC. In parallel that is about 1.80 kΩ per line, about 1.84 mA through a line held low at 3.3 V.

Addresses, as the boards set them and as the scan confirmed:

| Sensor | Address | Address pins as measured |
|--------|---------|--------------------------|
| MPU-6050 | 0x68 | AD0 at 0 V, about 4.65 kΩ to GND on the board |
| BMP280 | 0x76 | SDO at about 0 V; CSB at about 3.31 V, which selects I2C. The silkscreen calls the SDO pin `SDD` |

The MPU-6050 board carries a 3.3 V regulator between VCC and the chip (the marking reads like MIC5219-3.3, not verified). Fed with 3.3 V it passes the rail through: its input and its output both measure 3.31 V. The pin headers, 8 pins on the MPU-6050 and 6 on the BMP280, were soldered by hand, and each pin was checked for continuity to its pad and against its neighbours.

Read back over the bus in M4, 23 Nov 2026: the identity register of the MPU-6050 (0x75) holds 0x68 and the id register of the BMP280 (0xD0) holds 0x58. On the MPU-6050 unit in use, at the ±2 g range where 16384 counts per g is nominal, the Z accelerometer axis averaged about 13100 counts with the breadboard face up and about -20700 face down. The two positions were set by hand, the cause is not established, and nothing is corrected. The spare MPU-6050 breakout has not been tried.

The USER button of the Nucleo, on PC13, became an input in M5: the firmware reads it once after reset to choose the boot mode. No pull is enabled in the MCU, and the pin reads high with the button released (read through the debugger on 27 Nov 2026), so the level comes from the board. Held during reset it reads low and selects the self-test mode (author's bench).

BMP280, read over the bus in M5 on 26 Nov 2026 and confirmed at the acceptance on 27 Nov. After the driver's init `config` (0xF5) reads back 0x28 and `ctrl_meas` (0xF4) reads back 0x57. The twelve calibration words of the unit in use, 24 bytes from 0x88, low byte first, T1 and P1 unsigned and the rest signed:

| T1 | T2 | T3 | P1 | P2 | P3 | P4 | P5 | P6 | P7 | P8 | P9 |
|----|----|----|----|----|----|----|----|----|----|----|----|
| 28492 | 26521 | 50 | 36606 | -10638 | 3024 | 7007 | -114 | -7 | 15500 | -14600 | 6000 |

Right after a reset both data triples read 0x80000. With the temperature measured and the pressure skipped, the first three bytes stay at 0x80000 and the last three move: the pressure is in 0xF7 to 0xF9 and the temperature in 0xFA to 0xFC, as the memory map of the datasheet says and against the labels in the figure of its section 3.12.

Barometer readings, a plausibility check and not a calibration. Author's bench, Kyiv, 26 Nov 2026: 100989.6 Pa at 20:19 over 60 samples, standard deviation about 1.4 Pa. The station value for Kyiv at 20:00 was 100200 Pa at 167 m; reduced by the author to an estimated bench height of 116 m it is 100843 Pa, which leaves the sensor 146.6 Pa above it. Neither the station value nor the reduction was checked independently, and no reference instrument was used. Raising the bench by hand and putting it back: 101030.63, 101020.11 and 101029.76 Pa, a drop of 10.08 Pa, where about 11.6 Pa per metre is expected near the ground. A finger on the BMP280 took its temperature from 24.01 to 27.85 °C while the MPU-6050 stayed between 25.2 and 25.5 °C. At the acceptance, 100 s at rest: 101068 Pa with a standard deviation of 1.8 Pa, 24.09 °C on the BMP280 and 25.7 °C on the MPU-6050.

MPU-6050 unit in use, M5, author's bench: five positions held by hand at the ±2 g range.

| Position, as the author labelled it | X | Y | Z |
|----------|---|---|---|
| level | 220 | 580 | 13380 |
| +X | 8580 | 540 | 11010 |
| -X | -6670 | 500 | 11540 |
| +Y | 260 | 4420 | 12750 |
| -Y | 80 | -3310 | 12940 |

The signs of X and Y follow gravity. At the nominal 16384 counts per g the five vectors are 0.81 to 0.85 g long. With the Z bias and scale that the two positions of M4 suggest, about -3780 counts and 16900 counts per g, they come to 0.99 to 1.02 g, so the short vector is the Z offset already seen in M4 and not a second effect; that estimate still rests on two positions set by hand. At rest, with the filter at the widest setting the driver writes, the accelerometer counts scatter with a standard deviation of about 80 and the gyroscope counts with 17 to 33. The gyroscope X axis rests at about -800 counts, -6 °/s, inside the ±20 °/s initial tolerance of the Product Specification (6.1). No gyroscope axis has been checked under a known rotation, and the spare breakout has still not been tried.

Phase 3 wiring (servo, rangefinder, separate supply) is recorded at M9 and M10.
