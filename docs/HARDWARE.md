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

Phase 3 wiring (servo, rangefinder, separate supply) is recorded at M9 and M10.
