# HomeSpan_PD_Blinds (PD-Stepper → native HomeKit)

This example registers a HomeKit **WindowCovering** accessory for PD-Stepper (ESP32-S3 + TMC2209 + AS5600):
- UART config of TMC2209 (microsteps, run current, stealthChop, StallGuard threshold)
- AS5600 absolute encoder → 0–100% mapping
- Simple jerk-limited motion toward TargetPosition
- **StallGuard homing to find MIN position (fully closed)**
- **StallGuard homing to find MAX position (fully open)**
- **PD voltage selection (5V, 9V, 12V, 15V, 20V) with measurement**
- **Web-based HTML configuration interface**
- Power-good gating from USB-PD "PG" pin
- Serial CLI to tune current, velocity, acceleration, jerk. Values persist in NVS.

## Requirements
- Arduino ESP32 core installed
- Libraries:
  - [HomeSpan] (Library Manager)
  - [TMC2209 by Janelia] (Library Manager)
  - [AsyncTCP] (Library Manager) - for web server
  - [ESPAsyncWebServer] (Library Manager) - for web server
  - [ArduinoJson] (Library Manager) - for API communication
- Board: **ESP32S3 Dev Module** (CDC on boot: Enabled)
- The pins and timing assume the ESP32-S3 target; the sketch now emits a compile-time error if a non-S3 board profile is chosen.

## Pins (PD-Stepper)
See `pd_pins.h`. Default I2C: SDA=8, SCL=9. TMC UART: TX=17, RX=18. DIAG=16. PG=15. PD CFG: 38/48/47.

## First run
1. Power via USB-PD. Default voltage is 12V (configurable via web interface).
2. Pair with Home using HomeSpan console (first boot) and your iPhone.
3. **Web Configuration (Recommended):**
   - Connect to WiFi network "PD-Blinds-Config" (password: config1234)
   - Open http://192.168.4.1 in your browser
   - Configure PD voltage, velocity, acceleration
   - Use "Find Min Position" to detect minimum travel using stall detection
   - Manually move to max position and use "Use Current as Max" or enter encoder count
4. **HomeKit Configuration:**
   - Tap **Start Homing** (finds MAX position using stall detection)
   - Tap **Find Min Position** (finds MIN position using stall detection)
   - Use the slider to move fully down. Tap **Set Bottom Limit** to lock BOTTOM manually.

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

## Web Configuration Interface

The device creates a WiFi Access Point for easy configuration:
- **SSID:** `PD-Blinds-Config`
- **Password:** `config1234`
- **URL:** `http://192.168.4.1`

### Features:
- **PD Voltage Selection:** Set 5V, 9V, 12V, 15V, or 20V with real-time voltage measurement
- **Motion Parameters:** Configure velocity and acceleration
- **Position Limits:**
  - Find MIN position using stall detection (moves down until stall)
  - Find MAX position using stall detection (moves up until stall)
  - Set max position manually by encoder count
  - Use current position as max limit
- **Real-time Status:** View current position, limits, and settings

### API Endpoints:
- `GET /api/status` - Get current configuration and status
- `POST /api/setPdVoltage` - Set PD voltage (JSON: `{"voltage": 12}`)
- `POST /api/setVelocity` - Set velocity (JSON: `{"velocity": 3000}`)
- `POST /api/setAcceleration` - Set acceleration (JSON: `{"acceleration": 8000}`)
- `POST /api/setMaxPosition` - Set max position (JSON: `{"position": 100000}`)
- `POST /api/setCurrentAsMax` - Use current encoder position as max
- `POST /api/findMinPosition` - Find min position using stall detection
- `POST /api/findMaxPosition` - Find max position using stall detection

## PD Voltage Configuration

The board supports USB-PD voltages: 5V, 9V, 12V, 15V, and 20V. The CFG pins (38, 48, 47) control the voltage request:
- **5V:** CFG1=LOW, CFG2=LOW, CFG3=LOW
- **9V:** CFG1=HIGH, CFG2=LOW, CFG3=LOW
- **12V:** CFG1=LOW, CFG2=LOW, CFG3=HIGH (default)
- **15V:** CFG1=HIGH, CFG2=LOW, CFG3=HIGH
- **20V:** CFG1=LOW, CFG2=HIGH, CFG3=HIGH

**Note:** If your board uses different CFG pin combinations, edit `PD_setVoltage()` in `tmc_init.cpp`.

Voltage can be measured via `PIN_VBUS_ADC` (pin 4) if connected to a voltage divider. Adjust the divider ratio in `PD_measureVoltage()` if needed.

## Safety
- Motion is inhibited if PG is not good.
- On stall during normal moves: halt and report STOPPED.

## Serial CLI

**Serial tuning (prefix with ':' to avoid clashing with HomeSpan console):**
```
:cur 700
:vel 2500
:acc 8000
:jerk 50000
:micro 32
:stealth 1
:stall 10
```
