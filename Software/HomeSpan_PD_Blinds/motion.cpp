#include "motion.h"
#include "as5600.h"
#include "tmc_init.h"
#include "pd_pins.h"
#include <Preferences.h>

Preferences prefs;
MotionParams g_mp;
Limits g_limits;

static volatile bool stallIRQ=false;
static void IRAM_ATTR onDiagISR(){ stallIRQ = true; }

static float pos_c=0, vel_cps=0, acc_cps2=0;
static float setpoint_c = 0;

static int percentFromCounts(int32_t c){
  if (!g_limits.set || g_limits.bottom==g_limits.top) return 0;
  float span=(float)(g_limits.bottom - g_limits.top);
  float pct=100.0f*(float)(c - g_limits.top)/span;
  if (pct<0) pct=0; if (pct>100) pct=100;
  return (int)lroundf(pct);
}
static int32_t countsFromPercent(int pct){
  if (!g_limits.set) return AS5600_totalCounts;
  pct = constrain(pct,0,100);
  float span=(float)(g_limits.bottom - g_limits.top);
  return (int32_t)lroundf((float)g_limits.top + span*(pct/100.0f));
}

void Motion_saveNVS(){
  prefs.begin(NVS_NS,false);
  prefs.putInt("top", g_limits.top);
  prefs.putInt("bottom", g_limits.bottom);
  prefs.putBool("set", g_limits.set);
  prefs.putUShort("micro", g_mp.microsteps);
  prefs.putUShort("curr", g_mp.runCurrent_mA);
  prefs.putChar("stallT", g_mp.stallThreshold);
  prefs.putFloat("vmax", g_mp.maxVel_cps);
  prefs.putFloat("amax", g_mp.maxAcc_cps2);
  prefs.putFloat("jmax", g_mp.maxJerk_cps3);
  prefs.putBool("stealth", g_mp.stealthChop);
  prefs.putUChar("pdVolt", (uint8_t)g_mp.pdVoltage);
  prefs.end();
}

void Motion_loadNVS(){
  prefs.begin(NVS_NS,true);
  g_limits.top    = prefs.getInt("top", 0);
  g_limits.bottom = prefs.getInt("bottom", g_limits.top+100000);
  g_limits.set    = prefs.getBool("set", false);
  g_mp.microsteps     = prefs.getUShort("micro", g_mp.microsteps);
  g_mp.runCurrent_mA  = prefs.getUShort("curr", g_mp.runCurrent_mA);
  g_mp.stallThreshold = prefs.getChar("stallT", g_mp.stallThreshold);
  g_mp.maxVel_cps     = prefs.getFloat("vmax", g_mp.maxVel_cps);
  g_mp.maxAcc_cps2    = prefs.getFloat("amax", g_mp.maxAcc_cps2);
  g_mp.maxJerk_cps3   = prefs.getFloat("jmax", g_mp.maxJerk_cps3);
  g_mp.stealthChop    = prefs.getBool("stealth", g_mp.stealthChop);
  g_mp.pdVoltage      = (PDVoltage)prefs.getUChar("pdVolt", g_mp.pdVoltage);
  prefs.end();
}

void Motion_begin(){
  attachInterrupt(digitalPinToInterrupt(PIN_DIAG), onDiagISR, RISING);
  AS5600_begin(PIN_SDA, PIN_SCL);
  AS5600_readTotal();
  pos_c = (float)AS5600_totalCounts;
  setpoint_c = pos_c;
}

void Motion_setTargetPercent(int pct){
  setpoint_c = (float)countsFromPercent(pct);
}

int Motion_getCurrentPercent(){
  return percentFromCounts(AS5600_totalCounts);
}

static void integrateTowards(float target_c, float dt){
  float err = target_c - pos_c;
  float wantVel = constrain(err, -g_mp.maxVel_cps, g_mp.maxVel_cps);
  float maxDa = g_mp.maxJerk_cps3 * dt;
  float wantAcc = constrain((wantVel - vel_cps)/dt, -g_mp.maxAcc_cps2, g_mp.maxAcc_cps2);
  float dAcc = constrain(wantAcc - acc_cps2, -maxDa, maxDa);
  acc_cps2 += dAcc;
  vel_cps  += acc_cps2 * dt;
  vel_cps   = constrain(vel_cps, -g_mp.maxVel_cps, g_mp.maxVel_cps);
  pos_c    += vel_cps * dt;

  static float resid=0.0f;
  float countsMoved = vel_cps*dt + resid;
  int steps = (int)floor(fabs(countsMoved));
  resid = countsMoved - (countsMoved>=0 ? steps : -steps);

  bool dirPos = (countsMoved>=0);
  digitalWrite(PIN_DIR, dirPos?HIGH:LOW);
  for(int i=0;i<steps;i++){
    digitalWrite(PIN_STEP, HIGH); delayMicroseconds(2);
    digitalWrite(PIN_STEP, LOW);  delayMicroseconds(2);
  }
}

void Motion_loop(float dt_ms, bool powerGood, bool& stallFlag){
  if (!powerGood || stallIRQ){
    stallFlag = stallIRQ; stallIRQ=false;
    TMC_enable(false);
    acc_cps2=0; vel_cps=0;
    delay(10);
    return;
  }

  int32_t tgt = (int32_t)lroundf(setpoint_c);
  AS5600_readTotal();
  pos_c = (float)AS5600_totalCounts;

  float err = (float)tgt - pos_c;
  if (fabs(err)>2.0f){
    TMC_setDynamic(g_mp);
    TMC_enable(true);
    integrateTowards((float)tgt, dt_ms/1000.0f);
  } else {
    TMC_enable(false);
    acc_cps2=0; vel_cps=0;
  }
}

void Motion_halt(){
  TMC_enable(false);
  acc_cps2=0; vel_cps=0;
}

void Motion_serialCLI() {
  // Only react to lines that START WITH ':' so we don't steal HomeSpan console input (W, ?)
  if (!Serial.available()) return;
  if (Serial.peek() != ':') return;

  Serial.read();  // consume leading ':'

  String line;
  unsigned long t0 = millis();
  bool gotLine = false;

  // Read up to 1s for a single line terminated by NL or CR
  while (millis() - t0 < 1000 && !gotLine) {
    while (Serial.available()) {
      char c = (char)Serial.read();
      if (c == 10 || c == 13) {  // '\n' or '\r' (numeric to avoid quoting issues)
        gotLine = true;
        break;
      }
      line += c;
    }
    delay(2);
  }

  line.trim();
  if (!line.length()) return;

  char cmd[16]; float val = 0;
  if (sscanf(line.c_str(), "%15s %f", cmd, &val) == 2) {
    if      (!strcmp(cmd, "cur"))     g_mp.runCurrent_mA  = (uint16_t)val;
    else if (!strcmp(cmd, "vel"))     g_mp.maxVel_cps     = val;
    else if (!strcmp(cmd, "acc"))     g_mp.maxAcc_cps2    = val;
    else if (!strcmp(cmd, "jerk"))    g_mp.maxJerk_cps3   = val;
    else if (!strcmp(cmd, "micro"))   g_mp.microsteps     = (uint16_t)val;
    else if (!strcmp(cmd, "stealth")) g_mp.stealthChop    = (val > 0.5f);
    else if (!strcmp(cmd, "stall"))   g_mp.stallThreshold = (int8_t)val;
    else {
      Serial.println(F("Unknown cmd. Use :cur/:vel/:acc/:jerk/:micro/:stealth/:stall"));
      return;
    }
    TMC_setDynamic(g_mp);
    Motion_saveNVS();
    Serial.println(F("OK"));
  } else {
    Serial.println(F("Usage: :cur 700 | :vel 2500 | :acc 8000 | :jerk 50000 | :micro 32 | :stealth 1 | :stall 10"));
  }
}

void Motion_home(bool& stallFlag){
  if (!PD_powerGood()) return;

  // homing speed high enough for spreadCycle
  g_mp.stealthChop = false;   // force spreadCycle for SG
  TMC_setDynamic(g_mp);
  TMC_enable(true);

  unsigned long t0=millis();
  stallFlag=false;
  acc_cps2=0; vel_cps=0;

  // drive upward (toward smaller counts)
  for(;;){
    if ((millis()-t0) > HOMING_TIMEOUT_MS) break;
    if (!PD_powerGood()) break;
    if (stallIRQ){ stallFlag=true; stallIRQ=false; break; }

    // push target far above current to keep moving up
    setpoint_c = pos_c - 20000.0f;
    integrateTowards(setpoint_c, 0.003f);
    delay(3);
    AS5600_readTotal(); pos_c = (float)AS5600_totalCounts;
  }

  // stop, set TOP
  Motion_halt();
  AS5600_readTotal();
  g_limits.top = AS5600_totalCounts;
  Motion_saveNVS();

  // restore stealth choice
  g_mp.stealthChop = true;
  TMC_setDynamic(g_mp);
}

void Motion_homeMin(bool& stallFlag){
  if (!PD_powerGood()) return;

  // homing speed high enough for spreadCycle
  g_mp.stealthChop = false;   // force spreadCycle for SG
  TMC_setDynamic(g_mp);
  TMC_enable(true);

  unsigned long t0=millis();
  stallFlag=false;
  acc_cps2=0; vel_cps=0;

  // drive downward (toward larger counts) to find min position
  for(;;){
    if ((millis()-t0) > HOMING_TIMEOUT_MS) break;
    if (!PD_powerGood()) break;
    if (stallIRQ){ stallFlag=true; stallIRQ=false; break; }

    // push target far below current to keep moving down
    setpoint_c = pos_c + 20000.0f;
    integrateTowards(setpoint_c, 0.003f);
    delay(3);
    AS5600_readTotal(); pos_c = (float)AS5600_totalCounts;
  }

  // stop, set BOTTOM (min position)
  Motion_halt();
  AS5600_readTotal();
  g_limits.bottom = AS5600_totalCounts;
  if (!g_limits.set) {
    // If limits not set yet, initialize top to current position
    g_limits.top = AS5600_totalCounts;
  }
  g_limits.set = true;
  Motion_saveNVS();

  // restore stealth choice
  g_mp.stealthChop = true;
  TMC_setDynamic(g_mp);
}

float Motion_getVelocity(){
  return g_mp.maxVel_cps;
}

float Motion_getAcceleration(){
  return g_mp.maxAcc_cps2;
}

void Motion_setVelocity(float vel){
  g_mp.maxVel_cps = vel;
  TMC_setDynamic(g_mp);
  Motion_saveNVS();
}

void Motion_setAcceleration(float acc){
  g_mp.maxAcc_cps2 = acc;
  TMC_setDynamic(g_mp);
  Motion_saveNVS();
}
