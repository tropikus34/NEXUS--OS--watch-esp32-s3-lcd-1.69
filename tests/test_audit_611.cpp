#include <cassert>
#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
void resetButtonAudit(){fakeButton=HIGH;btnStable=true;btnLastRead=true;btnPressed=false;btnLongFired=false;waitingDouble=false;consumeButtonPress=false;}
void neutralAudit(){resetButtonAudit();alarmRinging=timerRinging=timerRunning=previewActive=pendingPreview=moveReminderActive=moveReminderPending=specialRunning=false;restoredTimerPending=false;memset(alarmEnabledBank,0,sizeof(alarmEnabledBank));alarmEnabled=false;activityEnabled=false;moveRemindersEnabled=false;quietMode=true;doNotDisturb=false;screenTimeoutIndex=0;screenAwake=true;lastActivity=lastTick=lastRtcPoll=millis();timeValid=true;hh=12;mm=34;ss=0;day=27;mon=10;yr=26;}
void tapAudit(){fakeButton=LOW;handleButton();fakeMs+=51;handleButton();fakeMs+=80;fakeButton=HIGH;handleButton();fakeMs+=51;handleButton();}
int countInkAudit(int x,int y,int w,int h,uint16_t ink){int count=0;for(int j=y;j<y+h;++j)for(int i=x;i<x+w;++i)if(gfx->framebuffer[j*240+i]==ink)++count;return count;}
int main(){setup();
 // All editors/readers survive automatic LCD sleep, a consumed wake tap, and resume.
 for(int kind=0;kind<10;++kind){neutralAudit();
  switch(kind){
  case 0:showScreen(SC_ACTIVITY);onLongPress();activityProfileCursor=1;break;
  case 1:showScreen(SC_MENU);onLongPress();styleCursor=1;break;
  case 2:showScreen(SC_SETTINGS);onLongPress();settingsSectionCursor=6;onLongPress();profileCursor=4;break;
  case 3:showScreen(SC_SETTINGS);onLongPress();settingsSectionCursor=2;break;
  case 4:showScreen(SC_SETTINGS);onLongPress();settingsSectionCursor=0;onLongPress();onLongPress();break;
  case 5:showScreen(SC_TIME);onLongPress();timeCursor=2;break;
  case 6:showScreen(SC_MESSAGES);onLongPress();break;
  case 7:showScreen(SC_CLOCK);onLongPress();break;
  case 8:showScreen(SC_ALARM);onLongPress();alarmCursor=2;break;
  default:showScreen(SC_TIMER);onLongPress();timerCursor=3;break;
  }
  Screen screen=currentScreen;Mode mode=currentMode;int cursor=currentCursor(),section=settingsSectionCursor,step=settingsSectionStep;lastActivity=millis();fakeMs+=16001;loop();
  assert(!screenAwake&&currentMode==mode&&currentScreen==screen&&currentCursor()==cursor&&settingsSectionStep==step);
  tapAudit();fakeMs+=301;checkSingleTimeout();assert(screenAwake&&currentMode==mode&&currentScreen==screen&&currentCursor()==cursor&&settingsSectionCursor==section&&!waitingDouble);
  if(kind==0){personWeightDeciKg=700;onSingleClick();assert(personWeightDeciKg==705);}
 }
 // A pending click must not dismiss a newly started alarm/timer.
 for(bool timer:{false,true}){neutralAudit();showScreen(SC_CLOCK);waitingDouble=true;btnLastRelease=millis();fakeMs+=100;
  if(timer){timerRemainingMs=1;timerRunning=true;timerLastUpdate=millis()-2;timerCheck();}
  else{alarmEnabled=true;alarmH=hh;alarmM=mm;alarmDates[0]=0;alarmSlot=0;syncAlarmSlot();alarmCheck();}
  assert(!waitingDouble);fakeMs+=301;checkSingleTimeout();assert(timer?timerRinging:alarmRinging);stopAlerts();
 }
 // Likewise a notification cannot replay navigation or an edit under its overlay.
 neutralAudit();showScreen(SC_CLOCK);waitingDouble=true;btnLastRelease=millis();assert(injectTestNotification());loop();assert(previewActive&&!waitingDouble);fakeMs+=301;checkSingleTimeout();assert(currentScreen==SC_CLOCK&&previewActive);
 // A press already held before a notification is consumed until release.
 neutralAudit();showScreen(SC_SETTINGS);onLongPress();settingsSectionCursor=0;onLongPress();int field=settingsCursor;
 fakeButton=LOW;handleButton();fakeMs+=51;handleButton();assert(btnPressed);previewActive=true;wakeDisplay();fakeMs+=1000;checkLongPress();assert(settingsCursor==field&&consumeButtonPress);
 fakeButton=HIGH;handleButton();fakeMs+=51;handleButton();assert(!waitingDouble&&settingsCursor==field);
 // The raw second press can begin inside 300 ms but finish its debounce outside.
 // This also exercises subtraction across millis() rollover.
 for(uint32_t start:{10000u,0xffffff00u})for(uint32_t gap:{50u,140u,250u,290u,300u}){
  fakeMs=start;neutralAudit();showScreen(SC_CLOCK);tapAudit();assert(waitingDouble);fakeMs=btnLastRelease+gap;fakeButton=LOW;handleButton();fakeMs+=11;checkSingleTimeout();assert(waitingDouble&&currentScreen==SC_CLOCK);
  fakeMs+=40;handleButton();fakeMs+=80;fakeButton=HIGH;handleButton();fakeMs+=51;handleButton();assert(currentScreen==SC_TIME&&!waitingDouble);fakeMs+=301;checkSingleTimeout();assert(currentScreen==SC_TIME);
 }
 // Long press exactly at 700 ms on the raw edge fires once.
 neutralAudit();showScreen(SC_CLOCK);fakeButton=LOW;handleButton();uint32_t down=millis();fakeMs+=51;handleButton();fakeMs=down+699;checkLongPress();assert(currentScreen==SC_CLOCK);fakeMs=down+700;checkLongPress();assert(currentScreen==SC_SPECIAL);fakeMs+=4000;checkLongPress();assert(currentScreen==SC_SPECIAL);
 // Additional custom color changes minutes in both actual firmware layouts.
 neutralAudit();themeIndex=7;customAccent=0x55ff;customSecondary=0xf800;applyTheme();for(int style:{0,1}){clockStyle=style;showScreen(SC_CLOCK);int x=style?97:131,y=style?137:88,w=style?109:86,h=style?56:84;assert(countInkAudit(x,y,w,h,0xf800)>10);customSecondary=0x07e0;applyTheme();drawClockScreen();assert(countInkAudit(x,y,w,h,0xf800)==0);customSecondary=0xf800;applyTheme();}
 // All seven faces identify AM/PM in 12-hour mode, and omit it in 24-hour mode.
 wallpaperEnabled=false;for(int t=0;t<7;++t)for(int v=0;v<8;++v){themeIndex=t;faceVariant=v;applyTheme();for(int hour:{1,13}){hh=hour;use24h=true;drawClockScreen();auto clean=gfx->framebuffer;use24h=false;drawClockScreen();int marked=0;int y0=v<6?226:42,x0=v<6?65:197;for(int y=y0;y<y0+16;++y)for(int x=x0;x<x0+28;++x)if(gfx->framebuffer[y*240+x]!=clean[y*240+x])++marked;assert(marked>0);}}

 // A failed settings write stays dirty and can be retried without losing RAM state.
 prefs.failWrites=true;brightness=96;markSettingsDirty();assert(!saveSettings()&&settingsDirty&&brightness==96);prefs.failWrites=false;assert(saveSettings()&&!settingsDirty);
 // Reject the entire overlong line, including a valid-looking destructive tail.
 LittleFS.files["/wall.rgb"]=std::make_shared<std::vector<uint8_t>>(WALL_BYTES,0);wallpaperPresent=true;themeFsReady=true;Serial.output.clear();
 std::string overlong(256,'X');overlong+="NXDELETE\nNXHELLO\n";for(char c:overlong)Serial.input.push_back(c);while(Serial.available())themeSerialService();
 assert(LittleFS.exists("/wall.rgb")&&wallpaperPresent&&Serial.output.find("ERR HEADER")!=std::string::npos&&Serial.output.find("OK DELETED")==std::string::npos&&Serial.output.find("NEXUS 6.9.3")!=std::string::npos);
 puts("PASS AUDIT: 10 sleep/resume contexts, overlay input cancellation, alarm/timer pending clicks, debounce boundary/rollover, exact 700ms hold, both custom colors/layouts, AM/PM on seven faces, NVS retry");
}
