#include "tmc_init.h"
#include "pd_pins.h"
#include <Arduino.h>

HardwareSerial& SER = Serial2;
TMC2209 stepper;

void TMC_enable(bool en){
  pinMode(PIN_TMC_EN, OUTPUT);
  digitalWrite(PIN_TMC_EN, en?LOW:HIGH);
}

void PD_request_12V(){
  pinMode(PIN_CFG1, OUTPUT);
  pinMode(PIN_CFG2, OUTPUT);
  pinMode(PIN_CFG3, OUTPUT);
  // 12 V: CFG1 LOW, CFG2 LOW, CFG3 HIGH
  digitalWrite(PIN_CFG1, LOW);
  digitalWrite(PIN_CFG2, LOW);
  digitalWrite(PIN_CFG3, HIGH);
}

bool PD_powerGood(){
  pinMode(PIN_PG, INPUT_PULLUP);
  return digitalRead(PIN_PG)==LOW;
}

void TMC_setThresholds(){
  // Keep StallGuard valid at speed: set PWM threshold and coolStep duration.
  // Values are starting points. You will tune them.
  stepper.setPwmThreshold(300);            // boundary speed for stealth->spread
  stepper.setCoolStepDurationThreshold(5000);  // TCOOLTHRS-like
}

void TMC_setDynamic(const MotionParams& mp){
  stepper.setRunCurrent(mp.runCurrent_mA);
  stepper.setMicrostepsPerStep(mp.microsteps);
  stepper.setStallGuardThreshold(mp.stallThreshold);
  if (mp.stealthChop) stepper.enableStealthChop(); else stepper.disableStealthChop();
}

void TMC_init(const MotionParams& mp){
  // pins
  pinMode(PIN_STEP, OUTPUT); digitalWrite(PIN_STEP, LOW);
  pinMode(PIN_DIR, OUTPUT);  digitalWrite(PIN_DIR, LOW);
  pinMode(PIN_MS1, OUTPUT);  digitalWrite(PIN_MS1, LOW);
  pinMode(PIN_MS2, OUTPUT);  digitalWrite(PIN_MS2, LOW);
  pinMode(PIN_SPREAD, OUTPUT); digitalWrite(PIN_SPREAD, LOW);
  pinMode(PIN_DIAG, INPUT_PULLUP);
  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);

  SER.begin(115200, SERIAL_8N1, PIN_TMC_RX, PIN_TMC_TX);
  stepper.setup(SER, 115200, TMC2209::SERIAL_ADDRESS_0, PIN_TMC_RX, PIN_TMC_TX);

  stepper.enableAutomaticCurrentScaling();
  TMC_setDynamic(mp);
  TMC_setThresholds();

  // disabled at boot
  TMC_enable(false);
}
