# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Bare-metal C telemetry node on an STM32 Nucleo-F411RE: an MPU-6050 IMU and a BMP280 barometer on one I2C bus, telemetry streamed to a host PC over UART. The host side, a serial-to-MQTT bridge and a web dashboard, lives in a separate repo. Phase 3 puts a TF-Luna rangefinder on a pan servo and draws a live map of the room in that dashboard. The repo is the author's public C learning log, written from scratch while learning C and the STM32 platform. Details, status and roadmap: `README.md`.

## Ground rule: study and review only

Every line of firmware here is hand-written by the author. The README says so, and the commit history has to keep it true. So:

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

Debug: `st-util` (port 4242) in one terminal; `gdb-multiarch build/firmware.elf`, then `target extended-remote localhost:4242`, `monitor reset`, `break main`, `continue` in another. There is no test suite: a milestone is verified on the bench (M1: LD2 blinks) and by reading registers in gdb. Running `make`, `size`, `objdump` or `nm` to check a build is fine; editing the Makefile, the linker script or anything in `src/` is not (ground rule above).

## Architecture and scope

Data path: sensors, I2C, Nucleo-F411RE, UART, then the PC as gateway: a bridge publishes frames to a managed MQTT broker and the browser subscribes over WebSockets, replaying a recorded session while the board is off (`D-005`). The board has no network interface and the firmware gets no network stack. Phase 3 adds the rangefinder (UART or I2C, decided at M10) and a servo driven by PWM. Scope is bare-metal C with I2C, UART and timers/PWM. Out: RTOS, SPI, C++, flight control, networking on the board, a mobile platform (no odometry, no SLAM; `D-002`). Phase N+1 is discussed after phase N ships.

Build setup and source layout are `D-007`, register definitions `D-006`. Driver interfaces, the telemetry frame format and the rangefinder interface are undecided. Each becomes a decision-log entry when it is made. The frame format is the contract with the host repo: the spec lives here, the recorded dumps are its fixtures there.

Milestones, in order: M1 toolchain bring-up and LED blink, M2 UART hello to host, M3 I2C bus scan, M4 MPU-6050 readout, M5 BMP280 readout, M6 framed telemetry stream; phase 2, in the host repo: M7 frame parser and serial-to-MQTT bridge, M8 live plots with replay; phase 3: M9 servo sweep, M10 TF-Luna readout, M11 live map in the dashboard. Status lives in `README.md`.

## Docs

- `docs/DECISIONS.md`: append-only log for choices that are expensive to reverse. One `D-NNN` entry per decision using the template in the file, appended under `## Log`. A changed mind is a new entry that supersedes the old one, never a rewrite; only the old entry's status line gets the pointer.
- `docs/HARDWARE.md`: bill of materials, bench tools and wiring. Wiring is recorded only after the bench confirms it: M3 for the I2C sensors, M9 and M10 for the servo and the rangefinder.

## Git

Commit subjects are lowercase `type: summary` in English (`docs:`, `chore:`). No `Co-Authored-By`, `Generated by` or similar trailers anywhere: the repo's premise depends on it.
