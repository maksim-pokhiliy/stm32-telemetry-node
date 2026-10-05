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

TBD. Recorded here once the parts arrive and the I2C bus scan (M3) sees both sensors. Phase 3 wiring (servo, rangefinder, separate supply) is recorded at M9 and M10.
