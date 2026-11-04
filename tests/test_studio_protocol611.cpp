#include <cassert>
#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
std::string cmd600(const std::string& cmd){Serial.output.clear();for(char c:cmd+"\n")Serial.input.push_back(c);while(Serial.available())themeSerialService();return Serial.output;}
int main(){setup();assert(cmd600("NXHELLO").find("STUDIO=1")!=std::string::npos);auto initial=cmd600("NXGETCONFIG");assert(initial.find("\"specialization\":0")!=std::string::npos&&initial.find("\"wallMix\":25")!=std::string::npos);FILE* f=fopen("/tmp/studio600_config.json","w");fwrite(initial.data()+7,1,initial.size()-7,f);fclose(f);
 assert(cmd600("NXSYS 96 2 0 0 1 1 0 1 1").find("OK SETTINGS")!=std::string::npos);assert(brightness==96&&screenTimeoutIndex==2&&!use24h&&!colonBlink&&quietMode&&privatePreview&&!autoReconnect&&ecoMode&&doNotDisturb);
 assert(cmd600("NXSYS 0 0 1 1 0 0 1 0 0").find("ERR RANGE")!=std::string::npos&&brightness==96);
 assert(cmd600("NXOWNER EDUARD").find("OK SETTINGS")!=std::string::npos&&!strcmp(ownerName,"EDUARD"));assert(cmd600("NXOWNER BAD\"NAME").find("ERR OWNER")!=std::string::npos&&!strcmp(ownerName,"EDUARD"));
 for(int slot=0;slot<5;++slot){assert(cmd600("NXALARM "+std::to_string(slot)+" 6 "+std::to_string(slot)+" 1").find("OK SETTINGS")!=std::string::npos);}assert(alarmMinutes[4]==4&&alarmEnabledBank[4]);
 assert(cmd600("NXALARM 5 6 0 1").find("ERR ALARM")!=std::string::npos);
 assert(cmd600("NXTIMERSET 61").find("OK TIMER")!=std::string::npos&&timerPresetSeconds==61);assert(cmd600("NXTIMER START").find("OK TIMER")!=std::string::npos&&timerRunning);fakeMs+=1000;timerCheck();auto remaining=timerRemainingMs;assert(cmd600("NXTIMERSET 120").find("ERR BUSY")!=std::string::npos&&timerPresetSeconds==61);
 assert(cmd600("NXTIMER START").find("OK TIMER")!=std::string::npos&&timerRemainingMs==remaining);assert(cmd600("NXTIMER PAUSE").find("OK TIMER")!=std::string::npos&&!timerRunning);assert(cmd600("NXTIMER RESET").find("OK TIMER")!=std::string::npos&&timerRemainingMs==61000);
 assert(cmd600("NXSETTIME 2026 10 29 17 30 45").find("OK TIME")!=std::string::npos&&timeValid&&wday==4&&hh==17&&ss==45);assert(cmd600("NXSETTIME 2025 2 29 12 0 0").find("ERR TIME")!=std::string::npos&&day==29&&mon==10);
 assert(cmd600("NXHEALTHFULL 180 805 700 2 10000 45 1 1").find("OK SETTINGS")!=std::string::npos&&personWeightDeciKg==805&&stepGoal==10000&&activityEnabled);
 assert(cmd600("NXHEALTHFULL 0 0 0 1 8000 60 0 0").find("OK SETTINGS")!=std::string::npos&&personWeightDeciKg==0&&!activityEnabled);assert(cmd600("NXHEALTHFULL 1 0 0 1 8000 60 0 0").find("ERR HEALTH")!=std::string::npos&&personHeightCm==0);
 assert(cmd600("NXCOLORS 0 63488 65535 21130 2016 31").find("OK COLORS")!=std::string::npos);assert(cmd600("NXAPPEAR 7 2016 31 1 1 3 -60").find("OK SETTINGS")!=std::string::npos&&themePanel==63488&&themeText==65535&&travelOffset==-60);
 assert(cmd600("NXFACE 0 5 6 80").find("OK FACE")!=std::string::npos&&!faceSeconds&&faceCompA==5&&faceCompB==6&&wallpaperMix==80);assert(cmd600("NXFACE 1 8 0 101").find("ERR FACE")!=std::string::npos&&!faceSeconds);
 assert(cmd600("NXSOUND 2000 240").find("OK SOUND")!=std::string::npos&&notificationToneMs==240);assert(cmd600("NXSOUND 2000 0").find("ERR SOUND")!=std::string::npos&&notificationToneMs==240);
 faceSeconds=true;faceCompA=0;wallpaperMix=25;notificationToneMs=40;customPanel=0;loadThemeSettings();applyTheme();assert(!faceSeconds&&faceCompA==5&&wallpaperMix==80&&notificationToneMs==240&&customPanel==63488);
 assert(cmd600("NXSOUNDTEST").find("ERR SILENT_OR_BUSY")!=std::string::npos);quietMode=false;doNotDisturb=false;assert(cmd600("NXSOUNDTEST").find("OK SOUND")!=std::string::npos);
 assert(cmd600("NXPAIR").find("OK PAIR")!=std::string::npos&&pairWindowActive);assert(cmd600("NXCLEARMSG").find("ERR COMMAND")!=std::string::npos);messageCount=unreadMessages=1;assert(cmd600("NXCLEARMSG CONFIRM").find("OK CLEAR")!=std::string::npos&&!messageCount&&!unreadMessages);
 auto pixels=std::make_shared<std::vector<uint8_t>>(WALL_BYTES);for(size_t i=0;i<pixels->size();++i)(*pixels)[i]=i%251;LittleFS.files["/wall.rgb"]=pixels;themeFsReady=true;assert(cmd600("NXWALLINFO").find("SIZE=134400")!=std::string::npos);auto block=cmd600("NXWALLREAD 0");assert(block.rfind("WALL 0 000102",0)==0&&block.size()==520);assert(cmd600("NXWALLREAD 1").find("ERR READ")!=std::string::npos);assert(cmd600("NXWALLREAD 134144").size()>500);
 prefs.failWrites=true;auto oldMs=notificationToneMs;assert(cmd600("NXSOUND 1800 250").find("ERR SAVE")!=std::string::npos&&notificationToneMs==oldMs);prefs.failWrites=false;
 puts("PASS STUDIO PROTOCOL: snapshot, validated system/owner/5 alarms/timer/date/health/full palette/face/sound, persistence, paired actions, wallpaper readback and write failure");
}
