#pragma once
#include <TMC2209.h>
#include "config.h"

extern TMC2209 stepper;
void TMC_init(const MotionParams& mp);
void TMC_enable(bool en);
void PD_request_12V();
bool PD_powerGood();
void TMC_setDynamic(const MotionParams& mp);
void TMC_setThresholds();  // PWM & coolStep thresholds for StallGuard validity
