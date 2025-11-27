#pragma once

// PD-Stepper pins (ESP32-S3)
constexpr int PIN_TMC_EN   = 21;   // LOW = enable driver
constexpr int PIN_STEP     = 5;
constexpr int PIN_DIR      = 6;
constexpr int PIN_MS1      = 1;
constexpr int PIN_MS2      = 2;
constexpr int PIN_SPREAD   = 7;    // optional spread/stealth toggle
constexpr int PIN_TMC_TX   = 17;   // ESP32 TX -> TMC PDN_UART
constexpr int PIN_TMC_RX   = 18;   // ESP32 RX <- TMC PDN_UART
constexpr int PIN_DIAG     = 16;   // StallGuard DIAG
constexpr int PIN_INDEX    = 11;   // not used here
constexpr int PIN_PG       = 15;   // USB-PD power good (LOW = good)
constexpr int PIN_CFG1     = 38;   // PD voltage select
constexpr int PIN_CFG2     = 48;
constexpr int PIN_CFG3     = 47;
constexpr int PIN_VBUS_ADC = 4;    // not used in this starter, reserved
constexpr int PIN_LED1     = 10;
constexpr int PIN_LED2     = 12;

constexpr int PIN_SDA      = 8;    // AS5600 I2C
constexpr int PIN_SCL      = 9;

constexpr uint8_t AS5600_ADDR = 0x36;
constexpr uint8_t AS5600_RAW_ANG_H = 0x0C;
