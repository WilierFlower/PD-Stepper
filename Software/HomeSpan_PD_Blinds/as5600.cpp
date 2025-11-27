#include "as5600.h"
#include "pd_pins.h"
#include <Wire.h>

static int32_t prevRaw = -1;
static int32_t revs = 0;
volatile int32_t AS5600_totalCounts = 0;

void AS5600_begin(int sda, int scl){
  Wire.begin(sda, scl, 400000);
}

uint16_t AS5600_readRaw12(){
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(AS5600_RAW_ANG_H);
  if (Wire.endTransmission(false)!=0) return (prevRaw<0)?0:(uint16_t)prevRaw;
  Wire.requestFrom(AS5600_ADDR,(uint8_t)2);
  if (Wire.available()<2) return (prevRaw<0)?0:(uint16_t)prevRaw;
  uint16_t hi=Wire.read(), lo=Wire.read();
  return ((hi&0x0F)<<8)|lo;
}

int32_t AS5600_readTotal(){
  uint16_t raw = AS5600_readRaw12();
  if (prevRaw<0) prevRaw = raw;
  int16_t d = (int16_t)raw - (int16_t)prevRaw;
  if (d >  2048) revs--;    // 4095->0
  if (d < -2048) revs++;    // 0->4095
  prevRaw = raw;
  AS5600_totalCounts = (int32_t)raw + revs*4096;
  return AS5600_totalCounts;
}
