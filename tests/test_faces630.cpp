#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
#include <cassert>
uint64_t picture(){uint64_t hash=1469598103934665603ULL;for(auto p:gfx->framebuffer){hash^=p;hash*=1099511628211ULL;}return hash;}
int main(){setup();timeValid=true;day=29;mon=10;wday=4;batPercent=85;wallpaperEnabled=false;osd.enabled=false;
 for(int d=0;d<10;++d){int pixels=0;for(int y=0;y<72;++y)for(int x=0;x<48;++x)pixels+=faceAlpha(d,x,y,48,72)>0;assert(pixels>500);assert(faceAlpha(d,-1,0,48,72)==0);}
 for(int v=0;v<8;++v)for(int t=0;t<7;++t){faceVariant=v;themeIndex=t;applyTheme();use24h=true;hh=12;mm=48;ss=37;drawClockScreen();auto first=picture();hh=23;mm=59;drawClockScreen();assert(first!=picture());
  for(int h:{0,1,12,23})for(int m:{0,8,59}){hh=h;mm=m;for(bool fmt:{false,true}){use24h=fmt;drawClockScreen();assert(displayPending);}}
  hh=12;mm=48;faceCompA=1;faceCompB=3;drawClockScreen();auto fields=picture();faceCompA=faceCompB=6;drawClockScreen();assert(fields!=picture());faceCompA=faceCompB=0;
  timeValid=false;drawClockScreen();assert(picture()!=first);timeValid=true;
 }
 themeIndex=2;faceVariant=1;applyTheme();drawClockScreen();auto before=picture();activityAddSteps(5000,millis());drawClockScreen();assert(before!=picture());before=picture();batPercent=20;drawClockScreen();assert(before!=picture());
 faceVariant=3;markSettingsDirty();assert(saveSettings());faceVariant=0;loadThemeSettings();assert(faceVariant==3);
 assert(studioHandleCommand("NXVARIANT 6"));assert(faceVariant==6);faceVariant=0;loadThemeSettings();assert(faceVariant==6);
 assert(studioHandleCommand("NXVARIANT 8"));assert(faceVariant==6);
 prefs.failWrites=true;studioHandleCommand("NXVARIANT 2");assert(faceVariant==6);prefs.failWrites=false;
 prefs.data["variant"]={99};loadThemeSettings();assert(faceVariant==0);
 currentScreen=SC_STYLE;currentMode=MODE_EDIT;styleCursor=0;onLongPress();assert(styleCursor==1);adjustField(1);assert(faceVariant==1);onLongPress();assert(styleCursor==2);onLongPress();assert(currentScreen==SC_MENU&&currentMode==MODE_NAV);
 assert(studioHandleCommand("NXCUSTOM 1 0 0 4"));assert(paletteOverride&&!faceTop&&!faceDate&&cardRadius==4);paletteOverride=false;loadThemeSettings();assert(paletteOverride);prefs.failWrites=true;studioHandleCommand("NXCUSTOM 0 1 1 22");assert(paletteOverride);prefs.failWrites=false;studioHandleCommand("NXCUSTOM 0 1 1 22");
 puts("PASS FACES: custom palette/visibility/radius persistence; 49 combinations, variant persistence/range/write failure/watch editor; bold masks, seven layouts, time changes/12h/24h, hidden fields, unset time, real step and battery meters");
}
