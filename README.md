# STM32 Telemetry Node

Bare-metal C telemetry node on an STM32 Nucleo-F411RE: MPU-6050 (IMU) and BMP280 (barometer) over I2C, telemetry streamed to a host over UART. The host side, a serial-to-MQTT bridge and a web dashboard, lives in a separate repo. Phase 3 puts a TF-Luna rangefinder on a servo, sweeps the room and draws a live map in that dashboard.

## Why

Seven-plus years as a software engineer on web products, including real-time device control over MQTT, none of it on the firmware side. I am moving into embedded, and this repository is where that happens in public: one microcontroller, two sensors, one serial link, written from scratch while I learn C and the STM32 platform. An IMU and a barometer are the sensor pair flight controllers start from; phase 1 stops at reading them and streaming the data. Phase 2 puts that data in a browser, which is the stack I already know. Phase 3 adds a scanning rangefinder, because a map drawing itself in a browser is a better demo than a column of numbers.

## Status

In progress, started Sep 2026. The hardware arrived on 5 Oct 2026. M1 closed on 24 Oct 2026: the toolchain runs in WSL2 and LD2 on PA5 blinks from a hand-written linker script, startup code and register definitions. M2 closed on 28 Oct 2026: the board reports its uptime once a second over the ST-LINK virtual COM port, timed by a SysTick millisecond tick. M3 closed on 16 Nov 2026: pin headers soldered, both sensors wired to I2C1 and found by a bus scan, the MPU-6050 at 0x68 and the BMP280 at 0x76. M4 closed on 23 Nov 2026: the MPU-6050 is read ten times a second, seven raw values per sample, and the reads resume on their own after the SDA wire is pulled and put back; of the six axes only the accelerometer Z has been checked against gravity. M5 closed on 27 Nov 2026: the BMP280 is read ten times a second next to the MPU-6050 and compensated in its driver to hundredths of a degree and to pascals, the startup file is split into modules, the bench self-test runs only when the USER button is held at reset, and a sensor that holds SDA low after a reset is clocked free; the gyroscope has still not been checked under rotation. Harvard CS50x is paused until M6 ships and is not run alongside this project. Working telemetry is expected late Nov 2026. Phases 2 and 3 follow in order; phase N+1 is discussed after phase N ships.

All firmware code in this repo is hand-written as part of the learning process (this repo doubles as my C learning log); AI is used for study and review, not code generation. One footnote, recorded in the M2 commit: after the first UART evening I let an AI rename identifiers and trim comments in two files that already worked, `src/startup.c` and `src/stm32f411_regs.h`; every register value and every line of logic is mine. A second footnote, on dates: the sixteen firmware commits of M5 carry the day the milestone was accepted, 27 Nov 2026, because they were cut that day from the editor's saved versions of 24 to 26 Nov, each at a state that builds without warnings; every commit body names the moment its tree was saved.

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
- [x] M1: toolchain bring-up + LED blink (24 Oct 2026)
- [x] M2: UART "hello" to host (28 Oct 2026)
- [x] M3: I2C bus scan, sensors detected (16 Nov 2026)
- [x] M4: MPU-6050 readout (23 Nov 2026)
- [x] M5: BMP280 readout (27 Nov 2026)
- [ ] M6: framed telemetry stream to host (ETA late Nov 2026)

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

Toolchain, set up in M1: arm-none-eabi-gcc 13.2.1, GNU Make 4.3, gdb-multiarch 15.1 and stlink 1.8.0 on Ubuntu 24.04 inside WSL2. The board is plugged into Windows and forwarded into WSL with usbipd-win; the forward is repeated after every replug. `make` builds `build/firmware.elf`, `build/firmware.bin` and a map file and prints the section sizes, `make flash` writes the binary at 0x08000000 with st-flash, `make clean` removes `build/`. Debugging is `st-util` in one terminal and `gdb-multiarch build/firmware.elf` with `target extended-remote localhost:4242` in another. Source layout, build flags and the initial stack pointer: [D-007](docs/DECISIONS.md); hand-written register definitions instead of CMSIS: [D-006](docs/DECISIONS.md).

Serial console, set up in M2: USART2 on PA2 (TX) and PA3 (RX), wired to the ST-LINK virtual COM port through solder bridges SB13 and SB14 (UM1724), 115200 baud, 8N1, no flow control. The baud divider is 0x8B from the 16 MHz HSI, 0.08 percent low. In WSL the port is `/dev/ttyACM0`, read with `picocom -b 115200 /dev/ttyACM0`. The firmware prints an uptime line once a second, in hex until M4 and in decimal with the read counters since; the UART module (`src/uart.c`) is polled and blocking: init, send a byte, send a string, send a 32-bit value as hex, as an unsigned or as a signed decimal. Its waits on the transmitter have no time limit. Time comes from SysTick at 1 ms from the core clock, so the tick is as accurate as the HSI and has not been measured against a reference; the 32-bit millisecond counter wraps after about 49.7 days and the displayed seconds restart with it.

I2C bus, set up in M3: I2C1 on PB8 (SCL, Arduino D15) and PB9 (SDA, D14) as AF4 open-drain pins, standard mode at a nominal 100 kHz from the 16 MHz HSI (CCR 80, TRISE 17). The only pull-ups are the ones on the two breakouts, about 1.8 kΩ per line together. The MPU-6050 answers at 0x68 and the BMP280 at 0x76; the wiring and the measurements are in [docs/HARDWARE.md](docs/HARDWARE.md). The bus scan probes every address from 0x08 to 0x77 and prints the ones that acknowledged and a line with the counts; it takes about 18 ms, its own printing included. Up to M4 it ran at every reset, and since M5 it is part of the self-test boot mode. The driver (`src/i2c.c`) is polled. Every wait in it is limited to 5 ms by the SysTick counter: with the SDA wire pulled the scan ends in 112 timeouts instead of hanging. Driver functions return one status type, [D-008](docs/DECISIONS.md). The SCL frequency has not been measured.

MPU-6050 readout, set up in M4: the bus driver gained register operations, write one register and read N bytes starting at a register, with a repeated START and the three receive endings the STM32F4 I2C needs for one byte, two bytes and more ([D-009](docs/DECISIONS.md)). `src/mpu6050.c` checks the identity register (0x68 at 0x75), wakes the sensor by writing 0x01 to the power management register at 0x6B, which reads 0x40 after power-up, waits 100 ms, and reads the 14 measurement bytes from 0x3B in one transfer into seven signed 16-bit values: three accelerometer axes, temperature, three gyroscope axes, raw counts, high byte first. The ranges are the power-on defaults, ±2 g at 16384 counts per g and ±250 °/s at 131 counts per °/s; the two configuration registers read back as 0x00 on the bench. In M4 the driver did not write them; since M5 it writes them, together with the filter, the rate divider and the second power register, all to zero, so the ranges no longer depend on what an earlier run left in the sensor. The main loop reads a sample every 100 ms and prints the seven counts and the temperature in hundredths of a degree, raw / 340 + 36.53 in integer arithmetic; once a second it prints the uptime, the counts of good and failed reads and the number of bus recoveries. In M4 the firmware scanned the bus and ran the bus self-test of the milestone, tests 1 to 19, at every reset, which took about a second before the first sample; since M5 that is the self-test boot mode. A failed read returns a status and the loop carries on: with the SDA wire pulled from a running board and put back, 400 reads over 40 seconds gave 278 good ones, 94 timeouts and 28 bus errors with 150 recovery attempts, and ten good reads a second again without a reset.

Not verified in M4: only the accelerometer Z axis was checked against gravity, in two positions, and it reads about 13100 counts face up and about -20700 face down where ±16384 is nominal; the asymmetry is recorded, not explained and not corrected. The X and Y axes and the gyroscope under rotation were not checked. A sensor that holds SDA low is not recovered: the recovery resets the I2C peripheral and sends no clock pulses. The sensor is initialised once, at reset.

Modules and boot modes, set up in M5 ([D-010](docs/DECISIONS.md)): the startup file was split. `src/startup.c` keeps the reset handler, the default handler and the vector table; `src/systick.c` owns the millisecond counter, which no other module can write, and offers a getter, an elapsed-time helper and a delay; `src/app.c` holds `main`, the periodic tasks, the statistics and the printing; `src/selftest.c` holds the bench tests; `src/conversions.c` holds the temperature arithmetic of the MPU-6050. One image has two boot modes. The USER button on PC13 is read once after reset: released, the board goes straight to the sensors and the first sample is out 219 ms after reset; held, it first runs the conversion tests of both sensors, the bus scan and the bus self-test, 25 verdicts that all have to say PASS, and the first sample comes after about 1.09 s. The build gained `-Wconversion` and `-Wshadow` and has no warnings; the image is 13848 bytes, 4284 of them the bus self-test.

Bus clear, set up in M5: a reset of the MCU does not reset a sensor, and one that was sending a byte keeps SDA low and waits for clock pulses that never come. That happened on the bench: after a second reset in the middle of the self-test the init failed after every further reset, with SCL at 3.3 V and SDA at 0 V. Since then `i2c_init` releases both pins before it enables the peripheral and, if SDA reads low while SCL is high, drives SCL as an open-drain GPIO for up to nine pulses until SDA goes high (UM10204 3.1.16); SDA itself is never driven. A counter of successful bus clears is printed next to the recovery counter. At the acceptance the case was reproduced with the debugger, once per sensor: the board was halted at the start of a multi-byte read, with both lines low, and reset there; the next boot reported one bus clear and both sensors streamed, the first sample 3 ms and 31 ms later than usual. With the SDA wire pulled from a running board and put back, each sensor lost 87 of 100 reads over ten seconds, with 202 recovery attempts, and both returned to ten good reads a second without a reset.

BMP280 readout, set up in M5 ([D-011](docs/DECISIONS.md)): `src/bmp280.c` checks the id register (0x58 at 0xD0), resets the sensor by command (0xB6 to 0xE0), waits for its start-up and for the end of its NVM copy, reads the twelve calibration words in one transfer (24 bytes from 0x88, low byte first, two of the words unsigned) and writes 0x28 to `config` (0xF5) and then 0x57 to `ctrl_meas` (0xF4): normal mode, pressure oversampling ×16, temperature ×2, IIR filter coefficient 4, 62.5 ms standby, the "handheld device low-power" row of the datasheet, nominally ten measurements a second. A sample is six bytes read in one transfer from 0xF7, the pressure first and the temperature second, 20 bits each. The driver compensates both with the integer formulas of the datasheet, 32-bit for the temperature and 64-bit for the pressure, and returns hundredths of a degree Celsius and pascals in Q24.8, that is 1/256 Pa; the raw counts stay available. The self-test checks the temperature formula against the datasheet example, a sample of this unit and a negative temperature, and the pressure formula against the datasheet example, where the integer formula gives 25767233 and the figure prints 25767236. The main loop reads each sensor every 100 ms; the once-a-second report has a counter line per sensor, and a barometer sample counts as good only after a good read and a good compensation. At the acceptance 1001 consecutive barometer lines matched an independent implementation of the formulas digit for digit, and over 100 s every report showed ten good samples from each sensor. The serial link carries about 1640 bytes of text a second, 14% of what 115200 baud can take, and sending it blocks the loop for about 12 ms out of every 100.

Not verified in M5: the gyroscope under a known rotation, and the accelerometer beyond five positions held by hand, in which the signs of X and Y follow gravity and the vector is short by the Z offset noted in M4. The absolute pressure was not compared with a reference instrument: by the author's own reduction of a weather-station value to the height of the bench, which nobody checked, the sensor reads about 1.5 hPa above it, and raising the bench by hand lowers the reading by 10 Pa. The filter coefficient is known only from the value written to the sensor and read back. A sensor whose init fails at boot is not tried again. A sensor that resets or loses power in a running stream is not detected: the barometer would deliver its reset value, which compensates to a plausible-looking pair without an error. Nothing in the stream says how fresh a sample is, the polling period stretches while a recovery blocks, and the transmit waits of the UART are still unbounded. The reviewer entered the self-test mode through the debugger; the button path was run by the author.

## License

MIT
