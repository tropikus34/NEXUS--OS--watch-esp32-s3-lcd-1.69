#include <cassert>
#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
int main(){
 setup();
 assert(!timeValid);
 assert(daysInMonth(2,24)==29 && daysInMonth(2,25)==28);
 assert(!validTime(0,0,0,0,31,4,26));
 draftH=12;draftM=34;draftD=26;draftMo=10;draftY=26;commitDraftTime();
 assert(timeValid && wday==1 && hh==12 && mm==34);
 draftD=29;draftMo=2;draftY=24;commitDraftTime();assert(wday==4);
 currentScreen=SC_TIME;timeCursor=4;draftY=24;draftMo=2;draftD=29;adjustField(1);assert(draftY==25&&draftD==28);
 currentScreen=SC_TIME;currentMode=MODE_EDIT;timeCursor=4;draftH=9;onLongPress();assert(currentMode==MODE_NAV&&hh==9);
 assert(batteryCurve(3.2f)==0 && batteryCurve(4.2f)==100);
 for(int i=321;i<=420;++i)assert(batteryCurve(i/100.f)>=batteryCurve((i-1)/100.f));
 timerPresetSeconds=5;timerRemainingMs=5000;timerRunning=false;timerAction(1);fakeMs+=2100;timerCheck();assert(timerRemainingMs==2900);
 checkpointTimer();uint32_t st[4];prefs.getBytes("timer",st,sizeof(st));assert(st[0]==1&&st[2]>clockEpoch());
 restoredDeadline=clockEpoch()+3;restoredTimerPending=true;timerRunning=false;restoreTimer();assert(timerRunning&&timerRemainingMs==3000);
 restoredDeadline=clockEpoch()-1;restoredTimerPending=true;timerRunning=false;restoreTimer();assert(timerRinging);
 stopAlerts();restoredDeadline=clockEpoch()-600;restoredTimerPending=true;restoreTimer();assert(!timerRinging);
 fakeMs=0xfffffff0;timerRemainingMs=100;timerRunning=true;timerLastUpdate=fakeMs;fakeMs+=150;timerCheck();assert(timerRinging&&timerRemainingMs==0);stopAlerts();
 currentScreen=SC_SETTINGS;settingsCursor=4;quietMode=false;adjustField(1);assert(quietMode);
 Notification n{"app","title","body"};for(int i=0;i<8;++i)notificationCallback(n);assert(droppedMessages==4);
 loop();assert(messageCount==4&&unreadMessages==4&&previewActive);
 for(int i=0;i<4;++i) { notificationCallback(n); } loop();assert(messageCount==5&&unreadMessages==5);
 currentScreen=SC_MESSAGES;currentMode=MODE_EDIT;messageIndex=0;drawMessagesScreen();assert(unreadMessages==4);
 currentScreen=SC_SETTINGS;settingsCursor=6;adjustField(-1);assert(messageCount==0&&unreadMessages==0);

 // Button is the real debounce/hold implementation with controlled GPIO and time.
 quietMode=false; screenAwake=true; currentScreen=SC_SETTINGS;currentMode=MODE_NAV;
 btnStable=true;btnLastRead=true;btnPressed=false;btnLongFired=false;waitingDouble=false;consumeButtonPress=false;
 fakeButton=0;handleButton();fakeMs+=51;handleButton();fakeMs+=750;checkLongPress();
 assert(currentMode==MODE_EDIT && currentScreen==SC_SETTINGS_SECTIONS && settingsSectionCursor==0);
 fakeMs+=4000;checkLongPress();assert(currentMode==MODE_EDIT && currentScreen==SC_SETTINGS_SECTIONS && settingsSectionCursor==0); // no home or repeat
 fakeButton=1;handleButton();fakeMs+=51;handleButton();assert(currentScreen==SC_SETTINGS_SECTIONS);
 fakeButton=0;handleButton();fakeMs+=51;handleButton();fakeMs+=701;checkLongPress();
 assert(currentMode==MODE_EDIT && currentScreen==SC_SETTINGS && settingsCursor==0);
 fakeMs+=2500;checkLongPress();assert(currentMode==MODE_EDIT && currentScreen==SC_SETTINGS && settingsCursor==0);
 fakeButton=1;handleButton();fakeMs+=51;handleButton();assert(settingsCursor==0);
 // Wake consumes the full press rather than navigating or editing accidentally.
 showScreen(SC_CLOCK);screenAwake=false;
 fakeButton=0;handleButton();fakeMs+=51;handleButton();assert(screenAwake);
 fakeButton=1;handleButton();fakeMs+=51;handleButton();assert(currentScreen==SC_CLOCK);
 // Five independent alarms and the timer share one visible TIME tab.
 currentScreen=SC_ALARM;currentMode=MODE_EDIT;alarmCursor=0;alarmSlot=0;timeTool=0;
 alarmH=6;alarmM=15;alarmEnabled=true;syncAlarmSlot();
 adjustField(1);assert(alarmSlot==1&&currentScreen==SC_ALARM);
 alarmCursor=1;adjustField(1);assert(alarmH==9); // slot 2 defaults to 08:00
 alarmCursor=2;adjustField(1);assert(alarmM==1);
 alarmCursor=3;adjustField(1);assert(alarmEnabled);
 assert(alarmHours[0]==6&&alarmMinutes[0]==15&&alarmEnabledBank[0]);
 markSettingsDirty();assert(saveSettings());
 alarmHours[0]=0;alarmMinutes[0]=0;alarmEnabledBank[0]=false;alarmSlot=0;
 loadAlarmBank();assert(alarmHours[0]==6&&alarmMinutes[0]==15&&alarmEnabledBank[0]);
 assert(alarmHours[1]==9&&alarmMinutes[1]==1&&alarmEnabledBank[1]);
 // A non-selected alarm rings; stopping it doesn't disable the other alarms.
 timeValid=true;hh=9;mm=1;day=26;mon=10;yr=26;alarmCheck();
 assert(alarmRinging&&ringingAlarmSlot==1);stopAlerts();alarmCheck();assert(!alarmRinging);
 hh=6;mm=15;alarmCheck();assert(alarmRinging&&ringingAlarmSlot==0);stopAlerts();
 // Tool selection reaches timer and wraps without editing another alarm.
 currentScreen=SC_ALARM;alarmCursor=0;timeTool=0;alarmSlot=0;
 for(int i=0;i<5;++i)adjustField(1);
 assert(timeTool==5&&currentScreen==SC_TIMER&&getMaxFields()==4);
 timerCursor=1;timerRunning=false;timerRinging=false;timerPresetSeconds=60;adjustField(1);assert(timerPresetSeconds==120);
 timerCursor=0;adjustField(1);assert(timeTool==0&&currentScreen==SC_ALARM);
 nextScreen();assert(currentScreen==SC_MESSAGES);
 currentScreen=SC_CLOCK;timeTool=5;nextScreen();assert(currentScreen==SC_ACTIVITY);nextScreen();assert(currentScreen==SC_TIMER);nextScreen();assert(currentScreen==SC_MESSAGES);
 // Personalization survives reload; Bluetooth name is built from saved owner.
 currentScreen=SC_SETTINGS;settingsCursor=7;themeIndex=0;adjustField(1);assert(themeIndex==1&&themeAccent==WATCH_LOOKS[1].accent);
 settingsCursor=8;adjustField(1);assert(currentScreen==SC_PROFILE&&currentMode==MODE_EDIT);
 snprintf(ownerDraft,sizeof(ownerDraft),"ED      ");profileCursor=7;onLongPress();
 assert(currentScreen==SC_SETTINGS&&currentMode==MODE_NAV&&!strcmp(ownerName,"ED"));
 assert(saveSettings());themeIndex=0;snprintf(ownerName,sizeof(ownerName),"RESET");loadPersonalization();
 assert(themeIndex==1&&!strcmp(ownerName,"ED")&&!strcmp(bleName,"NEXUS-ED"));
 // Dropped advertising is restarted only when allowed.
 NimBLEDevice::server.count=0;NimBLEDevice::adv.active=false;autoReconnect=true;pairWindowActive=false;
 fakeMs+=5001;bleService();assert(NimBLEDevice::adv.active);
 autoReconnect=false;fakeMs+=5001;bleService();assert(!NimBLEDevice::adv.active);
 currentScreen=SC_SETTINGS;settingsCursor=11;adjustField(-1);fakeMs+=5001;bleService();assert(NimBLEDevice::adv.active);
 fakeMs+=120001;bleService();assert(!NimBLEDevice::adv.active);
 // No forced bond request interrupts Chronos; existing confirmed bonds are still tracked.
 NimBLEDevice::server.count=1;NimBLEConnInfo::fakeBonded=false;securityRequested=false;savedPeer[0]=0;
 int requests=NimBLEDevice::requests;fakeMs+=5001;bleService();assert(NimBLEDevice::requests==requests&&!savedPeer[0]);
 fakeMs+=5001;bleService();assert(NimBLEDevice::requests==requests);
 NimBLEConnInfo::fakeBonded=true;fakeMs+=5001;bleService();assert(peerBonded&&!strcmp(savedPeer,"aa:bb:cc:dd:ee:ff"));
 savedPeer[0]=0;loadPersonalization();assert(!strcmp(savedPeer,"aa:bb:cc:dd:ee:ff"));
 forgetPhone();assert(!savedPeer[0]&&!peerBonded&&NimBLEDevice::server.count==0&&!NimBLEConnInfo::fakeBonded);
 // Profile is reached from SET and omitted from normal cyclic screen navigation.
 currentScreen=SC_TIME;nextScreen();assert(currentScreen==SC_CLOCK);

 // Upload protocol validates size/CRC, works with split headers and ACK blocks.
 auto command=[](const std::string& text){for(uint8_t c:text)Serial.input.push_back(c);while(Serial.available())themeSerialService();};
 std::vector<uint8_t> pixels(WALL_BYTES);for(size_t i=0;i<pixels.size();++i)pixels[i]=(uint8_t)(i*13);
 uint32_t crc=0xffffffff;for(auto b:pixels)crc=crcStep(crc,b);crc^=0xffffffff;
 Serial.output.clear();command("NXHELLO\n");assert(Serial.output.find("NEXUS 6.9.3")!=std::string::npos);
 Serial.output.clear();command("NXBG1 12 0\n");assert(!themeReceiving&&Serial.output.find("ERR SIZE")!=std::string::npos);
 char header[80];snprintf(header,sizeof(header),"NXBG1 %lu %lx\n",(unsigned long)WALL_BYTES,(unsigned long)crc);
 command(header);assert(themeReceiving);
 for(size_t i=0;i<pixels.size();i+=512){size_t end=std::min(i+512,pixels.size());for(size_t j=i;j<end;++j)Serial.input.push_back(pixels[j]);themeSerialService();}
 assert(!themeReceiving&&wallpaperPresent&&*LittleFS.files["/wall.rgb"]==pixels);
 assert(Serial.output.find("ACK 512\n")!=std::string::npos&&Serial.output.find("OK SAVED")!=std::string::npos);
 auto old=*LittleFS.files["/wall.rgb"];
 command("NXBG1 134400 0\n");assert(themeReceiving);
 for(size_t i=0;i<pixels.size();i+=512){size_t end=std::min(i+512,pixels.size());for(size_t j=i;j<end;++j)Serial.input.push_back(pixels[j]);themeSerialService();}
 assert(!themeReceiving&&*LittleFS.files["/wall.rgb"]==old&&Serial.output.find("ERR CRC")!=std::string::npos);
 command(header);fakeMs+=10001;themeSerialService();assert(!themeReceiving&&!LittleFS.exists("/wall.tmp"));
 Serial.output.clear();command("NXSTYLE 7ff ffff 1\n");assert(themeIndex==7&&clockStyle==1&&customAccent==0x07ff&&Serial.output.find("OK THEME")!=std::string::npos);
 themeIndex=0;clockStyle=0;customAccent=0;loadThemeSettings();loadPersonalization();assert(themeIndex==7&&clockStyle==1&&customAccent==0x07ff&&wallpaperPresent);
 command("NXDELETE\n");assert(!wallpaperPresent&&!LittleFS.exists("/wall.rgb"));
 themeFsReady=false;command(header);assert(!themeReceiving);themeFsReady=true;
 assert(LittleFS.formatCalls==0); // neither boot nor image upload formats storage
 themeFsReady=false;LittleFS.ready=false;command("NXBG1 134400 0\n");assert(LittleFS.formatCalls==0);
 timerRunning=false;timerRinging=false;alarmRinging=false;command("NXFORMAT CONFIRM\n");assert(themeFsReady&&LittleFS.formatCalls==1);
 command("NXFORMAT CONFIRM\n");assert(LittleFS.formatCalls==1); // refuses to erase a valid mounted store
 // Seven looks are distinct and can be selected over USB without deleting wallpaper.
 for(int i=0;i<7;++i){char cmd[32];snprintf(cmd,sizeof(cmd),"NXLOOK %d\n",i);command(cmd);assert(themeIndex==i&&themeAccent==WATCH_LOOKS[i].accent);}
 assert(WATCH_LOOKS[5].bg==0 && WATCH_LOOKS[5].text>0); // light theme uses dark text
 // Stopwatch: pause and lap don't lose elapsed time, including millis wrap.
 specialization=0;specialReset();specialAction(1);fakeMs+=1234;specialService();assert(specialElapsed==1234);
 specialAction(-1);assert(specialLap==1234&&specialRunning);specialAction(1);fakeMs+=5000;specialService();assert(specialElapsed==1234);
 specialAction(-1);assert(!specialRunning&&specialElapsed==0);
 fakeMs=0xfffffff0;specialReset();specialAction(1);fakeMs+=200;specialService();assert(specialElapsed==200);
 // Focus runs through work and break, accounting for a delayed main loop.
 specialization=1;specialReset();specialAction(1);fakeMs+=1500000;specialService();assert(specialElapsed==1500000&&specialRunning);
 fakeMs+=300000;specialService();assert(specialElapsed==0&&specialRunning);
 specialAction(1);fakeMs+=1000;specialService();assert(specialElapsed==0);
 // Eight 40/20 rounds finish once, then can be restarted with one click.
 specialization=2;specialReset();specialAction(1);fakeMs+=40000;specialService();assert(specialElapsed==40000);
 fakeMs+=440000;specialService();assert(!specialRunning&&specialElapsed==480000);specialAction(1);assert(specialRunning&&specialElapsed==0);
 // Travel offset wraps safely at documented limits and survives saving.
 specialization=3;travelOffset=840;specialAction(1);assert(travelOffset==-720);specialAction(-1);assert(travelOffset==840);
 hh=23;mm=30;travelOffset=120;char summary[40];specialSummary(summary,sizeof(summary));assert(!strcmp(summary,"01:30"));
 timeValid=false;specialSummary(summary,sizeof(summary));assert(!strcmp(summary,"--:--"));timeValid=true;
 // Rhythm is a real 4/6 second cycle, pausable.
 specialization=4;specialReset();specialAction(1);fakeMs+=4000;specialService();assert(specialElapsed==4000);fakeMs+=6000;specialService();assert(specialElapsed==0);
 // Journal stores a bounded set of three real timestamps, then reloads it.
 specialization=5;fieldCount=0;for(int i=0;i<5;++i){mm=i;specialAction(1);}assert(fieldCount==3);
 assert(strstr(fieldMarks[0],"23:04")!=nullptr);specialization=5;assert(saveSpecialization());fieldCount=0;loadSpecialization();assert(fieldCount==3&&specialization==5);
 specialAction(-1);assert(fieldCount==0);
 // A specialization is an actual persisted selection, not a theme side effect.
 command("NXSPEC 6\n");assert(specialization==6);specialization=0;loadSpecialization();assert(specialization==6);
 currentScreen=SC_CLOCK;currentMode=MODE_NAV;onLongPress();assert(currentScreen==SC_SPECIAL&&currentMode==MODE_EDIT);
 onLongPress();assert(currentScreen==SC_CLOCK&&currentMode==MODE_NAV);
 // A quick three-field style picker is reachable without cycling 16 settings.
 currentScreen=SC_MENU;currentMode=MODE_NAV;onLongPress();assert(currentScreen==SC_STYLE&&styleCursor==0);
 uint8_t previous=themeIndex;adjustField(1);assert(themeIndex==(previous+1)%8);onLongPress();assert(styleCursor==1);
 uint8_t priorVariant=faceVariant;adjustField(1);assert(faceVariant==(priorVariant+1)%7);onLongPress();assert(styleCursor==2);
 uint8_t priorSpecial=specialization;adjustField(1);assert(specialization==(priorSpecial+1)%7);onLongPress();assert(currentScreen==SC_MENU&&currentMode==MODE_NAV);
 specialization=1;specialReset();specialAction(1);fakeMs+=1000;specialService();command("NXSPEC 1\n");assert(specialRunning&&specialElapsed==1000);
 // Old 5.5 custom theme migrates without losing the user's palette.
 uint8_t legacy[12]={1,3,1,'E','D',0,0,0,0,0,0,0};prefs.putBytes("personal",legacy,12);loadPersonalization();assert(themeIndex==7&&!strcmp(ownerName,"ED"));
 themeIndex=6;markSettingsDirty();assert(saveSettings());themeIndex=0;loadPersonalization();assert(themeIndex==6);
 // Silent mode must not hide notifications, including when editing and asleep.
 alarmRinging=false;timerRinging=false;timerRunning=false;specialReset();doNotDisturb=false;quietMode=true;
 currentScreen=SC_SETTINGS;currentMode=MODE_EDIT;settingsCursor=7;previewActive=false;pendingPreview=false;
 suspendDisplay();assert(!screenAwake&&lcdSleeping&&lcd->displaySleeping&&getCpuFrequencyMhz()==80);
 assert(injectTestNotification());loop();assert(screenAwake&&previewActive&&!lcdSleeping&&!lcd->displaySleeping&&getCpuFrequencyMhz()==160);
 assert(currentScreen==SC_SETTINGS&&currentMode==MODE_EDIT&&settingsCursor==7);
 // Dismissing consumes the press and doesn't requeue the same preview.
 btnStable=true;btnLastRead=true;btnPressed=false;consumeButtonPress=false;waitingDouble=false;
 fakeButton=0;handleButton();fakeMs+=51;handleButton();assert(!previewActive&&!pendingPreview&&settingsCursor==7);
 fakeButton=1;handleButton();fakeMs+=51;handleButton();loop();assert(!previewActive);
 // Explicit DND stores messages without waking the panel.
 doNotDisturb=true;suspendDisplay();int beforeMessages=messageCount;assert(injectTestNotification());loop();
 assert(!screenAwake&&!previewActive&&!pendingPreview&&messageCount>=beforeMessages);
 // A message received during an alarm is shown after the alarm is dismissed.
 doNotDisturb=false;wakeDisplay();alarmEnabledBank[alarmSlot]=true;alarmEnabled=true;alarmRinging=true;ringingAlarmSlot=alarmSlot;alarmStart=millis();
 assert(injectTestNotification());loop();assert(!previewActive&&pendingPreview);
 stopAlerts();loop();assert(previewActive&&!pendingPreview);
 fakeMs+=8001;loop();assert(!previewActive);
 // CPU/panel profile can be disabled without changing BLE connection state.
 ecoMode=false;powerService();assert(getCpuFrequencyMhz()==240);ecoMode=true;powerService();
 // Diagnostic path confirms callback receipt independently from local display tests.
 uint32_t rxBefore=notificationRx;notificationCallback(n);loop();assert(notificationRx==rxBefore+1);
 Serial.output.clear();command("NXSTATUS\n");assert(Serial.output.find("STATUS LINK=")!=std::string::npos&&Serial.output.find("CPU=")!=std::string::npos);
 command("NXNOTIFY\n");loop();assert(previewActive);
 // Runtime choices survive reset, with sound and DND stored separately.
 ecoMode=true;doNotDisturb=true;quietMode=false;markSettingsDirty();assert(saveSettings());ecoMode=false;doNotDisturb=false;loadRuntimeSettings();assert(ecoMode&&doNotDisturb&&!quietMode);
 const char* broken="\xD0";assert(nextCodepoint(broken)=='?');
 assert(drawTextPage("",-100,0,0,7,COL_TEXT)==1);
 puts("PASS: calendar, editing, battery curve, timer persistence/expiry/wrap, quiet mode, queue overflow, unread eviction, clear, button holds, five alarms, shared timer, profile persistence, BLE bond tracking, advertising recovery, theme upload, CRC rollback, seven looks/tools, migration, notification wake/quiet/DND/edit/alert priority, preview dismiss, LCD sleep and ECO clocks");
}
