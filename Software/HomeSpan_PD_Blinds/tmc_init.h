#pragma once
#include <TMC2209.h>
#include "config.h"

extern TMC2209 stepper;
void TMC_init(const MotionParams& mp);
void TMC_enable(bool en);
void PD_request_12V();
void PD_setVoltage(PDVoltage voltage);
float PD_measureVoltage();  // Measure actual voltage via ADC
bool PD_powerGood();
void TMC_setDynamic(const MotionParams& mp);

// Declaration only — no function body in the header
void TMC_setThresholds();
