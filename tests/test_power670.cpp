#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
#include <cassert>
int main(){setup();powerAwakeMs=powerOffMs=0;screenAwake=true;powerSampleAt=millis();fakeMs+=1000;powerAccounting();assert(powerAwakeMs==1000&&powerOffMs==0);
 ecoMode=false;suspendDisplay();assert(!screenAwake&&lcdSleeping&&lcd->displaySleeping);fakeMs+=5000;powerAccounting();assert(powerOffMs==5000);wakeDisplay();assert(screenAwake&&!lcdSleeping&&!lcd->displaySleeping);
 powerSampleAt=0xfffffff0;fakeMs=0x20;auto before=powerAwakeMs;powerAccounting();assert(powerAwakeMs-before==48);
 Serial.output.clear();studioPrintConfig();assert(Serial.output.find("\"awakeMs\"")!=std::string::npos);
 ecoMode=true;brightness=128;screenTimeoutIndex=0;screenAwake=true;screenDimmed=false;lastActivity=millis();fakeMs+=11999;backlightService();assert(!screenDimmed);fakeMs+=1;backlightService();assert(screenDimmed&&fakeBacklight==32);
 lastActivity=millis();backlightService();assert(!screenDimmed&&fakeBacklight==128);
 fakeMs+=13000;alarmRinging=true;backlightService();assert(!screenDimmed);alarmRinging=false;backlightService();assert(screenDimmed);wakeDisplay();assert(!screenDimmed&&fakeBacklight==128);
 screenTimeoutIndex=4;fakeMs+=100000;backlightService();assert(!screenDimmed);
 auto& server=NimBLEDevice::server;server.count=0;blePowerService();server.count=1;screenAwake=false;fakeMs+=5000;blePowerService();assert(server.updates==1&&server.minInterval==80&&server.maxInterval==160&&server.latency==4&&server.timeout==600);fakeMs+=6000;blePowerService();assert(server.updates==1);screenAwake=true;blePowerService();assert(server.updates==2&&server.minInterval==24&&server.latency==0);
 assert(!configureAutoSleep(80,true)&&!autoSleepConfigured&&autoSleepError==-1);
 puts("PASS POWER: dim at 3 seconds, activity/alert/timeout restore, BLE requests/rate limiting, unsupported-PM fallback; awake/off accounting, wraparound, LCD sleep outside ECO and wake, readable telemetry");
}
