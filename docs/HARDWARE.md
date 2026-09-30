# Hardware

Bill of materials for the telemetry node and the phase 3 scanner. Ordered 30 Sep 2026 in three carts, in transit: uamper.com for most parts, arduino.ua for the soldering kit and the USB cables, MiniComp for the Nucleo. Exact breakout-board revisions and the wiring get filled in when the parts arrive.

## Phase 1: telemetry node

| Part | Role | Bus | Qty | Status |
|------|------|-----|-----|--------|
| STM32 Nucleo-F411RE | MCU board with on-board ST-LINK, USB to the host | - | 1 | ordered |
| MPU-6050 breakout (GY-521) | 6-axis IMU: 3-axis gyroscope and 3-axis accelerometer | I2C | 2, one spare | ordered |
| BMP280 breakout (GY-BM, 3 V) | barometric pressure and temperature sensor | I2C | 2, one spare | ordered |
| Breadboard, 830 points | prototyping base | - | 1 | ordered |
| Dupont jumper wires, 20 cm, M-F | Nucleo pin headers to the breadboard | - | 40 | ordered |
| Dupont jumper wires, 20 cm, M-M | breadboard to breadboard, breadboard to the Nucleo's female headers | - | 30 | ordered |
| USB-A to mini-USB cable | one for the Nucleo, one for the breadboard power module; neither ships with a cable | - | 2 | ordered |

F-F wires were not bought: every run here starts at a pin header or at a breadboard hole.

## Phase 3: scanning rangefinder

| Part | Role | Bus | Qty | Status |
|------|------|-----|-----|--------|
| TF-Luna (Benewake), cable included | single-beam rangefinder, sweeps the room on the servo | UART or I2C, decided at M10 | 1 | ordered |
| MG90S servo, 180 degrees | pan sweep; metal gears for repeatable angles | PWM | 2, one spare | ordered |
| Pan-tilt bracket (PT Pan) | mounts the rangefinder on the servo | - | 1 | ordered |
| MB102 breadboard power module, mini-USB input | separate 5 V rail for the servo and the rangefinder, fed by a phone charger, so they do not load the Nucleo's USB; 500 mA max per the listing | - | 1 | ordered |

## Bench tools

| Tool | Role | Status |
|------|------|--------|
| Multimeter ZT98 | continuity, voltage | ordered |
| Soldering iron, 60 W with a power regulator, stand and five tips | pin headers on the breakouts | ordered |
| Solder 1.0 mm 60/40, 100 g, flux-cored; rosin 20 g | joints on the pin headers | ordered |
| USB logic analyzer, 24 MHz, 8 channels, 10 leads included | looking at the I2C and UART lines in PulseView when a bus goes quiet | ordered |

Host: a PC running the serial-to-MQTT bridge from phase 2, and a browser for the plots and, from phase 3, the live map.

## Wiring

TBD. Recorded here once the parts arrive and the I2C bus scan (M3) sees both sensors. Phase 3 wiring (servo, rangefinder, separate supply) is recorded at M9 and M10.
