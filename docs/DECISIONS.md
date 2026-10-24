# Decisions

Decision log for choices that are expensive to reverse: telemetry frame format, driver layout, build setup, host-side tooling, sensor and scope choices. One entry per decision, appended in order, never rewritten; a changed mind gets a new entry that supersedes the old one.

## Template

### D-NNN: Short title

- Date: YYYY-MM-DD
- Status: accepted (or: proposed, superseded by D-NNN)

**Context.** The situation that forced a choice, the constraints, the alternatives considered.

**Decision.** What was chosen and why.

**Consequences.** What gets easier, what gets harder, what to watch for.

## Log

### D-001: Phase 2 is a scanning rangefinder with a live map

- Date: 2026-09-29
- Status: accepted; the milestone numbering is superseded by D-003, the sensor choice and the scope stand

**Context.** The telemetry node (M0-M6) reads an IMU and a barometer and streams frames to a serial terminal. Correct, and dull to watch. The project needs a demo that carries a README gif and a short video on its own, and the host side should use the stack the author already knows, the web. A room scanner does that, and its first five links (toolchain, UART, bus, sensor readout, framed stream) are the M1-M6 roadmap verbatim; only the payload and the consumer change. Sensor candidates: HC-SR04 ultrasonic (cheap, wide cone that smears walls and corners into a submarine-movie radar), VL53L1X laser time-of-flight (I2C, room-scale range, an honest but soft outline), TF-Luna (single-beam rangefinder, the longest range and the narrowest beam of the three, UART or I2C).

**Decision.** Phase 2, milestones M7-M9: a TF-Luna on an MG90S pan servo sweeps the room, the firmware streams angle-distance pairs to the host as telemetry frames, the host draws a live map in a web dashboard. TF-Luna over the cheaper options because the map is the product and the beam width decides how it looks. MG90S over SG90 for the metal gears: less backlash, repeatable angles, a map that does not breathe. Phase 1 milestones and dates do not move; phase 2 starts after M6 ships. The rangefinder interface (UART or I2C) is decided at M8 and gets its own entry.

**Consequences.** New ground: timers and PWM for the servo, possibly a second serial link, and a separate 5 V rail so the servo does not brown out the Nucleo over USB. Hardware for both phases is ordered together (docs/HARDWARE.md). The wording stays "scanning rangefinder + live map": no "SLAM", no "lidar cartography", because there is no odometry and no pose stitching, and any embedded interviewer sees that gap in a second.

### D-002: No mobile platform

- Date: 2026-09-29
- Status: accepted

**Context.** With a rangefinder in hand the next idea is obvious: put it on a wheeled chassis and scan the whole flat. The chassis is a weekend: a kit, two motors, a driver board. The mapping is not. To place a measurement on the map the firmware needs the platform's pose at the moment of the measurement, and a cheap chassis has no way to know it. Wheel odometry drifts from the first metre (slip, wheel diameter tolerance, and no encoders on TT motors at all), a gyro heading drifts within minutes, and correcting either from a single-beam rangefinder is SLAM: a richer sensor, a graduate-level class of math, and a compute budget that in hobby terms means ROS on a Linux board, not bare-metal C on an F411. It would also break the story of this repo. The demo argument points the same way: a static scan gives a sharp room outline, a naively moving one gives doubled walls and a drift spiral. Three times the effort for a worse video.

**Decision.** No wheels in this project. The scanner stays on a static mount. If a phase 3 ever happens it is stop-and-scan: drive, stop, take a full static scan, send it as one frame, drive on. Every scan stays clean, SLAM is sidestepped, and the new skill is real: encoders and PID to drive straight and turn an honest 90 degrees. Rule for the project from here on: phase N+1 is discussed after phase N ships. Phase 3 is not discussed before M9 is done.

**Consequences.** Scope stays bare-metal C with I2C, UART and timers/PWM. The README avoids "SLAM", "mapping robot" and "lidar cartography". The vocabulary (odometry drift, dead reckoning, loop closure) is worth knowing anyway: "I understand why I did not do SLAM on an MCU with a single-beam rangefinder" is a better interview answer than a crooked robot on GitHub.

### D-003: Host side before the scanner

- Date: 2026-09-30
- Status: accepted; supersedes the milestone numbering of D-001

**Context.** D-001 put the scanner right after the telemetry node and left the host side for the last milestone. That leaves M6, the framed stream, validated only by a serial terminal, which shows bytes but checks neither framing nor checksums. It also builds the host pipeline and the scanner in one step, a big-bang integration at the very end. The host side is the author's native stack and the fastest part to build, and it is also the most comfortable place to overbuild.

**Decision.** Phase 2 is the host side: M7 a frame parser and a serial-to-MQTT bridge, built and tested on dumps recorded at M6; M8 live plots in the browser, with replay of a recorded session when the board is off. The scanner becomes phase 3: M9 servo sweep, M10 TF-Luna readout, M11 the live map in the same dashboard. Phase 1 milestones and dates do not move. Phases are defined by a shipped outcome, not by an activity: phase 1 ends when the board streams framed telemetry, not when the soldering ends. Phase 2 is time-boxed to the bridge, the plots and the replay: no auth, no database, no servers of the author's own.

**Consequences.** The protocol gets a real consumer while it is still warm, so framing and checksum mistakes surface before the scanner depends on it. When the scanner arrives, only the payload and one visualization are new. The full story, device to protocol to bridge to browser, exists after phase 2 even if phase 3 slips. The rule of D-002 ("stop-and-scan" is the earliest candidate for a phase 4) still applies, and its "M9" now reads M11.

### D-004: Firmware and host side live in separate repos

- Date: 2026-09-30
- Status: accepted

**Context.** The host side is a bridge process and a web dashboard sharing a frame parser. Options: a monorepo with the firmware, a GitHub organisation with one repo per tier, or two repos. This repo's premise is hand-written firmware; the host side is web code, where a different authoring policy is fine.

**Decision.** Two repos. This one stays pure C: firmware, decision log, hardware notes, and the demo gif with a link to the host repo. The host repo holds the bridge and the dashboard together, because they share the parser and the frame types, and nobody wants to publish a package for a parser of a couple of hundred lines. No organisation: that is a tool for a team or a brand, and a reader opens a repo, not an org page. The frame format is the contract between the two repos: the spec lives here in the decision log, and raw dumps recorded from the board are the fixtures the host parser tests against.

**Consequences.** Two toolchains, two CI setups and two READMEs stay apart, and the "hand-written" disclaimer of this repo stays absolute. The AI policy for the host repo is decided there. A protocol change is a two-repo change, which is the point: it forces the spec to be written down first.

### D-005: Host transport is a managed MQTT broker, replay first

- Date: 2026-09-30
- Status: accepted

**Context.** The demo should be viewable by someone in another city, and "switch the board on and it streams" is the goal. The board has no network interface, so the firmware cannot talk to a broker; the PC it is plugged into can. That is the standard node-gateway topology: a node on bare metal over UART, a gateway with the network stack, a broker, a browser. Candidates for the relay: a managed MQTT broker with WebSocket support, a self-written WebSocket relay on a rented server, a tunnel exposing the local bridge, or a realtime database. Live only works while the board and the PC are on, and a visitor opens the link when they are off.

**Decision.** The bridge on the PC publishes frames to a managed MQTT broker with a free tier; the dashboard subscribes with MQTT over WebSockets. One code path serves the desk and the remote viewer. The "API" is the topic layout and the payload schema, not REST routes. Publishing credentials live only in the bridge, the browser uses a read-only user, and a retained message carries the last frame. The dashboard is replay first: a recorded session ships with the static site and plays by default, labelled with its recording date; the view switches to live, labelled as such, when a stream is up. No database: browsing history is a later phase or never. No network on the board: a WiFi module and a TCP/IP stack in the firmware would be a phase of its own. The gateway can later become a small Linux board, so that "switch it on and it streams" is literal, with no firmware change.

**Consequences.** One hosted dependency, the broker, and its credentials to manage. Recorded dumps do quadruple duty: parser fixtures, offline development without the board, the public demo, and the replay mode. Firmware scope is untouched; the mistake to avoid is putting the MQTT client on the node.

### D-006: Hand-written peripheral register definitions

- Date: 2026-10-19
- Status: accepted

**Context.** Bare-metal firmware requires direct access to STM32 peripheral registers. Two approaches were considered: using CMSIS device headers from Arm/ST or defining the required register addresses and bit masks manually from RM0383. The repository is both a working firmware project and a public C learning log, where understanding the hardware and writing firmware independently take priority over development speed.

**Decision.** Use hand-written register definitions for Phase 1 (M1-M6), without CMSIS device headers or HAL. Define only the registers and bit masks actually needed by the firmware in `src/stm32f411_regs.h`, using names consistent with RM0383. Register addresses use the `_ADDR` suffix to distinguish them from register values. AI may assist with documentation lookup and explanations, but the author writes the definitions and verifies them against the reference manual and hardware.

**Consequences.** Every register address, bit position and mask must be understood and explicitly defined, improving familiarity with the STM32 memory map and peripheral architecture. The firmware remains fully hand-written without external device-header dependencies. The trade-offs are additional manual work, repetitive definitions and the risk of transcription errors. Register definitions must be checked against RM0383 and validated through debugging or hardware tests as peripherals are introduced. CMSIS remains a possible alternative for future phases if the maintenance cost outweighs the educational benefit.

### D-007: Source layout, build and the initial stack pointer

- Date: 2026-10-24
- Status: accepted

**Context.** M1 produced the first files that had to live somewhere: a linker script, startup code, a register header and a build. The layout was chosen on 8 Oct 2026 and is written up here at the close of M1. The alternatives were a vendor-style tree (CMSIS folders, a generated project, a `Startup/` directory with an assembly file) and a flat layout the author understands completely. The initial stack pointer needed a home too: the factory demo firmware on the board reports an initial SP of 0x20000560, a fixed-size stack placed right after its data, while the ST linker templates put the stack at the top of RAM.

**Decision.** Flat layout: the linker script `stm32f411re.ld` at the repo root, all C in `src/`, one module per `.c` and `.h` pair, the register header `src/stm32f411_regs.h` (D-006), a GNU `Makefile` at the root once the hand-typed commands stopped changing, build products in `build/`, which is gitignored. The build is `arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -Og -g3 -ffreestanding -Wall -Wextra` with `-MMD -MP` dependency files, linked through the gcc driver with `-nostdlib -lgcc -T stm32f411re.ld` and a map file, `objcopy -O binary` for the flash image, `st-flash write` at 0x08000000. The initial stack pointer is the top of SRAM, `ORIGIN(SRAM) + LENGTH(SRAM)` = 0x20020000, defined once in the linker script as `STACK_TOP` and referenced from C as an address: the stack grows towards lower addresses, so placed at the top it gets all of SRAM below it.

**Consequences.** Everything the chip executes is in files the author wrote, and `size`, `objdump` and `nm` on `build/firmware.elf` are the ground truth for what landed where. No startup assembly: the reset handler is C, with the vector table as a C array in its own section. The flat `src/` will need structure when drivers multiply; that is a later entry. `-nostdlib` means no libc and no C runtime: `-lgcc` is on the link line for the day a 64-bit division or a float needs a helper, and anything else (memcpy, printf) is written here or not used. The stack has no guard: it grows down towards `.bss` without warning, so stack depth is something to watch once interrupts and drivers arrive. The FPU stays off (`-mfloat-abi=soft` by default) until a decision at M5, when floats first appear.
