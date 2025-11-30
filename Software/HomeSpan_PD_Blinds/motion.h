#pragma once
#include "config.h"

struct Limits {
  int32_t top=0;
  int32_t bottom=100000;
  bool set=false;
};

extern MotionParams g_mp;
extern Limits g_limits;

void Motion_begin();
void Motion_loop(float dt_ms, bool powerGood, bool& stallFlag);
void Motion_setTargetPercent(int pct);
int  Motion_getCurrentPercent();
void Motion_halt();
void Motion_home(bool& stallFlag);
void Motion_homeMin(bool& stallFlag);  // Find min position using stall detection
void Motion_saveNVS();
void Motion_loadNVS();

void Motion_serialCLI();  // parses and persists changes

// Getters for web interface
float Motion_getVelocity();
float Motion_getAcceleration();
void Motion_setVelocity(float vel);
void Motion_setAcceleration(float acc);
