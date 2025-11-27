# HomeSpan_PD_Blinds (PD-Stepper → native HomeKit)

This example registers a HomeKit **WindowCovering** accessory for PD-Stepper (ESP32-S3 + TMC2209 + AS5600):
- UART config of TMC2209 (microsteps, run current, stealthChop, StallGuard threshold)
- AS5600 absolute encoder → 0–100% mapping
- Simple jerk-limited motion toward TargetPosition
- StallGuard homing to find TOP, then user sets BOTTOM
- Power-good gating from USB-PD “PG” pin
- Serial CLI to tune current, velocity, acceleration, jerk. Values persist in NVS.

## Requirements
- Arduino ESP32 core installed
- Libraries:
  - [HomeSpan] (Library Manager)
  - [TMC2209 by Janelia] (Library Manager)
- Board: **ESP32S3 Dev Module** (CDC on boot: Enabled)

## Pins (PD-Stepper)
See `pd_pins.h`. Default I2C: SDA=8, SCL=9. TMC UART: TX=17, RX=18. DIAG=16. PG=15. PD CFG: 38/48/47.

## First run
1. Power via USB-PD. 12 V is requested at boot (edit in `tmc_init.cpp` if desired).
2. Pair with Home using HomeSpan console (first boot) and your iPhone.
3. Tap **Start Homing**. The unit moves up until StallGuard trips → sets TOP.
4. Use the slider to move fully down. Tap **Set Bottom Limit** to lock BOTTOM.

## Serial CLI (115200)
```
cur <mA>      e.g. cur 700
vel <cps>     counts/sec
acc <cps2>    counts/sec^2
jerk <cps3>   counts/sec^3
micro <n>     4|8|16|32|64|256
stealth 0|1
stall <thr>   -64..63
```
All persisted to NVS.

## Tuning notes
- StallGuard requires spreadCycle at sufficient speed. We set PWM & coolStep thresholds internally. During homing we run above the threshold so DIAG will fire.
- If motion is too zippy or too loud, lower `vel/acc/jerk` and keep `stealth 1`.
- If homing never trips, raise `stall <thr>` or increase the homing speed slightly.

## Safety
- Motion is inhibited if PG is not good.
- On stall during normal moves: halt and report STOPPED.
