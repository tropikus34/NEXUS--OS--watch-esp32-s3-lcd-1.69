#pragma once
// USB protocol + bounded, transactional image storage. No filesystem auto-format.
#include <LittleFS.h>
#include <esp_partition.h>
constexpr uint32_t WALL_BYTES=240u*280u*2u;
uint32_t themePartitionBytes(){const esp_partition_t* p=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_ANY,"spiffs");return p?p->size:0;}
bool themeFsReady=false,wallpaperEnabled=true,wallpaperPresent=false;
uint8_t clockStyle=0;
File themeUpload;
bool themeReceiving=false;
uint32_t themeReceived=0,themeExpectedCrc=0,themeCrc=0xFFFFFFFF,themeLastByte=0;
uint8_t themeBlock[512];size_t themeBlockCount=0;
char themeCommand[256]={};size_t themeCommandCount=0;bool themeCommandOverflow=false;
uint32_t crcStep(uint32_t crc,uint8_t value) {
  crc^=value;for(int i=0;i<8;++i) crc=(crc>>1) ^ ((crc&1)?0xEDB88320u:0u);return crc;
}
void loadThemeSettings() {
  osdLoad();
  uint8_t personal[4]={};if(prefsOK&&prefs.getBytesLength("visual")==4&&prefs.getBytes("visual",personal,4)==4&&personal[0]<=1&&personal[1]<=1&&personal[2]<=1&&personal[3]<=24){paletteOverride=personal[0];faceTop=personal[1];faceDate=personal[2];cardRadius=personal[3];}
  if(prefsOK) {
    uint16_t c[2];
    if(prefs.getBytesLength("palette")==sizeof(c) && prefs.getBytes("palette",c,sizeof(c))==sizeof(c)) {customAccent=c[0];customSecondary=c[1];}
    uint16_t surface[5]={};if(prefs.getBytesLength("surface")==sizeof(surface)&&prefs.getBytes("surface",surface,sizeof(surface))==sizeof(surface)&&surface[0]==1){customBg=surface[1];customPanel=surface[2];customText=surface[3];customMuted=surface[4];}
    uint8_t storedVariant=0;faceVariant=(prefs.getBytesLength("variant")==1&&prefs.getBytes("variant",&storedVariant,1)==1&&storedVariant<=7)?storedVariant:0;
    uint8_t face[5]={};size_t faceSize=prefs.getBytesLength("face");if((faceSize==4||faceSize==5)&&prefs.getBytes("face",face,faceSize)==faceSize&&((faceSize==4&&face[0]==1)||(faceSize==5&&face[0]==2&&face[4]<=100))&&face[1]<=1&&face[2]<=6&&face[3]<=6){faceSeconds=face[1];faceCompA=face[2];faceCompB=face[3];if(faceSize==5)wallpaperMix=face[4];}
    uint16_t sound[3]={};if(prefs.getBytesLength("sound")==sizeof(sound)&&prefs.getBytes("sound",sound,sizeof(sound))==sizeof(sound)&&sound[0]==1&&sound[1]>=200&&sound[1]<=5000&&sound[2]>=40&&sound[2]<=1000){notificationToneHz=sound[1];notificationToneMs=sound[2];}
    uint8_t a[2];
    if(prefs.getBytesLength("appear")==sizeof(a) && prefs.getBytes("appear",a,sizeof(a))==sizeof(a) && a[0]<=1 && a[1]<=1) {wallpaperEnabled=a[0];clockStyle=a[1];}
  }
  themeFsReady=themePartitionBytes() && LittleFS.begin(false);
  if(!themeFsReady) {Serial.println("LittleFS unavailable; select a partition with filesystem. No erase performed.");return;}
  if(!LittleFS.exists("/wall.rgb") && LittleFS.exists("/wall.bak")) LittleFS.rename("/wall.bak","/wall.rgb");
  File f=LittleFS.open("/wall.rgb","r");wallpaperPresent=f && f.size()==WALL_BYTES;f.close();
  LittleFS.remove("/wall.tmp");
}
bool removeWallpaper() {
  if(!themeFsReady || themeReceiving) return false;
  bool ok=!LittleFS.exists("/wall.rgb") || LittleFS.remove("/wall.rgb");
  if(ok) {LittleFS.remove("/wall.bak");wallpaperPresent=false;uiDirty=true;}
  return ok;
}
void themeAbort(const char* reason) {
  if(themeUpload) themeUpload.close();
  if(themeFsReady) LittleFS.remove("/wall.tmp");
  themeReceiving=false;themeBlockCount=0;Serial.println(reason);
}
void themeFinish() {
  themeUpload.flush();themeUpload.close();
  if((themeCrc^0xFFFFFFFFu)!=themeExpectedCrc) {themeAbort("ERR CRC");return;}
  LittleFS.remove("/wall.bak");
  bool had=LittleFS.exists("/wall.rgb");
  if(had && !LittleFS.rename("/wall.rgb","/wall.bak")) {themeAbort("ERR BACKUP");return;}
  if(!LittleFS.rename("/wall.tmp","/wall.rgb")) {
    if(had) {LittleFS.rename("/wall.bak","/wall.rgb");}
    themeAbort("ERR COMMIT");return;
  }
  // Keep backup until the next validated upload, permitting boot-time recovery.
  themeReceiving=false;wallpaperPresent=true;wallpaperEnabled=true;
  markSettingsDirty();uiDirty=true;wakeDisplay();Serial.println("OK SAVED");
}
void themeHandleCommand() {
  unsigned long length=0,crc=0;unsigned int a=0,b=0,style=0;char extra=0;
  if(strcmp(themeCommand,"NXHELLO")==0) {
    Serial.printf("NEXUS 6.9.3 FS=%d WALL=%d SIZE=%lu STUDIO=1 OSD=1 FSINFO=1 BUFFER=1 VARIANT=1 CUSTOM=1\n",themeFsReady,wallpaperPresent,(unsigned long)WALL_BYTES);return;
  }
  if(studioHandleCommand(themeCommand))return;
  if(sscanf(themeCommand,"NXBG1 %lu %lx %c",&length,&crc,&extra)==2) {
    if(length!=WALL_BYTES) {Serial.println("ERR SIZE");return;}
    if(!themeFsReady) {Serial.println(themePartitionBytes()?"ERR NO_FILESYSTEM":"ERR FS_NO_PARTITION");return;}
    uint32_t total=LittleFS.totalBytes(),used=LittleFS.usedBytes();if(used>total||total-used<WALL_BYTES+8192u){Serial.println("ERR FS_NO_SPACE");return;}
    LittleFS.remove("/wall.tmp");themeUpload=LittleFS.open("/wall.tmp","w");
    if(!themeUpload) {Serial.println("ERR OPEN");return;}
    themeReceived=0;themeBlockCount=0;themeExpectedCrc=(uint32_t)crc;themeCrc=0xFFFFFFFF;
    themeReceiving=true;themeLastByte=millis();wakeDisplay();Serial.println("OK READY");return;
  }
  if(sscanf(themeCommand,"NXSTYLE %x %x %u %c",&a,&b,&style,&extra)==3) {
    if(a>65535 || b>65535 || style>1) {Serial.println("ERR STYLE");return;}
    uint16_t palette[2]={(uint16_t)a,(uint16_t)b};
    if(!prefsOK || prefs.putBytes("palette",palette,sizeof(palette))!=sizeof(palette)) {Serial.println("ERR SAVE");return;}
    customAccent=a;customSecondary=b;clockStyle=style;themeIndex=7;applyTheme();markSettingsDirty();
    if(!saveSettings()) {Serial.println("ERR SAVE");return;}
    uiDirty=true;Serial.println("OK THEME");return;
  }
  if(strcmp(themeCommand,"NXFORMAT CONFIRM")==0) {
    if(themeFsReady) {Serial.println("ERR FS_ALREADY_READY");return;}
    if(timerRunning || alarmRinging || timerRinging) {Serial.println("ERR BUSY");return;}
    if(!themePartitionBytes()){Serial.println("ERR FS_NO_PARTITION");return;}
    // Only a separately confirmed host action can request formatting.
    themeFsReady=LittleFS.begin(true);wallpaperPresent=false;
    Serial.println(themeFsReady ? "OK FILESYSTEM":"ERR NO_FILESYSTEM");return;
  }
  unsigned int look=0;
  if(sscanf(themeCommand,"NXLOOK %u %c",&look,&extra)==1) {
    if(look>=7) {Serial.println("ERR LOOK");return;}
    themeIndex=look;applyTheme();markSettingsDirty();
    if(!saveSettings()) {Serial.println("ERR SAVE");return;}
    uiDirty=true;Serial.println("OK LOOK");return;
  }
  if(sscanf(themeCommand,"NXSPEC %u %c",&look,&extra)==1) {
    if(look>=7 || !setSpecialFromUsb(look)) {Serial.println("ERR SPECIAL");return;}
    Serial.println("OK SPECIAL");return;
  }
  unsigned int height=0,weight=0,stride=0,goal=0,interval=0;
  if(sscanf(themeCommand,"NXHEALTH %u %u %u %u %u %c",&height,&weight,&stride,&goal,&interval,&extra)==5) {
    Serial.println(activitySetProfile(height,weight,stride,goal,interval) ? "OK HEALTH":"ERR HEALTH");return;
  }
  unsigned int capacity=0;
  if(sscanf(themeCommand,"NXCAPACITY %u %c",&capacity,&extra)==1) {Serial.println(batterySetCapacity(capacity) ? "OK CAPACITY":"ERR CAPACITY");return;}
  if(strcmp(themeCommand,"NXBATTERY")==0) {printBatteryStatus();return;}
  if(strcmp(themeCommand,"NXACTIVITY")==0) {printActivityStatus();return;}
  if(strcmp(themeCommand,"NXPOWER")==0) {printPowerStatus();return;}
  if(strcmp(themeCommand,"NXSTATUS")==0) {printRuntimeStatus();return;}
  if(strcmp(themeCommand,"NXNOTIFY")==0) {Serial.println(injectTestNotification() ? "OK NOTIFY":"ERR QUEUE");return;}
  if(strcmp(themeCommand,"NXDELETE")==0) {Serial.println(removeWallpaper()?"OK DELETED":"ERR DELETE");return;}
  Serial.println("ERR COMMAND");
}
void themeSerialService() {
  if(themeReceiving && (uint32_t)(millis()-themeLastByte)>10000) themeAbort("ERR TIMEOUT");
  // Per-iteration work bounded; alarms, timer and button keep being serviced.
  for(int n=0;n<512 && Serial.available();++n) {
    int value=Serial.read();if(value<0) break;powerSerialActivity();
    if(themeReceiving) {
      themeLastByte=millis();lastActivity=millis();
      themeBlock[themeBlockCount++]=(uint8_t)value;themeCrc=crcStep(themeCrc,(uint8_t)value);++themeReceived;
      if(themeBlockCount==sizeof(themeBlock) || themeReceived==WALL_BYTES) {
        if(themeUpload.write(themeBlock,themeBlockCount)!=themeBlockCount) {themeAbort("ERR WRITE");break;}
        themeBlockCount=0;
        if(themeReceived==WALL_BYTES) {themeFinish();break;}
        Serial.printf("ACK %lu\n",(unsigned long)themeReceived);
      }
    } else if(value=='\n') {
      if(!themeCommandOverflow){themeCommand[themeCommandCount]=0;themeHandleCommand();}themeCommandCount=0;themeCommandOverflow=false;
    } else if(value!='\r' && !themeCommandOverflow) {
      if(themeCommandCount<sizeof(themeCommand)-1) themeCommand[themeCommandCount++]=(char)value;
      else {themeCommandCount=0;themeCommandOverflow=true;Serial.println("ERR HEADER");}
    }
  }
}
