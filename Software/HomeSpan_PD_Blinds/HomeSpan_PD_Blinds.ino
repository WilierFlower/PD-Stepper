#include <Arduino.h>
#include <HomeSpan.h>
#include "pd_pins.h"
#include "config.h"
#include "tmc_init.h"
#include "as5600.h"
#include "motion.h"

// HomeKit service wrapper
struct BlindsService : Service::WindowCovering {
  Characteristic::CurrentPosition cur{0};
  Characteristic::TargetPosition  tgt{0};
  Characteristic::PositionState   st{2};  // 0=DECR,1=INCR,2=STOP
  Characteristic::HoldPosition    hold;

  // Aux switches
  SpanService *homingSw; Characteristic::On *homingOn;
  SpanService *setBotSw; Characteristic::On *setBotOn;

  BlindsService(){
    homingSw = new Service::Switch();
    homingOn = new Characteristic::On(false);
    setBotSw = new Service::Switch();
    setBotOn = new Characteristic::On(false);
  }

  boolean update() override {
    if (tgt.updated()){
      int t = constrain(tgt.getNewVal(),0,100);
      tgt.setVal(t);
      Motion_setTargetPercent(t);
    }
    if (hold.updated() && hold.getNewVal()){
      Motion_halt();
      hold.setVal(false);
      st.setVal(2);
    }
    if (homingOn->updated() && homingOn->getNewVal()){
      homingOn->setVal(false);
      bool sg=false; Motion_home(sg);
      cur.setVal(Motion_getCurrentPercent());
      st.setVal(2);
    }
    if (setBotOn->updated() && setBotOn->getNewVal()){
      setBotOn->setVal(false);
      // set bottom at current encoder
      g_limits.bottom = AS5600_totalCounts;
      g_limits.set = true;
      Motion_saveNVS();
      cur.setVal(Motion_getCurrentPercent());
    }
    return true;
  }

  void loop() override {
    // UI state
    int cp = Motion_getCurrentPercent();
    int tp = tgt.getVal();
    if (tp>cp) st.setVal(1);
    else if (tp<cp) st.setVal(0);
    else st.setVal(2);
    cur.setVal(cp);

    // run motion
    static bool stall=false;
    Motion_loop(5.0f, PD_powerGood(), stall);
    if (stall){ st.setVal(2); stall=false; }
    delay(5);
  }
} *svc;

void setup(){
  Serial.begin(115200);
  delay(150);

  // Power and driver setup
  PD_request_12V();
  TMC_init(g_mp);
  Motion_loadNVS();
  Motion_begin();

  // HomeSpan
  homeSpan.begin(Category::WindowCoverings,"PD-Stepper Blinds");
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("PD-Blinds");
      new Characteristic::Manufacturer("You");
      new Characteristic::Model("PD-Stepper");
      new Characteristic::SerialNumber("0001");
      new Characteristic::FirmwareRevision("0.1");
      new Characteristic::Identify();
    svc = new BlindsService();

  Serial.println("\nCommands: cur/vel/acc/jerk/micro/stealth/stall");
}

void loop(){
  homeSpan.poll();
  Motion_serialCLI();
}
