#pragma once
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <cmath>
#include <atomic>
#include <initializer_list>
// QMI8658C: WHO=05, +/-4g, accelerometer-only 21Hz low-power; gyroscope off.
#ifndef NEXUS_HAPTIC_PIN
#define NEXUS_HAPTIC_PIN -1
#endif
#if NEXUS_HAPTIC_PIN >= 0
static_assert(NEXUS_HAPTIC_PIN<=48 && NEXUS_HAPTIC_PIN!=0 && NEXUS_HAPTIC_PIN!=1 && NEXUS_HAPTIC_PIN!=4 && NEXUS_HAPTIC_PIN!=5 && NEXUS_HAPTIC_PIN!=6 && NEXUS_HAPTIC_PIN!=7 && NEXUS_HAPTIC_PIN!=8 && NEXUS_HAPTIC_PIN!=10 && NEXUS_HAPTIC_PIN!=11 && NEXUS_HAPTIC_PIN!=15 && NEXUS_HAPTIC_PIN!=16 && NEXUS_HAPTIC_PIN!=38 && NEXUS_HAPTIC_PIN!=39 && NEXUS_HAPTIC_PIN!=40 && NEXUS_HAPTIC_PIN!=45 && NEXUS_HAPTIC_PIN!=46 && NEXUS_HAPTIC_PIN!=19 && NEXUS_HAPTIC_PIN!=20 && NEXUS_HAPTIC_PIN!=41 && NEXUS_HAPTIC_PIN!=42 && (NEXUS_HAPTIC_PIN<26 || NEXUS_HAPTIC_PIN>37),"Haptic pin conflicts with display, boot, battery, USB, memory or power. Choose a verified free pin and motor driver.");
#endif
struct MotionSample {uint32_t at;float x,y,z;};
SemaphoreHandle_t activityBusMutex=nullptr;
QueueHandle_t activitySamples=nullptr;
TaskHandle_t activityTask=nullptr;
std::atomic<bool> activityEnabled{true},imuPresent{false},imuStreaming{false},activityHaveSample{false};
std::atomic<uint32_t> activitySampleAt{0},activityLostSamples{0};
bool activityTaskOK=false,activityStatsDirty=false,activityStatsOK=true;
uint8_t activityImuAddress=0,activitySensitivity=1;
uint16_t personHeightCm=0,personWeightDeciKg=0,personStrideMm=0,moveIntervalMin=60;
uint32_t stepGoal=8000,activitySteps[7]={},activityDates[7]={},undatedSteps=0;
bool moveRemindersEnabled=true,moveReminderPending=false,moveReminderActive=false;
uint32_t moveReminderStart=0,lastWalkAt=0,lastMoveReminderAt=0,activityLastSave=0;
bool walkObserved=false;
int activityProfileCursor=0;
float activityGravity=1.0f,activityFiltered=0,activityPrevious=0;
bool activityFilterReady=false,activityPeakArmed=true;
uint16_t activityWarmSamples=0;
uint32_t activityPreviousAt=0,activityPeakAt=0,activityInterval=0;
uint8_t activityCadenceCount=0;
uint8_t activityI2CErrors=0;
uint32_t activityProbeAt=0;
std::atomic<bool> activityProbeAttempted{false};
void activitySaveStats(bool force=false);
void activityFilterReset() {activityFilterReady=false;activityWarmSamples=0;activityPreviousAt=0;activityCadenceCount=0;activityInterval=0;activityPeakAt=0;activityPeakArmed=true;activityFiltered=0;activityPrevious=0;}
class ActivityBusLock {
public:
 bool held;
 ActivityBusLock():held(!activityBusMutex || xSemaphoreTake(activityBusMutex,pdMS_TO_TICKS(50))==pdTRUE){}
 ~ActivityBusLock(){if(held && activityBusMutex)xSemaphoreGive(activityBusMutex);}
 explicit operator bool()const{return held;}
};
bool i2cProbe(uint8_t address) {ActivityBusLock lock;if(!lock)return false;Wire.beginTransmission(address);return Wire.endTransmission()==0;}
bool motionRead(uint8_t reg,uint8_t* data,uint8_t count) {
 Wire.beginTransmission(activityImuAddress);Wire.write(reg);
 if(Wire.endTransmission(false)!=0 || Wire.requestFrom(activityImuAddress,count)!=count)return false;
 for(uint8_t i=0;i<count;++i) {data[i]=Wire.read();}
 return true;
}
bool motionWrite(uint8_t reg,uint8_t value) {Wire.beginTransmission(activityImuAddress);Wire.write(reg);Wire.write(value);return Wire.endTransmission()==0;}
bool motionConfigure() {
 for(uint8_t address:{(uint8_t)0x6B,(uint8_t)0x6A}) {
  activityImuAddress=address;uint8_t id=0;
  if(motionRead(0,&id,1) && id==0x05) {
   // Disable all sensors before changing full scale and ODR. No software reset or CTRL9 sequence needed.
   if(motionWrite(0x08,0x00) && motionWrite(0x02,0x40) && motionWrite(0x03,0x1D) && motionWrite(0x06,0x00) && motionWrite(0x14,0x00) && motionWrite(0x08,0x01)) {delay(20);return true;}
  }
 }
 activityImuAddress=0;return false;
}
void motionSamplerOnce() {
 uint32_t now=millis();
 ActivityBusLock lock;if(!lock)return;
 if(!activityEnabled) {
  if(imuStreaming) {motionWrite(0x08,0);motionWrite(0x02,0x41);imuStreaming=false;}
  return;
 }
 if(!imuStreaming) {
  if(activityProbeAttempted && (uint32_t)(now-activityProbeAt)<10000)return;
  activityProbeAttempted=true;activityProbeAt=now;bool ready=motionConfigure();imuPresent=ready;imuStreaming=ready;activityI2CErrors=0;
  if(!imuStreaming)return;
 }
 uint8_t status=0,raw[6]={};
 if(!motionRead(0x2E,&status,1)) {if(++activityI2CErrors>=3){imuPresent=false;imuStreaming=false;}return;}
 if(!(status&1)){if((uint32_t)(now-activitySampleAt)>5000 && (uint32_t)(now-activityProbeAt)>5000){imuPresent=false;imuStreaming=false;}return;}
 if(!motionRead(0x35,raw,6)) {if(++activityI2CErrors>=3){imuPresent=false;imuStreaming=false;}return;}
 activityI2CErrors=0;activitySampleAt=millis();activityHaveSample=true;
 MotionSample sample={activitySampleAt,0,0,0};
 sample.x=(int16_t)((uint16_t)raw[0]|((uint16_t)raw[1]<<8))/8192.0f;
 sample.y=(int16_t)((uint16_t)raw[2]|((uint16_t)raw[3]<<8))/8192.0f;
 sample.z=(int16_t)((uint16_t)raw[4]|((uint16_t)raw[5]<<8))/8192.0f;
 if(xQueueSend(activitySamples,&sample,0)!=pdTRUE) {MotionSample old;xQueueReceive(activitySamples,&old,0);++activityLostSamples;xQueueSend(activitySamples,&sample,0);}
}
constexpr uint32_t MOTION_POLL_MS=40; // Sensor output is ~21 Hz (47.6ms); keep polling faster than new samples.
void motionTaskMain(void*) {for(;;){motionSamplerOnce();vTaskDelay(pdMS_TO_TICKS(activityEnabled?MOTION_POLL_MS:2000));}}
void activityHapticBegin() {
#if NEXUS_HAPTIC_PIN >= 0
 pinMode(NEXUS_HAPTIC_PIN,OUTPUT);digitalWrite(NEXUS_HAPTIC_PIN,LOW);
#endif
}
void activityBusBegin() {activityBusMutex=xSemaphoreCreateMutex();}
void activitySamplerBegin() {
 activitySamples=xQueueCreate(64,sizeof(MotionSample));
 if(!activitySamples || !activityBusMutex){activityTaskOK=false;return;}
 activityTaskOK=xTaskCreate(motionTaskMain,"nexus-imu",3072,nullptr,1,&activityTask)==pdPASS;
}
uint32_t activityDateKey() {return timeValid ? (uint32_t)(2000+yr)*10000+mon*100+day:0;}
bool activityValidDate(uint32_t d) {int y=d/10000,m=(d/100)%100,dd=d%100;return y>=2000&&y<=2099&&m>=1&&m<=12&&dd>=1&&dd<=daysInMonth(m,y-2000);}
uint32_t activityCrc(const uint8_t* p,size_t size) {uint32_t crc=0xffffffff;for(size_t i=0;i<size;++i)crc=crcStep(crc,p[i]);return crc^0xffffffff;}
void activityLoad() {
 if(!prefsOK)return;
 uint16_t profile[9]={};
 if(prefs.getBytesLength("health")==sizeof(profile) && prefs.getBytes("health",profile,sizeof(profile))==sizeof(profile) && profile[0]==1 && (profile[1]==0||(profile[1]>=80&&profile[1]<=230)) && (profile[2]==0||(profile[2]>=200&&profile[2]<=2500)) && (profile[3]==0||(profile[3]>=300&&profile[3]<=1300)) && profile[4]>=1000&&profile[4]<=30000 && profile[5]>=15&&profile[5]<=180 && profile[6]<=1&&profile[7]<=1&&profile[8]<3) {
  personHeightCm=profile[1];personWeightDeciKg=profile[2];personStrideMm=profile[3];stepGoal=profile[4];moveIntervalMin=profile[5];moveRemindersEnabled=profile[6];activityEnabled=profile[7];activitySensitivity=profile[8];
 }
 uint32_t stats[17]={};
 if(prefs.getBytesLength("steps")!=sizeof(stats))return;
 bool ok=prefs.getBytes("steps",stats,sizeof(stats))==sizeof(stats) && stats[0]==1 && stats[1]<=250000 && stats[16]==activityCrc((uint8_t*)stats,16*sizeof(uint32_t));
 for(int i=0;i<7 && ok;++i) {if(stats[3+i*2]>250000 || (stats[2+i*2]&&!activityValidDate(stats[2+i*2])))ok=false;for(int j=0;j<i;++j)if(stats[2+i*2] && stats[2+i*2]==stats[2+j*2])ok=false;}
 activityStatsOK=ok;
 if(ok){undatedSteps=stats[1];for(int i=0;i<7;++i){activityDates[i]=stats[2+i*2];activitySteps[i]=stats[3+i*2];}}
}
bool activitySaveProfile() {
 if(!prefsOK)return false;
 uint16_t p[9]={1,personHeightCm,personWeightDeciKg,personStrideMm,(uint16_t)stepGoal,moveIntervalMin,(uint16_t)moveRemindersEnabled,(uint16_t)activityEnabled,activitySensitivity};
 return prefs.putBytes("health",p,sizeof(p))==sizeof(p);
}
int activityDaySlot(uint32_t date) {
 for(int i=0;i<7;++i)if(activityDates[i]==date)return i;
 for(int i=6;i>0;--i){activityDates[i]=activityDates[i-1];activitySteps[i]=activitySteps[i-1];}
 activityDates[0]=date;activitySteps[0]=0;activityStatsDirty=true;activityLastSave=millis()-300000;return 0;
}
uint32_t activityTodaySteps() {uint32_t d=activityDateKey();if(!d)return undatedSteps;for(int i=0;i<7;++i)if(activityDates[i]==d)return activitySteps[i];return 0;}
float activityStrideMm() {return personStrideMm ? personStrideMm:(personHeightCm ? personHeightCm*4.15f:0.0f);}
float activityDistanceKm() {return activityTodaySteps()*activityStrideMm()/1000000.0f;}
float activityWalkingKcal() {return activityDistanceKm()*(personWeightDeciKg/10.0f)*0.5f;}
void activitySaveStats(bool force) {
 if(!prefsOK || !activityStatsDirty || (!force&&(uint32_t)(millis()-activityLastSave)<300000))return;
 uint32_t s[17]={1,undatedSteps};for(int i=0;i<7;++i){s[2+i*2]=activityDates[i];s[3+i*2]=activitySteps[i];}s[16]=activityCrc((uint8_t*)s,16*sizeof(uint32_t));
 if(prefs.putBytes("steps",s,sizeof(s))==sizeof(s)){activityStatsDirty=false;activityStatsOK=true;}
 activityLastSave=millis();
}
void activityAddSteps(uint32_t count,uint32_t at) {
 uint32_t d=activityDateKey();
 if(d){int slot=activityDaySlot(d);activitySteps[slot]=std::min<uint32_t>(250000,activitySteps[slot]+count);}else undatedSteps=std::min<uint32_t>(250000,undatedSteps+count);
 activityStatsDirty=true;walkObserved=true;lastWalkAt=at;
 if(moveReminderActive){moveReminderActive=false;uiDirty=true;}moveReminderPending=false;
}
void activityAcceptPeak(uint32_t at) {
 if(!activityCadenceCount){activityCadenceCount=1;activityPeakAt=at;return;}
 uint32_t interval=at-activityPeakAt;
 if(interval<260){activityCadenceCount=1;activityInterval=0;activityPeakAt=at;return;}
 if(interval>1600 || (activityInterval && (uint32_t)abs((int32_t)interval-(int32_t)activityInterval)>std::max<uint32_t>(180,activityInterval*40/100))) {
  activityCadenceCount=1;activityInterval=0;activityPeakAt=at;return;
 }
 activityPeakAt=at;activityInterval=activityInterval ? (activityInterval*3+interval)/4:interval;
 if(activityCadenceCount<4) {++activityCadenceCount;if(activityCadenceCount==4)activityAddSteps(4,at);}
 else activityAddSteps(1,at);
}
void activityProcessSample(const MotionSample& sample) {
 if(!std::isfinite(sample.x)||!std::isfinite(sample.y)||!std::isfinite(sample.z))return;
 uint32_t dt=sample.at-activityPreviousAt;
 float magnitude=sqrtf(sample.x*sample.x+sample.y*sample.y+sample.z*sample.z);
 if(magnitude<0.2f || magnitude>3.5f){activityFilterReset();return;}
 if(!activityFilterReady || dt>250) {activityFilterReset();activityGravity=magnitude;activityFilterReady=true;activityPreviousAt=sample.at;return;}
 activityPreviousAt=sample.at;
 if(dt<15)return;
 if(activityWarmSamples<20){activityGravity+=(magnitude-activityGravity)*0.15f;++activityWarmSamples;return;}
 activityGravity+=(magnitude-activityGravity)*0.06f;
 activityFiltered+=(magnitude-activityGravity-activityFiltered)*0.5f;
 const float levels[]={0.16f,0.11f,0.075f};float threshold=levels[activitySensitivity];
 if(activityPeakArmed && activityPrevious>=threshold && activityFiltered<activityPrevious){activityPeakArmed=false;activityAcceptPeak(sample.at);}
 if(activityFiltered<0.02f)activityPeakArmed=true;
 activityPrevious=activityFiltered;
}
void movementReminderAcknowledge() {moveReminderActive=false;moveReminderPending=false;lastMoveReminderAt=millis();uiDirty=true;
#if NEXUS_HAPTIC_PIN >= 0
 digitalWrite(NEXUS_HAPTIC_PIN,LOW);
#endif
}
void activityService() {
 uint32_t now=millis(),date=activityDateKey();
 if(date){int slot=activityDaySlot(date);if(undatedSteps){activitySteps[slot]=std::min<uint32_t>(250000,activitySteps[slot]+undatedSteps);undatedSteps=0;activityStatsDirty=true;}}
 MotionSample sample;
 for(int i=0;activitySamples && i<16 && xQueueReceive(activitySamples,&sample,0)==pdTRUE;++i)if(activityEnabled)activityProcessSample(sample);
 if(!activityEnabled || !imuPresent || (uint32_t)(now-activitySampleAt)>2000){walkObserved=false;activityFilterReset();moveReminderPending=false;}
 if(moveRemindersEnabled && activityEnabled && imuPresent && walkObserved && timeValid && hh>=8 && hh<22 && !doNotDisturb && (uint32_t)(now-lastWalkAt)>=(uint32_t)moveIntervalMin*60000 && (uint32_t)(now-lastMoveReminderAt)>=(uint32_t)moveIntervalMin*60000) {moveReminderPending=true;lastMoveReminderAt=now;}
 activitySaveStats();
}
void movementReminderService() {
 bool blocked=alarmRinging||timerRinging||previewActive;
 if(doNotDisturb||!moveRemindersEnabled||!activityEnabled||!imuPresent){moveReminderActive=false;moveReminderPending=false;}
 else if(blocked && moveReminderActive){moveReminderPending=true;moveReminderActive=false;}
 else if(moveReminderPending&&!blocked){moveReminderPending=false;moveReminderActive=true;wakeDisplay();moveReminderStart=millis();beep(1200,140);uiDirty=true;}
 if(moveReminderActive&&(uint32_t)(millis()-moveReminderStart)>=8000){moveReminderActive=false;uiDirty=true;}
#if NEXUS_HAPTIC_PIN >= 0
 uint32_t t=millis()-moveReminderStart;bool on=moveReminderActive&&(t<150||(t>=250&&t<400));digitalWrite(NEXUS_HAPTIC_PIN,on?HIGH:LOW);
#endif
}
void activityAdjustField(int dir) {
 if(activityProfileCursor==0)personHeightCm=personHeightCm ? (personHeightCm-80+151+dir)%151+80:170;
 else if(activityProfileCursor==1)personWeightDeciKg=personWeightDeciKg ? ((personWeightDeciKg-200)/5+461+dir)%461*5+200:700;
 else if(activityProfileCursor==2){if(!personStrideMm)personStrideMm=dir>0 ? 300:1300;else{int n=personStrideMm+dir*10;personStrideMm=(n<300||n>1300) ? 0:n;}}
 else if(activityProfileCursor==3){activitySensitivity=(activitySensitivity+3+dir)%3;activityFilterReset();}
 else if(activityProfileCursor==4)stepGoal=((stepGoal/1000-1+30+dir)%30+1)*1000;
 else if(activityProfileCursor==5)moveIntervalMin=((moveIntervalMin/15-1+12+dir)%12+1)*15;
 else if(activityProfileCursor==6){moveRemindersEnabled=!moveRemindersEnabled;if(!moveRemindersEnabled)movementReminderAcknowledge();}
 else if(activityProfileCursor==7){activityEnabled=!activityEnabled;activityProbeAttempted=false;activityFilterReset();}
 markSettingsDirty();uiDirty=true;
}
bool activitySetProfile(unsigned height,unsigned weight,unsigned stride,unsigned goal,unsigned interval) {
 if(height<80||height>230||weight<200||weight>2500||(stride&& (stride<300||stride>1300))||goal<1000||goal>30000||interval<15||interval>180)return false;
 personHeightCm=height;personWeightDeciKg=weight;personStrideMm=stride;stepGoal=goal;moveIntervalMin=interval;markSettingsDirty();uiDirty=true;return saveSettings();
}
void printActivityStatus() {Serial.printf("ACTIVITY IMU=%d TASK=%d ENABLED=%d STEPS=%lu DATE=%lu HEIGHT=%u WEIGHT10=%u STRIDE_MM=%u LOST=%lu STATS_OK=%d\n",(bool)imuPresent,activityTaskOK,(bool)activityEnabled,(unsigned long)activityTodaySteps(),(unsigned long)activityDateKey(),personHeightCm,personWeightDeciKg,personStrideMm,(unsigned long)activityLostSamples,activityStatsOK);}
