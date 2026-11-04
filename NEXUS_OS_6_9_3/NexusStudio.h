#pragma once
// Studio protocol v1. One bounded command at a time; no charger controls or bond-key export.
void studioPrintConfig(){
 syncAlarmSlot();powerAccounting();
 Serial.printf("CONFIG {\"schema\":1,\"firmware\":\"6.9.3\",\"owner\":\"%s\",\"theme\":%u,\"specialization\":%u,\"style\":%u,\"accent\":%u,\"secondary\":%u,\"bg\":%u,\"panel\":%u,\"text\":%u,\"muted\":%u,\"wallpaper\":%d,",ownerName,themeIndex,specialization,clockStyle,customAccent,customSecondary,customBg,customPanel,customText,customMuted,wallpaperEnabled);
 Serial.printf("\"brightness\":%u,\"timeout\":%u,\"format24\":%d,\"blink\":%d,\"quiet\":%d,\"privatePreview\":%d,\"autoReconnect\":%d,\"eco\":%d,\"dnd\":%d,",brightness,screenTimeoutIndex,use24h,colonBlink,quietMode,privatePreview,autoReconnect,ecoMode,doNotDisturb);
 Serial.printf("\"variant\":%u,\"paletteOverride\":%d,\"faceTop\":%d,\"faceDate\":%d,\"cardRadius\":%u,",faceVariant,paletteOverride,faceTop,faceDate,cardRadius);
 Serial.printf("\"seconds\":%d,\"compA\":%u,\"compB\":%u,\"wallMix\":%u,\"toneHz\":%u,\"toneMs\":%u,\"height\":%u,\"weight10\":%u,\"strideMm\":%u,\"sensitivity\":%u,\"goal\":%lu,\"moveMinutes\":%u,\"moveReminders\":%d,\"stepsEnabled\":%d,\"capacity\":%u,\"travelOffset\":%d,",faceSeconds,faceCompA,faceCompB,wallpaperMix,notificationToneHz,notificationToneMs,personHeightCm,personWeightDeciKg,personStrideMm,activitySensitivity,(unsigned long)stepGoal,moveIntervalMin,moveRemindersEnabled,(bool)activityEnabled,batteryCapacityMah,travelOffset);
 Serial.printf("\"alarms\":[");for(int i=0;i<ALARM_COUNT;++i)Serial.printf("%s{\"h\":%u,\"m\":%u,\"on\":%d}",i?",":"",alarmHours[i],alarmMinutes[i],alarmEnabledBank[i]);
 Serial.printf("],\"timer\":{\"preset\":%u,\"remainingMs\":%lu,\"running\":%d},\"clock\":{\"year\":%d,\"month\":%d,\"day\":%d,\"h\":%d,\"m\":%d,\"s\":%d,\"valid\":%d},",timerPresetSeconds,(unsigned long)timerRemainingMs,timerRunning,2000+yr,mon,day,hh,mm,ss,timeValid);
 Serial.printf("\"osd\":{\"version\":1,\"enabled\":%d,\"items\":[",osd.enabled);for(int id=0;id<OSD_COUNT;++id){const auto& item=osd.items[id];Serial.printf("%s{\"x\":%u,\"y\":%u,\"size\":%u,\"on\":%u,\"color\":%u}",id?",":"",item.x,item.y,item.size,item.on,item.color);}Serial.printf("]},");
 Serial.printf("\"power\":{\"awakeMs\":%llu,\"offMs\":%llu,\"frames\":%lu,\"cpuMHz\":%lu},",(unsigned long long)powerAwakeMs,(unsigned long long)powerOffMs,(unsigned long)displayFrames,(unsigned long)getCpuFrequencyMhz());
 Serial.printf("\"sleep\":{\"configured\":%d,\"error\":%d,\"bleRequestedMode\":%d,\"pmBuilt\":%d,\"ticklessBuilt\":%d,\"btModemBuilt\":%d,\"reason\":\"%s\",\"lastOffConfigured\":%d,\"rawBatteryV\":%.3f,\"sampleAgeMs\":%lu},",autoSleepConfigured,autoSleepError,blePowerMode,powerPmBuilt,powerTicklessBuilt,powerBtSleepBuilt,powerSleepReason(),lastOffSleepConfigured,batteryRawVolts,(unsigned long)(millis()-batterySampleAt));
 Serial.printf("\"powerPolicy\":{\"minMHz\":%lu,\"maxMHz\":%lu,\"advertisingMode\":%d},",(unsigned long)powerMinMHz,(unsigned long)powerMaxMHz,bleAdvertisingMode);
 Serial.printf("\"storage\":{\"partitionBytes\":%lu,\"total\":%lu,\"used\":%lu},\"buffered\":%d,",(unsigned long)themePartitionBytes(),(unsigned long)(themeFsReady?LittleFS.totalBytes():0),(unsigned long)(themeFsReady?LittleFS.usedBytes():0),displayBuffered);
 Serial.printf("\"live\":{\"ble\":%d,\"sub\":%d,\"rx\":%lu,\"lost\":%lu,\"batteryV\":%.3f,\"percent\":%d,\"imu\":%d,\"steps\":%lu,\"heap\":%u,\"fs\":%d,\"wall\":%d,\"paired\":%d,\"fsPartitionBytes\":%lu,\"fsTotal\":%lu,\"fsUsed\":%lu,\"buffered\":%d}}\n",chronosConnected,watch.isSubscribed(),(unsigned long)notificationRx,(unsigned long)droppedMessages,batteryVolts,batPercent,(bool)imuPresent,(unsigned long)activityTodaySteps(),(unsigned)ESP.getFreeHeap(),themeFsReady,wallpaperPresent,savedPeer[0]!=0,(unsigned long)themePartitionBytes(),(unsigned long)(themeFsReady?LittleFS.totalBytes():0),(unsigned long)(themeFsReady?LittleFS.usedBytes():0),displayBuffered);
}
void studioReplySave(){uiDirty=true;Serial.println(saveSettings()?"OK SETTINGS":"ERR SAVE");}
bool studioHandleCommand(const char* command){
 if(nxlfCommand(command))return true;
 unsigned a=0,b=0,c=0,d=0,e=0,f=0,g=0,h=0,i=0;int offset=0;char extra=0;
 if(sscanf(command,"NXCUSTOM %u %u %u %u %c",&a,&b,&c,&d,&extra)==4){
 if(a>1||b>1||c>1||d>24){Serial.println("ERR RANGE");return true;}uint8_t next[4]={(uint8_t)a,(uint8_t)b,(uint8_t)c,(uint8_t)d};
 if(!prefsOK||prefs.putBytes("visual",next,4)!=4){Serial.println("ERR SAVE");return true;}paletteOverride=a;faceTop=b;faceDate=c;cardRadius=d;applyTheme();uiDirty=true;Serial.println("OK CUSTOM");return true;
 }
 if(sscanf(command,"NXVARIANT %u %c",&a,&extra)==1){
  if(a>7){Serial.println("ERR RANGE");return true;}uint8_t next=(uint8_t)a;
  if(!prefsOK||prefs.putBytes("variant",&next,1)!=1){Serial.println("ERR SAVE");return true;}
  faceVariant=next;applyTheme();uiDirty=true;Serial.println("OK VARIANT");return true;
 }
 if(!strncmp(command,"NXOSD ",6)){
  const char* hex=command+6;uint8_t data[OSD_BYTES];if(strlen(hex)!=OSD_BYTES*2){Serial.println("ERR OSD_LENGTH");return true;}
  for(int j=0;j<OSD_BYTES;++j){unsigned v=0;for(int k=0;k<2;++k){char ch=hex[j*2+k];int digit=ch>='0'&&ch<='9'?ch-'0':(ch>='a'&&ch<='f'?ch-'a'+10:(ch>='A'&&ch<='F'?ch-'A'+10:-1));if(digit<0){Serial.println("ERR OSD_HEX");return true;}v=v*16+digit;}data[j]=v;}
  OsdLayout next={};if(!osdDecode(data,next)){Serial.println("ERR OSD_RANGE_OR_CRC");return true;}
  if(!prefsOK||prefs.putBytes("osd",data,sizeof(data))!=sizeof(data)){Serial.println("ERR SAVE");return true;}osd=next;uiDirty=true;Serial.println("OK OSD");return true;
 }
 if(!strcmp(command,"NXGETCONFIG")){studioPrintConfig();return true;}
 if(sscanf(command,"NXSYS %u %u %u %u %u %u %u %u %u %c",&a,&b,&c,&d,&e,&f,&g,&h,&i,&extra)==9){
  if(a<32||a>255||b>4||c>1||d>1||e>1||f>1||g>1||h>1||i>1){Serial.println("ERR RANGE");return true;}
  brightness=a;screenTimeoutIndex=b;use24h=c;colonBlink=d;quietMode=e;privatePreview=f;autoReconnect=g;ecoMode=h;doNotDisturb=i;
  if(doNotDisturb){previewActive=false;pendingPreview=false;movementReminderAcknowledge();}
  if(screenAwake){analogWrite(LCD_BL,brightness);}bleCheckAt=millis()-5000;powerService();markSettingsDirty();studioReplySave();return true;
 }
 if(!strncmp(command,"NXOWNER ",8)){
  const char* name=command+8;size_t n=strlen(name);bool ok=n>0&&n<=8;
  for(size_t j=0;j<n&&ok;++j)if(!strchr(" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_",name[j]))ok=false;
  bool nonSpace=false;for(size_t j=0;j<n;++j)if(name[j]!=' ')nonSpace=true;
  if(!ok||!nonSpace){Serial.println("ERR OWNER");return true;}
  snprintf(ownerName,sizeof(ownerName),"%s",name);markSettingsDirty();studioReplySave();return true;
 }
 if(sscanf(command,"NXALARM %u %u %u %u %c",&a,&b,&c,&d,&extra)==4){
  if(a>=ALARM_COUNT||b>23||c>59||d>1){Serial.println("ERR ALARM");return true;}
  syncAlarmSlot();alarmHours[a]=b;alarmMinutes[a]=c;alarmEnabledBank[a]=d;
  if((int)a==alarmSlot){alarmH=b;alarmM=c;alarmEnabled=d;}
  if((int)a==ringingAlarmSlot&&!d){stopAlerts();}markSettingsDirty();studioReplySave();return true;
 }
 if(sscanf(command,"NXTIMERSET %u %c",&a,&extra)==1){
  if(a<1||a>5999){Serial.println("ERR TIMER");return true;}if(timerRunning||timerRinging){Serial.println("ERR BUSY");return true;}
  timerPresetSeconds=a;timerRemainingMs=a*1000;restoredTimerPending=false;timerCheckpointDirty=true;timerCheckpointAt=millis()-1500;markSettingsDirty();
  bool ok=saveSettings();checkpointTimer();uiDirty=true;Serial.println(ok&&!timerCheckpointDirty?"OK TIMER":"ERR SAVE");return true;
 }
 if(!strncmp(command,"NXTIMER ",8)){
  if(timerRinging||alarmRinging){Serial.println("ERR BUSY");return true;}
  const char* action=command+8;
  if(!strcmp(action,"START")){if(!timerRunning)timerAction(1);}
  else if(!strcmp(action,"PAUSE")){if(timerRunning)timerAction(1);}
  else if(!strcmp(action,"RESET"))timerAction(-1);
  else {Serial.println("ERR TIMER");return true;}
  timerCheckpointDirty=true;timerCheckpointAt=millis()-1500;checkpointTimer();uiDirty=true;Serial.println(timerCheckpointDirty?"ERR SAVE":"OK TIMER");return true;
 }
 if(sscanf(command,"NXSETTIME %u %u %u %u %u %u %c",&a,&b,&c,&d,&e,&f,&extra)==6){
  if(a<2000||a>2099||!validTime(d,e,f,0,c,b,a-2000)){Serial.println("ERR TIME");return true;}
  yr=a-2000;mon=b;day=c;hh=d;mm=e;ss=f;wday=(clockEpoch()/86400+6)%7;timeValid=true;lastTick=millis();
  rtcOK=rtcPresent&&rtcWriteTime(hh,mm,ss,wday,day,mon,yr);if(timerRunning)timerCheckpointDirty=true;uiDirty=true;Serial.println(rtcOK?"OK TIME RTC=1":"OK TIME RTC=0");return true;
 }
 if(sscanf(command,"NXHEALTHFULL %u %u %u %u %u %u %u %u %c",&a,&b,&c,&d,&e,&f,&g,&h,&extra)==8){
  if((a&&(a<80||a>230))||(b&&(b<200||b>2500))||(c&&(c<300||c>1300))||d>2||e<1000||e>30000||f<15||f>180||g>1||h>1){Serial.println("ERR HEALTH");return true;}
  personHeightCm=a;personWeightDeciKg=b;personStrideMm=c;activitySensitivity=d;stepGoal=e;moveIntervalMin=f;moveRemindersEnabled=g;activityEnabled=h;activityProbeAttempted=false;activityFilterReset();if(!g||!h)movementReminderAcknowledge();markSettingsDirty();studioReplySave();return true;
 }
 if(sscanf(command,"NXAPPEAR %u %u %u %u %u %u %d %c",&a,&b,&c,&d,&e,&f,&offset,&extra)==7){
  if(a>7||b>65535||c>65535||d>1||e>1||f>6||offset< -720||offset>840||offset%30){Serial.println("ERR APPEAR");return true;}
  uint16_t palette[2]={(uint16_t)b,(uint16_t)c};if(!prefsOK||prefs.putBytes("palette",palette,sizeof(palette))!=sizeof(palette)){Serial.println("ERR SAVE");return true;}
  if(specialization!=f){specialReset();}themeIndex=a;customAccent=b;customSecondary=c;clockStyle=d;wallpaperEnabled=e;specialization=f;travelOffset=offset;applyTheme();markSettingsDirty();studioReplySave();return true;
 }
 if(sscanf(command,"NXCOLORS %u %u %u %u %u %u %c",&a,&b,&c,&d,&e,&f,&extra)==6){
  if(a>65535||b>65535||c>65535||d>65535||e>65535||f>65535){Serial.println("ERR COLORS");return true;}
  uint16_t surface[5]={1,(uint16_t)a,(uint16_t)b,(uint16_t)c,(uint16_t)d},palette[2]={(uint16_t)e,(uint16_t)f};
  if(!prefsOK||prefs.putBytes("surface",surface,sizeof(surface))!=sizeof(surface)||prefs.putBytes("palette",palette,sizeof(palette))!=sizeof(palette)){Serial.println("ERR SAVE");return true;}
  customBg=a;customPanel=b;customText=c;customMuted=d;customAccent=e;customSecondary=f;applyTheme();uiDirty=true;Serial.println("OK COLORS");return true;
 }
 if(sscanf(command,"NXFACE %u %u %u %u %c",&a,&b,&c,&d,&extra)==4){
  if(a>1||b>6||c>6||d>100){Serial.println("ERR FACE");return true;}uint8_t face[5]={2,(uint8_t)a,(uint8_t)b,(uint8_t)c,(uint8_t)d};
  if(!prefsOK||prefs.putBytes("face",face,sizeof(face))!=sizeof(face)){Serial.println("ERR SAVE");return true;}faceSeconds=a;faceCompA=b;faceCompB=c;wallpaperMix=d;uiDirty=true;Serial.println("OK FACE");return true;
 }
 if(sscanf(command,"NXSOUND %u %u %c",&a,&b,&extra)==2){
  if(a<200||a>5000||b<40||b>1000){Serial.println("ERR SOUND");return true;}uint16_t sound[3]={1,(uint16_t)a,(uint16_t)b};
  if(!prefsOK||prefs.putBytes("sound",sound,sizeof(sound))!=sizeof(sound)){Serial.println("ERR SAVE");return true;}notificationToneHz=a;notificationToneMs=b;Serial.println("OK SOUND");return true;
 }
 if(!strcmp(command,"NXSOUNDTEST")){if(quietMode||doNotDisturb||alarmRinging||timerRinging)Serial.println("ERR SILENT_OR_BUSY");else{beep(notificationToneHz,notificationToneMs);Serial.println("OK SOUND");}return true;}
 if(!strcmp(command,"NXPAIR")){pairWindowActive=true;pairWindowStart=millis();bleCheckAt=millis()-5000;Serial.println("OK PAIR");return true;}
 if(!strcmp(command,"NXFORGET CONFIRM")){forgetPhone();Serial.println(forgetFailed?"ERR FORGET":"OK FORGET");return true;}
 if(!strcmp(command,"NXCLEARMSG CONFIRM")){memset(messages,0,sizeof(messages));messageCount=messageIndex=messagePage=unreadMessages=0;previewActive=pendingPreview=false;uiDirty=true;Serial.println("OK CLEAR");return true;}
 if(!strcmp(command,"NXWAKE")){wakeDisplay();Serial.println("OK WAKE");return true;}
 if(!strcmp(command,"NXSTOPALERT")){stopAlerts();Serial.println("OK STOP");return true;}
 if(!strcmp(command,"NXREBOOT CONFIRM")){markSettingsDirty();if(!saveSettings()){Serial.println("ERR SAVE");return true;}activitySaveStats(true);if(activityStatsDirty){Serial.println("ERR SAVE");return true;}timerCheckpointDirty=true;timerCheckpointAt=millis()-1500;checkpointTimer();if(timerCheckpointDirty){Serial.println("ERR SAVE");return true;}Serial.println("OK REBOOT");delay(150);ESP.restart();return true;}
 if(!strcmp(command,"NXWALLINFO")){
  if(!themeFsReady||themeReceiving){Serial.println("ERR NO_BACKGROUND");return true;}File file=LittleFS.open("/wall.rgb","r");if(!file||file.size()!=WALL_BYTES){Serial.println("WALLINFO SIZE=0 CRC=0");return true;}
  uint8_t block[256];uint32_t crc=0xffffffff;for(uint32_t off=0;off<WALL_BYTES;off+=256){size_t n=file.read(block,std::min<uint32_t>(256,WALL_BYTES-off));if(n!=std::min<uint32_t>(256,WALL_BYTES-off)){Serial.println("ERR READ");return true;}for(size_t j=0;j<n;++j)crc=crcStep(crc,block[j]);}
  Serial.printf("WALLINFO SIZE=%lu CRC=%lx\n",(unsigned long)WALL_BYTES,(unsigned long)(crc^0xffffffff));return true;
 }
 if(sscanf(command,"NXWALLREAD %u %c",&a,&extra)==1){
  if(a>=WALL_BYTES||a%256||!themeFsReady||themeReceiving){Serial.println("ERR READ");return true;}File file=LittleFS.open("/wall.rgb","r");uint8_t block[256];size_t n=std::min<uint32_t>(256,WALL_BYTES-a);
  if(!file||file.size()!=WALL_BYTES||!file.seek(a)||file.read(block,n)!=n){Serial.println("ERR READ");return true;}Serial.printf("WALL %u ",a);for(size_t j=0;j<n;++j)Serial.printf("%02x",block[j]);Serial.println("");return true;
 }
 return false;
}
