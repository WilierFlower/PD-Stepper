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
void Motion_saveNVS();
void Motion_loadNVS();

void Motion_serialCLI();  // parses and persists changes
