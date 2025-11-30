#pragma once
#include <Arduino.h>

// Encoder resolution
constexpr int ENC_COUNTS_PER_REV = 4096;

// PD Voltage options
enum PDVoltage {
  PD_5V  = 5,
  PD_9V  = 9,
  PD_12V = 12,
  PD_15V = 15,
  PD_20V = 20
};

// Default motion parameters (counts are encoder counts)
struct MotionParams {
  float maxVel_cps   = 3000.0f;     // counts/sec
  float maxAcc_cps2  = 8000.0f;     // counts/sec^2
  float maxJerk_cps3 = 50000.0f;    // counts/sec^3

  uint16_t microsteps     = 32;     // 4,8,16,32,64,256
  uint16_t runCurrent_mA  = 800;    // mA
  int8_t   stallThreshold  = 10;     // -64..63
  bool     stealthChop     = true;   // quiet low-speed mode
  PDVoltage pdVoltage      = PD_12V; // PD voltage setting
};

// NVS space
#define NVS_NS "pdstepper"

// Homing
constexpr unsigned long HOMING_TIMEOUT_MS = 20000;
