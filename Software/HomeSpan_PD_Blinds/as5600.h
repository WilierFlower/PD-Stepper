#pragma once
#include <Arduino.h>

void     AS5600_begin(int sda, int scl);
uint16_t AS5600_readRaw12();
int32_t  AS5600_readTotal();        // wrap-aware total
extern volatile int32_t AS5600_totalCounts;
