# STM32 Telemetry Node

Bare-metal C telemetry node on an STM32 Nucleo-F411RE: MPU-6050 (IMU) and BMP280 (barometer) over I2C, telemetry streamed to a host over UART. The host side, a serial-to-MQTT bridge and a web dashboard, lives in a separate repo. Phase 3 puts a TF-Luna rangefinder on a servo, sweeps the room and draws a live map in that dashboard.

## Why

Seven-plus years as a software engineer on web products, including real-time device control over MQTT, none of it on the firmware side. I am moving into embedded, and this repository is where that happens in public: one microcontroller, two sensors, one serial link, written from scratch while I learn C and the STM32 platform. An IMU and a barometer are the sensor pair flight controllers start from; phase 1 stops at reading them and streaming the data. Phase 2 puts that data in a browser, which is the stack I already know. Phase 3 adds a scanning rangefinder, because a map drawing itself in a browser is a better demo than a column of numbers.

## Status

In progress, started Sep 2026. The hardware is ordered and in transit. The C core of Harvard CS50x runs in parallel and is due mid-Oct 2026; working telemetry is expected late Oct 2026. Phases 2 and 3 follow in order; phase N+1 is discussed after phase N ships.

All firmware code in this repo is hand-written as part of the learning process (this repo doubles as my C learning log); AI is used for study and review, not code generation.

## Architecture

```
+------------+   +------------+   +------------+   +------------+
|  MPU-6050  |   |   BMP280   |   |  TF-Luna   |   |   MG90S    |
|    IMU     |   | barometer  |   | rangefinder|   | pan servo  |
+-----+------+   +-----+------+   +-----+------+   +-----+------+
      |                |                |                ^
      +-------+--------+                | UART or I2C    | PWM
              | I2C                     | (M10)          | (M9)
              v                         v                |
   +----------+-------------------------+----------------+----+
   |   Nucleo-F411RE, bare-metal C (this repo)                |
   |   phase 1: telemetry, phase 3: scanner                   |
   +----------------------------+-----------------------------+
                                | UART
                                v
   +----------------------------+-----------------------------+
   |   host PC: serial terminal (M2-M6),                      |
   |   bridge from UART frames to MQTT (M7, host repo)        |
   +----------------------------+-----------------------------+
                                | MQTT over WebSockets, managed broker
                                v
   +----------------------------+-----------------------------+
   |   browser: live plots (M8), live map (M11),              |
   |   replay of a recorded session while the board is off    |
   +----------------------------------------------------------+
```

Telemetry frame format: TBD. The choice and its reasoning go into [docs/DECISIONS.md](docs/DECISIONS.md) when M6 starts; the format is the contract between this repo and the host repo. The rangefinder interface (UART or I2C) is decided at M10, same place.

The board has no network interface and the firmware gets no network stack: its link to the world is the UART. The PC is the gateway. It runs the bridge that publishes frames to a managed MQTT broker, and the browser subscribes over WebSockets. The public dashboard replays a recorded session while the board is off and switches to live when it is on ([D-005](docs/DECISIONS.md)).

Scope: bare-metal C, I2C, UART, timers/PWM. Out of scope: RTOS, SPI, flight control, networking on the board, a mobile platform. The scanner stays on a static mount: no odometry, no SLAM, reasons in [D-002](docs/DECISIONS.md).

## Roadmap

Phase 1, telemetry node (this repo):

- [x] M0: repo, plan, hardware ordered (Sep 2026)
- [ ] M1: toolchain bring-up + LED blink
- [ ] M2: UART "hello" to host
- [ ] M3: I2C bus scan, sensors detected
- [ ] M4: MPU-6050 readout
- [ ] M5: BMP280 readout
- [ ] M6: framed telemetry stream to host (ETA late Oct 2026)

Phase 2, host side (separate repo, linked here once it exists):

- [ ] M7: frame parser and serial-to-MQTT bridge, built on dumps recorded at M6
- [ ] M8: live plots in the browser, replay of a recorded session when the board is off

Phase 3, scanning rangefinder (firmware here, map in the phase 2 dashboard):

- [ ] M9: servo sweep (timers, PWM)
- [ ] M10: TF-Luna readout
- [ ] M11: live map of the room in the dashboard

## Hardware and toolchain

- STM32 Nucleo-F411RE
- MPU-6050 IMU (I2C)
- BMP280 barometer (I2C)
- TF-Luna rangefinder on an MG90S pan servo (phase 3)
- breadboard, jumper wires, a separate 5 V supply for the servo

Bill of materials, bench tools and wiring, filled in as the parts arrive: [docs/HARDWARE.md](docs/HARDWARE.md).

Planned toolchain: arm-none-eabi-gcc, make, gdb, st-link. Nothing is set up yet; M1 is where that happens.

## License

MIT
