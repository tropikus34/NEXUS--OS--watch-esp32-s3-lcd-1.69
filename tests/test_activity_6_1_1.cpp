#include <cassert>
#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
void clearSteps(){memset(activitySteps,0,sizeof(activitySteps));memset(activityDates,0,sizeof(activityDates));undatedSteps=0;activityStatsDirty=false;activityFilterReset();walkObserved=false;}
void feed(float seconds,float amplitude,float hz,bool rotate=false){int samples=(int)(seconds/0.048f);for(int i=0;i<samples;++i){fakeMs+=48;float g=1+amplitude*sinf(i*0.048f*hz*6.2831853f);MotionSample m={fakeMs,rotate?g*0.3f:0,rotate?g*0.4f:0,rotate?g*sqrtf(0.75f):g};activityProcessSample(m);}}
int main(){setup();quietMode=true;doNotDisturb=false;hh=12;mm=0;ss=0;day=27;mon=10;yr=26;timeValid=true;
 clearSteps();feed(15,0.01f,2);assert(activityTodaySteps()==0);
 for(int i=0;i<10;++i){fakeMs+=3000;activityAcceptPeak(fakeMs);}assert(activityTodaySteps()==0);
 clearSteps();feed(2,0,0);feed(50,0.3f,2);uint32_t straight=activityTodaySteps();assert(straight>=96&&straight<=102);
 clearSteps();feed(2,0,0,true);feed(50,0.3f,2,true);assert(activityTodaySteps()==straight);
 clearSteps();for(int i=0;i<40;++i){fakeMs+=100;activityAcceptPeak(fakeMs);}assert(activityTodaySteps()==0); // rapid shaking cannot alias down to a walk
 for(float hz:{0.8f,1.1f,2.8f,3.2f}){clearSteps();feed(2,0,0);feed(40,0.3f,hz);assert(abs((int)activityTodaySteps()-(int)roundf(hz*40))<=2);}
 // Sensor at 21Hz, read on a 40ms scheduler: retain step detection under polling jitter.
 clearSteps();uint32_t start=fakeMs,nextSample=0;
 for(uint32_t elapsed=0;elapsed<52000;elapsed+=MOTION_POLL_MS){
   if(elapsed<nextSample)continue;
   float sampleTime=nextSample/1000.0f;nextSample+=48;fakeMs=start+elapsed;
   float g=sampleTime<2?1:1+0.3f*sinf((sampleTime-2)*2*6.2831853f);
   activityProcessSample(MotionSample{fakeMs,0,0,g});
 }
 assert(activityTodaySteps()>=96&&activityTodaySteps()<=102);
 clearSteps();activityAddSteps(10000,millis());personHeightCm=175;personWeightDeciKg=700;personStrideMm=0;
 assert(fabs(activityDistanceKm()-7.2625f)<0.001f);assert(fabs(activityWalkingKcal()-254.1875f)<0.01f);
 personStrideMm=700;assert(fabs(activityDistanceKm()-7.0f)<0.001f);assert(fabs(activityWalkingKcal()-245.0f)<0.001f);
 personHeightCm=0;personStrideMm=0;assert(activityStrideMm()==0);
 // Midnight and backwards time adjustment select records rather than erase them.
 day=28;assert(activityTodaySteps()==0);activityAddSteps(25,millis());day=27;assert(activityTodaySteps()==10000);day=28;assert(activityTodaySteps()==25);
 timeValid=false;activityAddSteps(8,millis());assert(undatedSteps==8);timeValid=true;day=29;activityService();assert(activityTodaySteps()==8&&undatedSteps==0);
 activitySaveStats(true);auto saved=prefs.data["steps"];clearSteps();activityLoad();assert(activityTodaySteps()==8);
 prefs.data["steps"][10]^=0x20;clearSteps();activityLoad();assert(!activityStatsOK&&activityTodaySteps()==0);prefs.data["steps"]=saved;
 assert(activitySetProfile(180,805,750,10000,45));personHeightCm=0;personWeightDeciKg=0;activityLoad();assert(personHeightCm==180&&personWeightDeciKg==805&&moveIntervalMin==45);
 assert(!activitySetProfile(0,700,0,8000,60));assert(!activitySetProfile(175,700,2000,8000,60));
 auto serialCommand=[](const char* command){for(const char*p=command;*p;++p)Serial.input.push_back(*p);while(Serial.available())themeSerialService();};
 Serial.output.clear();serialCommand("NXHEALTH 175 700 725 9000 60\n");assert(Serial.output.find("OK HEALTH")!=std::string::npos&&personStrideMm==725&&stepGoal==9000);
 activityProfileCursor=2;activityAdjustField(1);assert(personStrideMm==735);activityAdjustField(-1);assert(personStrideMm==725);
 Serial.output.clear();serialCommand("NXHEALTH 0 700 0 8000 60\n");assert(Serial.output.find("ERR HEALTH")!=std::string::npos&&personHeightCm==175);
 Serial.output.clear();serialCommand("NXACTIVITY\n");assert(Serial.output.find("ACTIVITY IMU=")!=std::string::npos);
 // Real I2C transaction path, data decoding and mutex arbitration, using a register device.
 Wire.emulate=true;Wire.devices[0x6b]={};Wire.devices[0x6b][0]=5;Wire.devices[0x6b][0x2e]=1;Wire.devices[0x6b][0x3a]=0x20;
 activityProbeAttempted=false;imuStreaming=false;activityEnabled=true;fakeMs+=10001;motionSamplerOnce();
 assert(imuPresent&&imuStreaming&&activityImuAddress==0x6b);
 assert(Wire.devices[0x6b][2]==0x40&&Wire.devices[0x6b][3]==0x1d&&Wire.devices[0x6b][8]==1);
 MotionSample sample;assert(xQueueReceive(activitySamples,&sample,0)==pdTRUE);assert(fabs(sample.z-1.0f)<0.001f&&sample.x==0);
 uint32_t transactions=Wire.transactions;assert(xSemaphoreTake(activityBusMutex,0)==pdTRUE);assert(!rtcReadTime());assert(Wire.transactions==transactions);xSemaphoreGive(activityBusMutex);
 for(int i=0;i<70;++i){fakeMs+=48;Wire.devices[0x6b][0x2e]=1;motionSamplerOnce();}assert(activityLostSamples>=6&&activitySamples->q.size()==64);
 activityEnabled=false;motionSamplerOnce();assert(!imuStreaming&&Wire.devices[0x6b][8]==0&&Wire.devices[0x6b][2]==0x41);
 activityEnabled=true;activityProbeAttempted=false;Wire.devices[0x6b][0x2e]=1;motionSamplerOnce();assert(imuStreaming);
 Wire.fail=true;for(int i=0;i<3;++i)motionSamplerOnce();assert(!imuPresent&&!imuStreaming);Wire.fail=false;
 Wire.devices.erase(0x6b);Wire.devices[0x6a]={};Wire.devices[0x6a][0]=5;Wire.devices[0x6a][0x2e]=1;Wire.devices[0x6a][0x3a]=0x20;activityProbeAttempted=false;motionSamplerOnce();assert(imuPresent&&activityImuAddress==0x6a&&Wire.devices[0x6a][0x14]==0);
 while(xQueueReceive(activitySamples,&sample,0)==pdTRUE){}
 // Inactivity is based on confirmed walking, requires healthy sampling, and respects hours/DND.
 fakeMs+=4000000;imuPresent=true;activityEnabled=true;activityHaveSample=true;activitySampleAt=millis();walkObserved=true;
 moveIntervalMin=60;moveRemindersEnabled=true;lastWalkAt=millis()-3600001;lastMoveReminderAt=millis()-3600001;hh=12;doNotDisturb=false;moveReminderPending=false;
 activityService();assert(moveReminderPending);alarmRinging=false;timerRinging=false;previewActive=true;movementReminderService();assert(!moveReminderActive&&moveReminderPending);
 previewActive=false;suspendDisplay();movementReminderService();assert(moveReminderActive&&screenAwake&&!lcdSleeping);movementReminderAcknowledge();assert(!moveReminderActive);
 hh=23;activitySampleAt=millis();lastWalkAt=millis()-3600001;lastMoveReminderAt=millis()-3600001;activityService();assert(!moveReminderPending);
 hh=12;doNotDisturb=true;activityService();assert(!moveReminderPending);doNotDisturb=false;activitySampleAt=millis()-3000;activityService();assert(!moveReminderPending&&!walkObserved);
 currentScreen=SC_ACTIVITY;currentMode=MODE_NAV;onLongPress();assert(currentScreen==SC_HEALTH&&getMaxFields()==8);activityProfileCursor=7;onLongPress();assert(currentScreen==SC_ACTIVITY&&currentMode==MODE_NAV);
#if NEXUS_HAPTIC_PIN >= 0
 // Mock driver pin only: verifies pulse pattern without specifying real wiring.
 doNotDisturb=false;alarmRinging=false;timerRinging=false;previewActive=false;moveRemindersEnabled=true;activityEnabled=true;imuPresent=true;
 moveReminderActive=true;moveReminderPending=false;moveReminderStart=millis();movementReminderService();assert(fakeDigitalValues[NEXUS_HAPTIC_PIN]==HIGH);
 fakeMs+=170;movementReminderService();assert(fakeDigitalValues[NEXUS_HAPTIC_PIN]==LOW);
 fakeMs+=100;movementReminderService();assert(fakeDigitalValues[NEXUS_HAPTIC_PIN]==HIGH);
 movementReminderAcknowledge();assert(fakeDigitalValues[NEXUS_HAPTIC_PIN]==LOW);
#else
 assert(fakeDigitalValues.count(17)==0);
#endif
 puts("PASS: stationary/isolated/rapid motion rejected; walking/orientation, distance/calories, midnight/rewind, checkpoint CRC, profile, IMU registers/decoding/mutex/overflow/failure, reminders and UI navigation");
}
