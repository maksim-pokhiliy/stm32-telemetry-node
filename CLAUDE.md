# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Bare-metal C telemetry node on an STM32 Nucleo-F411RE: an MPU-6050 IMU and a BMP280 barometer on one I2C bus, telemetry streamed to a host PC over UART. The host side, a serial-to-MQTT bridge and a web dashboard, lives in a separate repo. Phase 3 puts a TF-Luna rangefinder on a pan servo and draws a live map of the room in that dashboard. The repo is the author's public C learning log, written from scratch while learning C and the STM32 platform. Details, status and roadmap: `README.md`.

## Ground rule: study and review only

Every line of firmware here is hand-written by the author; the one footnote is a rename-and-comment pass by an AI over `src/startup.c` and `src/stm32f411_regs.h` after the first UART evening of M2, with the register values and the logic unchanged, recorded in README and the M2 commit. The README says so, and the commit history has to keep it true. So:

- Never write firmware, a Makefile, a linker script, startup code, CMSIS/HAL glue, or an OpenOCD/st-link/gdb config for this repo. Not as a starting point, not as a reference to be rewritten. Phase 3 firmware (servo, rangefinder) is under the same rule.
- Never write hardware facts into the repo ahead of the author confirming them on the bench: pinout, I2C addresses, register values, baud rates, wiring. The `TBD` markers in the docs stay until the author reports the milestone done.
- Explaining a register, a bus protocol or a linker concept is study. A drop-in function, file or config is code generation. When the line is unclear, leave the artifact out and ask.
- Ground explanations in the STM32F411 reference manual and the sensor datasheets, cited by section, rather than in recall.
- Review the author's code the way a senior embedded engineer would: register semantics, timing, error paths, what the datasheet says versus what the code assumes. Quizzing is welcome.
- Docs edits are fine when they record what the author already did or decided.
- The language is "C", never "C/C++".
- Nothing host-side is written here. The bridge and the dashboard belong to the host repo, with its own rules (`D-004`). If that repo does not exist yet, ask where it should go instead of creating a `host/` folder here.

The ceiling moves as the author learns. The README status section and the roadmap checkboxes say where it is now.

`.claude/hooks/guard-firmware-ceiling.sh` enforces the first rule: it blocks Write/Edit and Bash writes into `src/`, `*.c`, `*.h`, `*.s`, `*.ld`, Makefiles and debugger configs. If it fires, you were about to break the rule. Do not work around it.

## Build, flash, test

Toolchain: arm-none-eabi-gcc 13.2.1, GNU Make 4.3, gdb-multiarch 15.1 and stlink 1.8.0 on Ubuntu 24.04 in WSL2. The board is forwarded from Windows with usbipd-win; the forward has to be repeated after a replug. The author's `Makefile` at the repo root is the build; the linker script `stm32f411re.ld` sits beside it, sources in `src/`, outputs in the gitignored `build/` (`D-007`).

- `make`: compiles `src/*.c` into `build/`, links `build/firmware.elf`, prints `size`, writes `build/firmware.bin` and `build/firmware.map`.
- `make flash`: `st-flash write build/firmware.bin 0x08000000`.
- `make clean`: removes `build/`.

Debug: `st-util` (port 4242) in one terminal; `gdb-multiarch build/firmware.elf`, then `target extended-remote localhost:4242`, `monitor reset`, `break main`, `continue` in another. Serial console: the ST-LINK virtual COM port is `/dev/ttyACM0` in WSL, `picocom -b 115200 /dev/ttyACM0` (8N1, no flow control); since M4 the firmware prints, after a reset, a temperature conversion table, `i2c_init: OK`, the `mpu6050_init` status, the bus scan (`found 0x…` for each address that answered and a `scan done:` line with the counts) and the bus self-test, `test 1` to `test 19` with PASS or FAIL, about a second in all; then, ten times a second, a line of seven signed decimal counts (accel X Y Z, temperature, gyro X Y Z) and a `temp_centi=` line, and once a second `uptime_s=N ok=N nack=N timeout=N bus=N other=N recover=N` followed by `  last:` and the latest sample. A failed read prints `mpu read: status=0x…` with the numeric `status_t`. `src/uart.c` is the polled, blocking UART module (init, send byte, send string, send hex32, send a signed decimal); `src/i2c.c` is the polled I2C1 driver on PB8/PB9 (init, probe an address, write one register, read N bytes from a register with the three receive endings of RM0383 18.3.3; every wait bounded by the SysTick counter, recovery through an RCC reset of the peripheral, a counter of recovery attempts); `src/mpu6050.c` is the sensor driver on top of the two register functions (identity check, wake, one 14-byte read into seven `int16_t`, raw counts; `D-009`); `src/status.h` is the status type every driver returns (`D-008`, seven values since `D-009`); `src/systick.h` declares the millisecond tick; `src/startup.c` holds the reset handler, the vector table, the SysTick 1 ms tick, the bench self-tests of M4 and `main` with its three periodic tasks. The author plans to split that file at the start of M5. There is no test suite: a milestone is verified on the bench (M1: LD2 blinks; M2: uptime lines on the serial port; M3: the scan finds 0x68 and 0x76; M4: all self-tests PASS and every report says `ok=10` with no failed read) and by reading registers in gdb. On connect st-util resets the target and halts it a few milliseconds later, in the first prints after the reset; `monitor reset` halts it at the reset handler. Debugger reads of I2C1 have side effects: SR1 then SR2 clears ADDR, DR clears RxNE. A reset of the board does not reset the sensors: they keep their registers until their power is cycled. Running `make`, `size`, `objdump` or `nm` to check a build is fine, so is reading the serial port or registers over st-util and gdb when the author asks for an acceptance check; editing the Makefile, the linker script or anything in `src/` is not (ground rule above).

## Architecture and scope

Data path: sensors, I2C, Nucleo-F411RE, UART, then the PC as gateway: a bridge publishes frames to a managed MQTT broker and the browser subscribes over WebSockets, replaying a recorded session while the board is off (`D-005`). The board has no network interface and the firmware gets no network stack. Phase 3 adds the rangefinder (UART or I2C, decided at M10) and a servo driven by PWM. Scope is bare-metal C with I2C, UART and timers/PWM. Out: RTOS, SPI, C++, flight control, networking on the board, a mobile platform (no odometry, no SLAM; `D-002`). Phase N+1 is discussed after phase N ships.

Build setup and source layout are `D-007`, register definitions `D-006`, the driver status type and the rule that no wait is unbounded `D-008`, the boundary between the bus driver and a sensor driver `D-009`: the bus driver speaks registers, a sensor driver knows its register map, returns raw counts and touches no STM32 register. The telemetry frame format and the rangefinder interface are undecided. Each becomes a decision-log entry when it is made. The frame format is the contract with the host repo: the spec lives here, the recorded dumps are its fixtures there.

Milestones, in order: M1 toolchain bring-up and LED blink, M2 UART hello to host, M3 I2C bus scan, M4 MPU-6050 readout, M5 BMP280 readout, M6 framed telemetry stream; phase 2, in the host repo: M7 frame parser and serial-to-MQTT bridge, M8 live plots with replay; phase 3: M9 servo sweep, M10 TF-Luna readout, M11 live map in the dashboard. Status lives in `README.md`.

## Docs

- `docs/DECISIONS.md`: append-only log for choices that are expensive to reverse. One `D-NNN` entry per decision using the template in the file, appended under `## Log`. A changed mind is a new entry that supersedes the old one, never a rewrite; only the old entry's status line gets the pointer.
- `docs/HARDWARE.md`: bill of materials, bench tools and wiring. Wiring is recorded only after the bench confirms it: the I2C sensors went in at M3, the servo and the rangefinder follow at M9 and M10.

## Git

Commit subjects are lowercase `type: summary` in English (`docs:`, `chore:`). No `Co-Authored-By`, `Generated by` or similar trailers anywhere: the repo's premise depends on it.
