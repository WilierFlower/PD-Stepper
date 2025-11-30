# HomeSpan_PD_Stepper (ESP32-S3 Dev Module)

This folder documents how to use the HomeSpan-based PD Stepper sketch with the **ESP32S3 Dev Module** board profile. The sketch itself lives in `Software/HomeSpan_PD_Blinds` and targets the ESP32-S3 on the PD Stepper hardware, so the same board settings apply to the ESP32-S3 dev kits that ship with the product from @joshr120.

## Board selection
- Board: **ESP32S3 Dev Module**
- USB CDC On Boot: **Enabled**
- Flash size: 8MB (default for the kit)
- PSRAM: Enabled if your IDE prompts for it.

> The code now has a compile-time guard that will error if you pick a non-S3 target, making it clear when the wrong board profile is selected.

## Using the sketch
1. Open `Software/HomeSpan_PD_Blinds/HomeSpan_PD_Blinds.ino` in Arduino IDE.
2. Select the **ESP32S3 Dev Module** board profile and enable USB CDC on boot.
3. Install dependencies listed in that folder's README (HomeSpan, TMC2209, AsyncTCP, ESPAsyncWebServer, ArduinoJson).
4. Flash to your ESP32-S3 dev module; the pin map in `pd_pins.h` matches the PD Stepper hardware.

If you need to adapt the pin map for a different ESP32-S3 dev board layout, update the constants in `pd_pins.h` inside `Software/HomeSpan_PD_Blinds`.
