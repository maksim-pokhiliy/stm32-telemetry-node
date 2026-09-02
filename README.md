# STM32 Telemetry Node

Bare-metal C telemetry node on an STM32 Nucleo-F411RE: MPU-6050 (IMU) and BMP280 (barometer) over I2C, telemetry streamed to a host over UART.

## Why

Seven-plus years as a software engineer on web products, including real-time device control over MQTT, none of it on the firmware side. I am moving into embedded, and this repository is where that happens in public: one microcontroller, two sensors, one serial link, written from scratch while I learn C and the STM32 platform. An IMU and a barometer are the sensor pair flight controllers start from; this project stops at reading them and streaming the data.

## Status

In progress, started Sep 2026. The hardware is ordered and in transit. The C core of Harvard CS50x runs in parallel and is due mid-Oct 2026; working telemetry is expected late Oct 2026.

All firmware code in this repo is hand-written as part of the learning process (this repo doubles as my C learning log); AI is used for study and review, not code generation.

## Architecture

```
+------------+     +------------+
|  MPU-6050  |     |   BMP280   |
|    IMU     |     | barometer  |
+-----+------+     +-----+------+
      |                  |
      +--------+---------+
               |  I2C
               v
   +-----------+-----------+
   |   Nucleo-F411RE       |
   |   bare-metal C        |
   +-----------+-----------+
               |  UART
               v
   +-----------+-----------+
   |   host                |
   |   serial terminal     |
   +-----------------------+
```

Telemetry frame format: TBD. The choice and its reasoning go into [docs/DECISIONS.md](docs/DECISIONS.md) when M6 starts.

Scope: bare-metal C, I2C, UART. Out of scope: RTOS, SPI, flight control.

## Roadmap

- [x] M0: repo, plan, hardware ordered (Sep 2026)
- [ ] M1: toolchain bring-up + LED blink
- [ ] M2: UART "hello" to host
- [ ] M3: I2C bus scan, sensors detected
- [ ] M4: MPU-6050 readout
- [ ] M5: BMP280 readout
- [ ] M6: framed telemetry stream to host (ETA late Oct 2026)

## Hardware and toolchain

- STM32 Nucleo-F411RE
- MPU-6050 IMU (I2C)
- BMP280 barometer (I2C)
- breadboard and jumper wires

Bill of materials and wiring, filled in as the parts arrive: [docs/HARDWARE.md](docs/HARDWARE.md).

Planned toolchain: arm-none-eabi-gcc, make, gdb, st-link. Nothing is set up yet; M1 is where that happens.

## License

MIT
