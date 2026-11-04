#pragma once
// Included after application state and settings; owns power policy and display wake/sleep.
uint64_t powerAwakeMs=0,powerOffMs=0;uint32_t powerSampleAt=0;
void powerAccounting(){uint32_t now=millis(),dt=now-powerSampleAt;powerSampleAt=now;if(screenAwake)powerAwakeMs+=dt;else powerOffMs+=dt;}
// PM support is a property of the compiled core, never inferred from a screen being off.
bool autoSleepConfigured=false;int autoSleepError=-1;bool screenDimmed=false;
uint32_t powerMinMHz=0,powerMaxMHz=0;
int bleAdvertisingMode=-1;uint32_t bleLastConnectedAt=0;
int blePowerMode=-1;uint32_t blePowerAt=0;
uint32_t powerSerialAt=0;bool powerSerialSeen=false;int lastOffSleepConfigured=-1;
#if defined(ESP32) && defined(CONFIG_PM_ENABLE) && CONFIG_PM_ENABLE && ESP_IDF_VERSION_MAJOR >= 5
constexpr bool powerPmBuilt=true;
#else
constexpr bool powerPmBuilt=false;
#endif
#if defined(CONFIG_FREERTOS_USE_TICKLESS_IDLE) && CONFIG_FREERTOS_USE_TICKLESS_IDLE
constexpr bool powerTicklessBuilt=true;
#else
constexpr bool powerTicklessBuilt=false;
#endif
#if defined(CONFIG_BT_CTRL_MODEM_SLEEP) && CONFIG_BT_CTRL_MODEM_SLEEP
constexpr bool powerBtSleepBuilt=true;
#else
constexpr bool powerBtSleepBuilt=false;
#endif
bool powerSerialBusy(){
#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
 if(Serial)return true; // USB CDC session only, never HardwareSerial readiness.
#endif
 return powerSerialSeen&&(uint32_t)(millis()-powerSerialAt)<10000;
}
void powerSerialActivity(){powerSerialSeen=true;powerSerialAt=millis();}
const char* powerSleepReason(){
 if(!powerPmBuilt)return "PM_NOT_BUILT";
 if(!powerTicklessBuilt)return "TICKLESS_NOT_BUILT";
 if(!powerBtSleepBuilt)return "BT_MODEM_NOT_BUILT";
 if(!ecoMode)return "ECO_OFF";
 if(themeReceiving)return "UPLOAD";
 if(screenAwake)return "SCREEN_ON";
 if(powerSerialBusy())return "SERIAL_BUSY";
 if(autoSleepError!=0)return "PM_ERROR";
 return autoSleepConfigured?"CONFIGURED_NOT_MEASURED":"NOT_CONFIGURED";
}
void printPowerStatus(){
 powerAccounting();
 Serial.printf("POWER PM=%d TICKLESS=%d BT_MODEM=%d REASON=%s CONFIGURED=%d LAST_OFF_CONFIGURED=%d ERROR=%d CPU=%lu LCD_SLEEP=%d ECO=%d RAW_V=%.3f FILTERED_V=%.3f SAMPLE_AGE_MS=%lu DFS_MIN=%lu DFS_MAX=%lu ADV_MODE=%d\n",powerPmBuilt,powerTicklessBuilt,powerBtSleepBuilt,powerSleepReason(),autoSleepConfigured,lastOffSleepConfigured,autoSleepError,(unsigned long)getCpuFrequencyMhz(),lcdSleeping,ecoMode,batteryRawVolts,batteryVolts,(unsigned long)(millis()-batterySampleAt),(unsigned long)powerMinMHz,(unsigned long)powerMaxMHz,bleAdvertisingMode);
}
bool configureAutoSleep(uint32_t mhz,bool idle){
#if defined(ESP32) && defined(CONFIG_PM_ENABLE) && CONFIG_PM_ENABLE && ESP_IDF_VERSION_MAJOR >= 5
 esp_pm_config_t cfg={};cfg.max_freq_mhz=mhz;cfg.min_freq_mhz=(idle&&!powerSerialBusy())?40:mhz;cfg.light_sleep_enable=false;
#if defined(CONFIG_FREERTOS_USE_TICKLESS_IDLE) && CONFIG_FREERTOS_USE_TICKLESS_IDLE && defined(CONFIG_BT_CTRL_MODEM_SLEEP) && CONFIG_BT_CTRL_MODEM_SLEEP
 // USB studio must remain responsive. BLE/controller locks still decide whether sleep occurs.
 cfg.light_sleep_enable=idle && !powerSerialBusy();
#endif
 (void)idle;autoSleepError=esp_pm_configure(&cfg);autoSleepConfigured=autoSleepError==ESP_OK&&cfg.light_sleep_enable;
 if(autoSleepError==ESP_OK){powerMinMHz=cfg.min_freq_mhz;powerMaxMHz=cfg.max_freq_mhz;}
 return autoSleepError==ESP_OK;
#else
 (void)mhz;(void)idle;autoSleepConfigured=false;autoSleepError=-1;return false;
#endif
}
void blePowerService(){
 auto* server=NimBLEDevice::getServer();
 if(!server||!server->getConnectedCount()){blePowerMode=-1;blePowerAt=millis();return;}
 int mode=ecoMode&&!screenAwake&&!themeReceiving?1:0;
 if(mode==blePowerMode||(uint32_t)(millis()-blePowerAt)<5000)return;
 auto peer=server->getPeerInfo((uint8_t)0);
 // 30–60ms active; 100–200ms with latency 4 idle. Phone may reject the request.
 server->updateConnParams(peer.getConnHandle(),mode?80:24,mode?160:48,mode?4:0,600);
 blePowerMode=mode;blePowerAt=millis();
}
void powerService() {
  powerAccounting();
  static uint32_t requested=0,lastAttempt=0;static bool previousIdle=false;
  uint32_t want=ecoMode ? (screenAwake || themeReceiving ? 160u:80u):240u;
  bool idle=ecoMode&&!screenAwake&&!themeReceiving;
#if defined(ESP32)
 idle=idle&&!powerSerialBusy();
#endif
 if((want!=requested||idle!=previousIdle) && (powerClockOK||(uint32_t)(millis()-lastAttempt)>=1000)) {
  lastAttempt=millis();bool managed=configureAutoSleep(want,idle);
  powerClockOK=managed||setCpuFrequencyMhz(want);
  if(!managed&&powerClockOK){powerMinMHz=want;powerMaxMHz=want;}
  if(powerClockOK){requested=want;previousIdle=idle;}
 }
 if(!screenAwake&&!powerSerialBusy())lastOffSleepConfigured=autoSleepConfigured?1:0;
}
// 6.9.4: panel uses SLPIN/SLPOUT (not only DISPOFF), backlight PWM is fully released
// while off (LEDC on APB blocks light sleep), and the fresh frame reaches GRAM
// BEFORE the panel/backlight turn on, so the stale pre-sleep frame is never visible.
void redrawCurrentScreen();void advanceSecond();void alarmCheck();
extern uint32_t lastTick;
static void lcdCommand(uint8_t c){bus->beginWrite();bus->writeCommand(c);bus->endWrite();}
static void backlightRelease(){
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
 ledcDetach(LCD_BL);
#endif
 pinMode(LCD_BL,OUTPUT);digitalWrite(LCD_BL,LOW);gpio_hold_en((gpio_num_t)LCD_BL);
}
static void backlightSet(int level){gpio_hold_dis((gpio_num_t)LCD_BL);analogWrite(LCD_BL,level);}
static void clockCatchUp(){uint32_t cur=millis();while((uint32_t)(cur-lastTick)>=1000){lastTick+=1000;advanceSecond();alarmCheck();}}
void wakeDisplay() {
  // An external wake/overlay must not replay an earlier click or held press.
  cancelPendingButtonAction();
  powerAccounting();
  bool wasOff=!screenAwake||lcdSleeping;
  screenAwake=true;powerService(); // full clock before composing
  if(wasOff){
    if(lcdSleeping){lcdCommand(0x11);delay(120);} // SLPOUT, panel still DISPOFF + backlight off
    clockCatchUp();
    redrawCurrentScreen();displayPending=true;displayPresent(); // current frame into GRAM
    if(lcdSleeping){lcd->displayOn();lcdSleeping=false;}
    uiDirty=false;
  } else uiDirty=true;
  lastActivity=millis();screenDimmed=false;backlightSet(brightness); // light last
}
void suspendDisplay() {
  powerAccounting();screenAwake=false;backlightRelease();
  if(!lcdSleeping) {lcd->displayOff();lcdCommand(0x10);lcdSleeping=true;} // DISPOFF + SLPIN
  powerService();
}

// Called only while disconnected. Keep a fast discovery period after link loss.
void bleAdvertisingPowerService(){
 auto* advertising=NimBLEDevice::getAdvertising();if(!advertising)return;
 int mode=ecoMode&&!screenAwake&&!pairWindowActive&&(uint32_t)(millis()-bleLastConnectedAt)>=30000?1:0;
 if(mode==bleAdvertisingMode)return;
 // Parameters apply on start; never repeatedly stop an unchanged advertisement.
 if(advertising->isAdvertising()&&!advertising->stop())return;
 advertising->setMinInterval(mode?1600:160); // 1s or 100ms, units 0.625ms
 advertising->setMaxInterval(mode?2000:240); // 1.25s or 150ms
 bleAdvertisingMode=mode;
}
