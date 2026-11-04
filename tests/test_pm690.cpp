#define ESP32 1
#ifndef ARDUINO_USB_CDC_ON_BOOT
#define ARDUINO_USB_CDC_ON_BOOT 1
#endif
#define CONFIG_PM_ENABLE 1
#define CONFIG_FREERTOS_USE_TICKLESS_IDLE 1
#define CONFIG_BT_CTRL_MODEM_SLEEP 1
#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
int main(){
 Serial.attached=false;
 assert(configureAutoSleep(80,true));assert(autoSleepConfigured&&fakePmConfig.light_sleep_enable&&fakePmConfig.min_freq_mhz==40);
 Serial.attached=true;assert(configureAutoSleep(80,true));
#if ARDUINO_USB_CDC_ON_BOOT
 assert(!autoSleepConfigured&&!fakePmConfig.light_sleep_enable);
#else
 assert(autoSleepConfigured&&fakePmConfig.light_sleep_enable); // Initialized UART is not a USB cable.
#endif
 Serial.attached=false;powerSerialActivity();assert(configureAutoSleep(80,true));assert(!autoSleepConfigured);fakeMs+=10000;assert(configureAutoSleep(80,true));assert(autoSleepConfigured);
 Serial.attached=false;assert(configureAutoSleep(160,false));assert(!autoSleepConfigured&&fakePmConfig.max_freq_mhz==160);
 fakePmError=258;ecoMode=true;screenAwake=false;fakeMs=10000;powerService();assert(!autoSleepConfigured&&autoSleepError==258&&getCpuFrequencyMhz()==80);
 fakePmError=0;screenAwake=true;powerService();assert(!autoSleepConfigured&&fakePmConfig.max_freq_mhz==160);
 puts("PASS PM: supported/unsupported runtime configure, USB inhibits sleep, active mode disables sleep, failure falls back to manual clock");
}
