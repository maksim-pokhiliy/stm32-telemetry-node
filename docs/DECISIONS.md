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
- Status: accepted

**Context.** The telemetry node (M0-M6) reads an IMU and a barometer and streams frames to a serial terminal. Correct, and dull to watch. The project needs a demo that carries a README gif and a short video on its own, and the host side should use the stack the author already knows, the web. A room scanner does that, and its first five links (toolchain, UART, bus, sensor readout, framed stream) are the M1-M6 roadmap verbatim; only the payload and the consumer change. Sensor candidates: HC-SR04 ultrasonic (cheap, wide cone that smears walls and corners into a submarine-movie radar), VL53L1X laser time-of-flight (I2C, room-scale range, an honest but soft outline), TF-Luna (single-beam rangefinder, the longest range and the narrowest beam of the three, UART or I2C).

**Decision.** Phase 2, milestones M7-M9: a TF-Luna on an MG90S pan servo sweeps the room, the firmware streams angle-distance pairs to the host as telemetry frames, the host draws a live map in a web dashboard. TF-Luna over the cheaper options because the map is the product and the beam width decides how it looks. MG90S over SG90 for the metal gears: less backlash, repeatable angles, a map that does not breathe. Phase 1 milestones and dates do not move; phase 2 starts after M6 ships. The rangefinder interface (UART or I2C) is decided at M8 and gets its own entry.

**Consequences.** New ground: timers and PWM for the servo, possibly a second serial link, and a separate 5 V rail so the servo does not brown out the Nucleo over USB. Hardware for both phases is ordered together (docs/HARDWARE.md). The wording stays "scanning rangefinder + live map": no "SLAM", no "lidar cartography", because there is no odometry and no pose stitching, and any embedded interviewer sees that gap in a second.

### D-002: No mobile platform

- Date: 2026-09-29
- Status: accepted

**Context.** With a rangefinder in hand the next idea is obvious: put it on a wheeled chassis and scan the whole flat. The chassis is a weekend: a kit, two motors, a driver board. The mapping is not. To place a measurement on the map the firmware needs the platform's pose at the moment of the measurement, and a cheap chassis has no way to know it. Wheel odometry drifts from the first metre (slip, wheel diameter tolerance, and no encoders on TT motors at all), a gyro heading drifts within minutes, and correcting either from a single-beam rangefinder is SLAM: a richer sensor, a graduate-level class of math, and a compute budget that in hobby terms means ROS on a Linux board, not bare-metal C on an F411. It would also break the story of this repo. The demo argument points the same way: a static scan gives a sharp room outline, a naively moving one gives doubled walls and a drift spiral. Three times the effort for a worse video.

**Decision.** No wheels in this project. The scanner stays on a static mount. If a phase 3 ever happens it is stop-and-scan: drive, stop, take a full static scan, send it as one frame, drive on. Every scan stays clean, SLAM is sidestepped, and the new skill is real: encoders and PID to drive straight and turn an honest 90 degrees. Rule for the project from here on: phase N+1 is discussed after phase N ships. Phase 3 is not discussed before M9 is done.

**Consequences.** Scope stays bare-metal C with I2C, UART and timers/PWM. The README avoids "SLAM", "mapping robot" and "lidar cartography". The vocabulary (odometry drift, dead reckoning, loop closure) is worth knowing anyway: "I understand why I did not do SLAM on an MCU with a single-beam rangefinder" is a better interview answer than a crooked robot on GitHub.
