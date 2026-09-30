# Hardware

Bill of materials for the telemetry node and the phase 3 scanner. Ordered Sep 2026, in transit. Exact breakout-board revisions and the wiring get filled in when the parts arrive.

## Phase 1: telemetry node

| Part | Role | Bus | Qty | Status |
|------|------|-----|-----|--------|
| STM32 Nucleo-F411RE | MCU board with on-board ST-LINK, USB to the host | - | 1 | ordered |
| MPU-6050 breakout (GY-521) | 6-axis IMU: 3-axis gyroscope and 3-axis accelerometer | I2C | 2, one spare | ordered |
| BMP280 breakout | barometric pressure and temperature sensor | I2C | 2, one spare | ordered |
| Breadboard, 830 points | prototyping base | - | 1 | ordered |
| Dupont jumper wires, 20 cm, M-M, M-F, F-F | connections between the boards | - | 30 of each | ordered |

## Phase 3: scanning rangefinder

| Part | Role | Bus | Qty | Status |
|------|------|-----|-----|--------|
| TF-Luna | single-beam rangefinder, sweeps the room on the servo | UART or I2C, decided at M10 | 1 | ordered |
| MG90S servo | pan sweep; metal gears for repeatable angles | PWM | 2, one spare | ordered |
| Pan-tilt bracket | mounts the rangefinder on the servo | - | 1 | ordered |
| MB102 breadboard power module | separate 5 V rail for the servo and the rangefinder, so they do not load the Nucleo's USB | - | 1 | ordered |

## Bench tools

| Tool | Role | Status |
|------|------|--------|
| Multimeter ZT101 | continuity, voltage, current | ordered |
| Soldering iron FNIRSI HS-01, solder, flux | pin headers on the breakouts | ordered |
| USB logic analyzer, 24 MHz, 8 channels | looking at the I2C and UART lines in PulseView when a bus goes quiet | ordered |

Host: a PC running the serial-to-MQTT bridge from phase 2, and a browser for the plots and, from phase 3, the live map.

## Wiring

TBD. Recorded here once the parts arrive and the I2C bus scan (M3) sees both sensors. Phase 3 wiring (servo, rangefinder, separate supply) is recorded at M9 and M10.
