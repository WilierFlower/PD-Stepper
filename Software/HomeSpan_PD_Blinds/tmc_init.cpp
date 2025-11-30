#include "tmc_init.h"
#include "pd_pins.h"
#include "config.h"
#include <Arduino.h>

HardwareSerial& SER = Serial2;
TMC2209 stepper;

void TMC_enable(bool en){
  pinMode(PIN_TMC_EN, OUTPUT);
  digitalWrite(PIN_TMC_EN, en?LOW:HIGH);
}

void PD_request_12V(){
  PD_setVoltage(PD_12V);
}

void PD_setVoltage(PDVoltage voltage){
  pinMode(PIN_CFG1, OUTPUT);
  pinMode(PIN_CFG2, OUTPUT);
  pinMode(PIN_CFG3, OUTPUT);
  
  // USB-PD voltage selection via CFG pins
  // Common pattern: CFG1, CFG2, CFG3 combinations
  // Note: Actual pin mapping may vary by board design
  switch(voltage){
    case PD_5V:
      // 5V: CFG1 LOW, CFG2 LOW, CFG3 LOW
      digitalWrite(PIN_CFG1, LOW);
      digitalWrite(PIN_CFG2, LOW);
      digitalWrite(PIN_CFG3, LOW);
      break;
    case PD_9V:
      // 9V: CFG1 HIGH, CFG2 LOW, CFG3 LOW
      digitalWrite(PIN_CFG1, HIGH);
      digitalWrite(PIN_CFG2, LOW);
      digitalWrite(PIN_CFG3, LOW);
      break;
    case PD_12V:
      // 12V: CFG1 LOW, CFG2 LOW, CFG3 HIGH
      digitalWrite(PIN_CFG1, LOW);
      digitalWrite(PIN_CFG2, LOW);
      digitalWrite(PIN_CFG3, HIGH);
      break;
    case PD_15V:
      // 15V: CFG1 HIGH, CFG2 LOW, CFG3 HIGH
      digitalWrite(PIN_CFG1, HIGH);
      digitalWrite(PIN_CFG2, LOW);
      digitalWrite(PIN_CFG3, HIGH);
      break;
    case PD_20V:
      // 20V: CFG1 LOW, CFG2 HIGH, CFG3 HIGH
      digitalWrite(PIN_CFG1, LOW);
      digitalWrite(PIN_CFG2, HIGH);
      digitalWrite(PIN_CFG3, HIGH);
      break;
    default:
      // Default to 12V
      digitalWrite(PIN_CFG1, LOW);
      digitalWrite(PIN_CFG2, LOW);
      digitalWrite(PIN_CFG3, HIGH);
      break;
  }
  
  // Wait for PD negotiation
  delay(100);
}

float PD_measureVoltage(){
  // Measure voltage via ADC if available
  // PIN_VBUS_ADC should be connected to a voltage divider
  // Typical divider: VBUS -> 10k -> ADC -> 10k -> GND (1:2 ratio)
  // For 20V max: ADC reads 10V max, ESP32 ADC is 3.3V max, so need different divider
  // Assuming a 1:6 divider for 20V -> 3.33V max at ADC
  pinMode(PIN_VBUS_ADC, INPUT);
  int adcValue = analogRead(PIN_VBUS_ADC);
  // ESP32-S3 ADC is 12-bit (0-4095) for 0-3.3V
  float adcVoltage = (adcValue / 4095.0f) * 3.3f;
  // Assuming 1:6 voltage divider (adjust based on actual hardware)
  float vbusVoltage = adcVoltage * 6.0f;
  return vbusVoltage;
}

bool PD_powerGood(){
  pinMode(PIN_PG, INPUT_PULLUP);
  return digitalRead(PIN_PG)==LOW;
}

void TMC_setThresholds(){
  // No-op: this TMC2209 lib lacks PWM/coolstep threshold helpers.
  // StallGuard homing still works because Motion_home() forces spreadCycle (stealthChop=false).
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
