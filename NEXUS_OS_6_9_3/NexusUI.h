#pragma once
// WATCH UI: rounded surfaces, capsule actions, quiet chrome. Original open fonts.
#define UI_CARD themePanel
#define UI_MUTED themeMuted
void uiText(const char* text,int x,int y,int scale=1,uint16_t color=COL_TEXT) {
 displayPending=true;
 const char* p=text;
 while(*p) {
  uint32_t cp=nextCodepoint(p);int advance=scale==2 ? premiumTitleAdvance(cp):premiumAdvance(cp)*scale;
  if(x+advance>228) break;
  if(scale==2) premiumTitleDraw(cp,x,y,color,uiFontBg);else premiumDraw(cp,x,y,scale,color,uiFontBg);x+=advance;
 }
}
void uiCenter(const char* text,int y,uint16_t color,int scale=1) {
 int width=0;const char* p=text;while(*p) {uint32_t cp=nextCodepoint(p);width+=scale==2 ? premiumTitleAdvance(cp):premiumAdvance(cp)*scale;}
 uiText(text,std::max(12,(240-width)/2),y,scale,color);
}
void uiBackground(int from=0,int height=280) {
 displayPending=true;
  File wall;
  if(wallpaperEnabled && wallpaperPresent && themeFsReady && currentScreen==SC_CLOCK)
    wall=LittleFS.open("/wall.rgb","r");
  uint16_t row[240];
  if(wall) wall.seek((uint32_t)from*480);
  if(!wall) gfx->fillRect(0,from,240,height,themeBg);
  else for(int y=from;y<from+height && y<280;++y) {
    bool image=wall && wall.read((uint8_t*)row,sizeof(row))==sizeof(row);
    if(image){for(int x=0;x<240;++x)row[x]=colorMix(row[x],themeBg,wallpaperMix,100);}
    if(!image) {
      // Depth without noise: a gentle vertical falloff from a lifted top.
      uint16_t color=themeBg;
      for(int x=0;x<240;++x) row[x]=color;
    }
    gfx->draw16bitRGBBitmap(0,y,row,240,1);
  }
  bool photo=(bool)wall;wall.close();uiFontBg=themeBg;
  if(currentScreen!=SC_CLOCK&&!photo&&from==0&&height>=280&&nxDark(themeBg)){nxWash(themePanel,themeBg,0,150);nxGlow(120,-40,190,themeAccent,30);}
}
int uiRadius(){return cardRadius;}
void uiCard(int x,int y,int w,int h,bool selected=false) {
 displayPending=true;
 int radius=std::min<int>(uiRadius(),std::min(w,h)/2);
 nxRoundRect(x,y,w,h,radius,themePanel);
 nxRoundRect(x,y,w,h,radius,themeText,1.0f,22); // hairline edge, glass-like
 uiFontBg=themePanel;
 if(selected){nxRoundRect(x,y,w,h,radius,themeAccent,2.0f);nxDisc(x+w-13,y+13,3,themeAccent);}
}
void uiIcon(int icon,int x,int y,uint16_t ink) {
 switch(icon){
 case 0:gfx->drawRoundRect(x,y+3,20,14,3,ink);gfx->drawLine(x+6,y+20,x+14,y+20,ink);break;
 case 1:gfx->drawCircle(x+10,y+11,9,ink);gfx->drawLine(x+10,y+3,x+10,y+19,ink);gfx->fillCircle(x+10,y+11,3,ink);break;
 case 2:gfx->drawRoundRect(x+4,y,12,22,3,ink);gfx->drawLine(x+8,y+18,x+12,y+18,ink);break;
 case 3:gfx->drawRoundRect(x,y+2,20,15,4,ink);gfx->drawLine(x+4,y+17,x+4,y+21,ink);gfx->drawLine(x+4,y+21,x+9,y+17,ink);gfx->drawLine(x+5,y+7,x+15,y+7,ink);gfx->drawLine(x+5,y+11,x+12,y+11,ink);break;
 case 4:gfx->fillRoundRect(x+2,y+3,6,11,3,ink);gfx->fillCircle(x+5,y+17,2,ink);gfx->fillRoundRect(x+12,y+7,6,11,3,ink);gfx->fillCircle(x+15,y+21,2,ink);break;
 case 5:gfx->drawRoundRect(x,y+5,18,12,2,ink);gfx->fillRect(x+19,y+9,2,4,ink);gfx->fillRect(x+3,y+8,5,6,ink);break;
 case 6:gfx->drawCircle(x+10,y+5,4,ink);gfx->drawRoundRect(x+2,y+13,16,9,4,ink);break;
 case 7:gfx->drawCircle(x+10,y+11,10,ink);gfx->drawLine(x+10,y+11,x+10,y+5,ink);gfx->drawLine(x+10,y+11,x+15,y+13,ink);break;
 default:gfx->drawLine(x+18,y+11,x+2,y+11,ink);gfx->drawLine(x+2,y+11,x+8,y+5,ink);gfx->drawLine(x+2,y+11,x+8,y+17,ink);break;
 }
}
void drawCornerBrackets() {} // retained API; the modern UI has no decorative frame.
// Layout helpers share the same font metrics as the renderer.
int uiWidth(const char* text,int scale=1) {
 int width=0;while(*text){uint32_t cp=nextCodepoint(text);width+=scale==2 ? premiumTitleAdvance(cp):premiumAdvance(cp)*scale;}return width;
}
void uiFit(const char* text,int x,int y,int maxWidth,uint16_t color,int preferred=2) {
 int scale=preferred;if(uiWidth(text,scale)>maxWidth)scale=1;
 if(uiWidth(text,scale)<=maxWidth){uiText(text,x,y,scale,color);return;}
 int used=0;const char* cursor=text;char clipped[160];size_t n=0;
 while(*cursor){const char* start=cursor;uint32_t cp=nextCodepoint(cursor);int advance=premiumAdvance(cp);size_t bytes=cursor-start;
  if(used+advance+uiWidth("...")>maxWidth || n+bytes+4>sizeof(clipped))break;
  memcpy(clipped+n,start,bytes);n+=bytes;used+=advance;
 }
 memcpy(clipped+n,"...",4);uiText(clipped,x,y,1,color);
}
void drawTopBar() {
 uiFontBg=themeBg;char label[24];
 if(currentScreen==SC_CLOCK)snprintf(label,sizeof(label),"%s",ownerName);
 else if(timeValid)snprintf(label,sizeof(label),"%02d:%02d",hh,mm);
 else snprintf(label,sizeof(label),"--:--");
 uiFit(label,16,11,68,UI_MUTED,1);
 if(unreadMessages>0){gfx->drawRoundRect(90,13,13,10,2,COL_ACCENT);gfx->drawLine(91,14,96,18,COL_ACCENT);gfx->drawLine(96,18,101,14,COL_ACCENT);snprintf(label,sizeof(label),"%d",unreadMessages);uiText(label,106,10,1,COL_ACCENT);}
 gfx->drawLine(130,10,130,25,chronosConnected ? COL_ACCENT:UI_MUTED);
 gfx->drawLine(126,13,134,21,UI_MUTED);gfx->drawLine(126,22,134,14,UI_MUTED);
 if(!chronosConnected)gfx->drawLine(125,25,136,10,UI_MUTED);
 char b[12];if(batPercent<0)snprintf(b,sizeof(b),"--%%");else snprintf(b,sizeof(b),"~%d%%",batPercent);
 uiText(b,148,11,1,batteryLow ? COL_WARN:COL_TEXT);
 gfx->drawRoundRect(205,14,17,10,2,UI_MUTED);gfx->fillRect(223,17,2,4,UI_MUTED);
 if(batPercent>0)gfx->fillRect(207,16,constrain(batPercent*13/100,1,13),6,batteryLow ? COL_WARN:COL_ACCENT);
}
void drawNavBar() {
 int selected=navigationIndex(currentScreen);
 for(int i=0;i<7;++i){int x=70+i*16;if(i==selected)gfx->fillRoundRect(x-2,274,12,3,1,COL_ACCENT);else gfx->fillCircle(x+2,275,1,UI_MUTED);}
}
void drawPageTitle(const char* title,uint16_t color) {uiFontBg=themeBg;uiFit(title,16,39,208,color);}
void uiFooter(const char* text) {
 uiFontBg=themeBg;char first[96]={};const char* action=text;
 bool normal=currentMode==MODE_NAV && !previewActive && !alarmRinging && !timerRinging && !moveReminderActive;
 if(normal)snprintf(first,sizeof(first),"1: дальше  2: назад");
 else {const char* hold=strstr(text,"Удерж:");if(hold&&hold!=text){size_t n=std::min<size_t>(sizeof(first)-1,hold-text);memcpy(first,text,n);first[n]=0;action=hold;}}
 if(first[0])uiFit(first,18,237,204,UI_MUTED,1);
 gfx->fillRoundRect(12,254,216,18,9,themePanel);uiFontBg=themePanel;
 // A hardware hint inside a capsule is still operated with BOOT, not touch.
 uiFit(action,22,253,196,COL_TEXT,1);drawNavBar();
}
void uiFieldPosition(int index,int count) {
 char text[32];snprintf(text,sizeof(text),"%02d / %02d",index+1,count);uiFontBg=themeBg;uiText(text,162,66,1,UI_MUTED);
}
void uiFocusField(const char* label,const char* value,const char* next,bool editing,int index,int count) {
 uiFieldPosition(index,count);
 uiCard(12,89,216,102,editing);
 uiText(label,24,101,1,UI_MUTED);
 uiFit(value,24,128,192,COL_TEXT);
 // Adjacent item gives context without shrinking the selected value.
 uiFontBg=themeBg;uiText(index==count-1 ? "Далее: сохранить":"Далее",16,204,1,UI_MUTED);
 if(index<count-1)uiFit(next,70,204,154,UI_MUTED,1);
 
}
static inline int uiGlyphA(const uint8_t* g,int sx,int sy){if(sx<0||sy<0||sx>=36||sy>=64)return 0;return (g[sy*9+sx/4]>>(6-2*(sx%4)))&3;}
void uiDigit(int digit,int x,int y,int w,int h,uint16_t color) {
 if(digit<0 || digit>9){return;}
 displayPending=true;
 const uint8_t* g=PREMIUM_SANS[digit];
 // Bilinear upsampling of the 2-bit mask gives smooth large numerals.
 for(int j=0;j<h;++j){float fy=(j+0.5f)*64.0f/h-0.5f;int sy=floorf(fy);float ty=fy-sy;
  for(int i=0;i<w;++i){float fx=(i+0.5f)*36.0f/w-0.5f;int sx=floorf(fx);float tx=fx-sx;
   float a=(uiGlyphA(g,sx,sy)*(1-tx)+uiGlyphA(g,sx+1,sy)*tx)*(1-ty)+(uiGlyphA(g,sx,sy+1)*(1-tx)+uiGlyphA(g,sx+1,sy+1)*tx)*ty;
   int v=(int)((a/3.0f-0.5f)*2.2f*255+128);if(v>0)nxPx(x+i,y+j,color,v>255?255:v);}}
}
void uiNumber(int value,int x,int y,int w,int h,uint16_t color) {displayPending=true;uiDigit(value/10,x,y,w,h,color);uiDigit(value%10,x+w+5,y,w,h,color);}
int uiNextAlarm() {
  syncAlarmSlot();int best=-1,wait=1441;
  if(!timeValid) return -1;
  for(int i=0;i<ALARM_COUNT;++i) if(alarmEnabledBank[i]) {
    int d=(alarmHours[i]*60+alarmMinutes[i]-hh*60-mm+1440)%1440;
    if(d==0 && ss>0) d=1440;
    if(d<wait) {wait=d;best=i;}
  }
  return best;
}
void specialSummary(char* out,size_t size) {
 uint32_t seconds=specialElapsed/1000;
 if(specialization==0) snprintf(out,size,"%02lu:%02lu:%02lu",(unsigned long)(seconds/3600),(unsigned long)(seconds/60%60),(unsigned long)(seconds%60));
 else if(specialization==1) {uint32_t left=(specialElapsed<1500000 ? 1500000:1800000)-specialElapsed;snprintf(out,size,"%02lu:%02lu",(unsigned long)((left+999)/1000/60),(unsigned long)((left+999)/1000%60));}
 else if(specialization==2) {uint32_t left=specialElapsed>=480000 ? 0:(specialElapsed%60000<40000 ? 40000:60000)-specialElapsed%60000;snprintf(out,size,"%02lu сек",(unsigned long)((left+999)/1000));}
 else if(specialization==3 && !timeValid) snprintf(out,size,"--:--");
 else if(specialization==3) {int t=(hh*60+mm+travelOffset+1440)%1440;snprintf(out,size,"%02d:%02d",t/60,t%60);}
 else if(specialization==4) snprintf(out,size,"%s %lu",specialElapsed<4000 ? "Вдох":"Выдох",(unsigned long)(specialElapsed<4000 ? (4000-specialElapsed+999)/1000:(10000-specialElapsed+999)/1000));
 else if(specialization==5) snprintf(out,size,"%u метки",fieldCount);
 else snprintf(out,size,"%.2f V",batteryVolts);
}
void uiComplication(uint8_t id,char* label,size_t labelSize,char* value,size_t valueSize){
 if(id==1){snprintf(label,labelSize,"Шаги");snprintf(value,valueSize,"%lu",(unsigned long)activityTodaySteps());}
 else if(id==2){snprintf(label,labelSize,"Таймер");uint32_t seconds=(timerRemainingMs+999)/1000;snprintf(value,valueSize,"%02lu:%02lu",(unsigned long)(seconds/60),(unsigned long)(seconds%60));}
 else if(id==3){snprintf(label,labelSize,"Будильник");int slot=uiNextAlarm();if(slot<0)snprintf(value,valueSize,"Не задан");else snprintf(value,valueSize,"%02u:%02u",alarmHours[slot],alarmMinutes[slot]);}
 else if(id==4){snprintf(label,labelSize,"Сообщения");snprintf(value,valueSize,"%d новых",unreadMessages);}
 else if(id==5){snprintf(label,labelSize,"Батарея");if(batPercent<0)snprintf(value,valueSize,"Нет данных");else snprintf(value,valueSize,"~%d%%",batPercent);}
 else{label[0]=value[0]=0;}
}
void drawStatusRows() {
 char value[40];specialSummary(value,sizeof(value));if(specialization==0)snprintf(value,sizeof(value),"%lu",(unsigned long)activityTodaySteps());
 const char* shortNames[]={"Шаги","Фокус","Интервалы","Второе время","Ритм","Метки","Система"};
 int a=uiNextAlarm();char auxiliary[32];const char* auxiliaryLabel="Будильник";
 if(timerRunning){uint32_t seconds=(timerRemainingMs+999)/1000;snprintf(auxiliary,sizeof(auxiliary),"%02lu:%02lu",(unsigned long)(seconds/60),(unsigned long)(seconds%60));auxiliaryLabel="Таймер";}
 else if(unreadMessages){snprintf(auxiliary,sizeof(auxiliary),"%d новых",unreadMessages);auxiliaryLabel="Сообщения";}
 else if(a<0)snprintf(auxiliary,sizeof(auxiliary),"Не задан");else snprintf(auxiliary,sizeof(auxiliary),"%02u:%02u",alarmHours[a],alarmMinutes[a]);
 char primaryLabel[32];snprintf(primaryLabel,sizeof(primaryLabel),"%s",shortNames[specialization]);char auxLabel[32];snprintf(auxLabel,sizeof(auxLabel),"%s",auxiliaryLabel);
 if(faceCompA)uiComplication(faceCompA,primaryLabel,sizeof(primaryLabel),value,sizeof(value));
 if(faceCompB)uiComplication(faceCompB,auxLabel,sizeof(auxLabel),auxiliary,sizeof(auxiliary));
 int layout=themeIndex<7 ? WATCH_LOOKS[themeIndex].layout:0;
 if(layout==1 || layout==5 || layout==4 || layout==6){
  bool useAux=faceCompB!=6&&(faceCompB!=0||timerRunning||unreadMessages);if(!useAux&&faceCompA==6)return;
  uiCard(14,216,212,20);uiFit(useAux?auxLabel:primaryLabel,23,216,103,UI_MUTED,1);
  uiFit(useAux?auxiliary:value,132,216,85,COL_TEXT,1);return;
 }
 if(faceCompA!=6){uiCard(14,199,103,35);uiFit(primaryLabel,23,199,86,UI_MUTED,1);uiFit(value,23,216,86,COL_TEXT,1);}
 if(faceCompB!=6){uiCard(123,199,103,35);uiFit(auxLabel,132,199,86,UI_MUTED,1);uiFit(auxiliary,132,216,86,COL_TEXT,1);}
}
void drawDate() {
 if(!timeValid) return;
 const char* days[]={"ВС","ПН","ВТ","СР","ЧТ","ПТ","СБ"};char date[40];
 snprintf(date,sizeof(date),"%s  %02d / %02d",days[constrain(wday,0,6)],day,mon);
 uiFontBg=themeBg;uiCenter(date,42,UI_MUTED);
}
void uiOrbit(int cx,int cy,int r,uint16_t color,int step=6) {(void)step;nxArc(cx,cy,r,1.4f,0,360,color);}
void uiRing(int cx,int cy,int radius,int thickness,float fraction,uint16_t ink,uint16_t background) {
 fraction=std::max(0.0f,std::min(1.0f,fraction));nxArc(cx,cy,radius,thickness,0,360,background);nxArc(cx,cy,radius,thickness,0,fraction*360,ink);
}
void uiHand(float angle,int length,uint16_t color,int width) {nxHand(120,136,angle,length,length*0.18f,width+0.6f,color);}
// Seven original faces; no animation loop or new persistent settings.
void uiAnalog(bool atelier) {
 uiFontBg=themeBg;
 if(!atelier)uiOrbit(120,136,74,colorMix(COL_ACCENT,themeBg,1,4),4);
 for(int i=0;i<60;++i){
  if(atelier && i%5)continue;
  float a=i*0.1047197551f;int r=i%5==0 ? 65:72;
  gfx->drawLine(120+sinf(a)*r,136-cosf(a)*r,120+sinf(a)*74,136-cosf(a)*74,i%5==0 ? COL_ACCENT:colorMix(UI_MUTED,themeBg,1,3));
 }
 if(atelier){uiText("12",109,75,1,COL_TEXT);uiText("6",115,177,1,COL_TEXT);}
 else {uiText("12",109,75,1,COL_TEXT);uiText("3",175,126,1,COL_TEXT);uiText("6",115,177,1,COL_TEXT);uiText("9",51,126,1,COL_TEXT);}
 if(timeValid){
  uiHand((hh%12+mm/60.0f)*30,37,COL_TEXT,atelier?5:3);
  uiHand((mm+(faceSeconds?ss/60.0f:0))*6,56,COL_ACCENT,atelier?3:2);
  if(faceSeconds)uiHand(ss*6,64,COL_ACCENT2,1);
  gfx->fillCircle(120,136,4,COL_ACCENT);gfx->fillCircle(120,136,1,themeBg);
 }
}
void drawTimeBig(int h,int m,int sec) {
 int layout=themeIndex<7 ? WATCH_LOOKS[themeIndex].layout:(clockStyle ? 2:0);
 if(!timeValid){uiCard(20,91,200,88);uiCenter("--:--",106,COL_TEXT,3);uiCenter("Установите время",160,UI_MUTED);return;}
 int hour=use24h ? h:(h%12 ? h%12:12);uiFontBg=themeBg;
 if(layout==1 || layout==5){uiAnalog(layout==5);return;}
 if(layout==2){ // GLACIER: generous stacked numerals, quiet offset rule.
  gfx->fillRoundRect(22,77,3,108,1,COL_ACCENT);
  uiNumber(hour,48,67,45,58,COL_TEXT);uiNumber(m,100,132,45,58,themeIndex==7?COL_ACCENT2:COL_ACCENT);return;
 }
 if(layout==4){ // NEBULA: two orbital tracks, legible central time.
  uiOrbit(120,135,73,colorMix(COL_ACCENT2,themeBg,1,4),4);
  uiRing(120,135,65,3,faceSeconds?(sec+1)/60.0f:0,COL_ACCENT,colorMix(COL_ACCENT,themeBg,1,6));
  uiNumber(hour,57,112,25,46,COL_TEXT);uiNumber(m,128,112,25,46,COL_TEXT);
  gfx->fillCircle(120,128,2,COL_ACCENT);gfx->fillCircle(120,145,2,COL_ACCENT);return;
 }
 if(layout==6){ // VECTOR: activity instrument, progress also readable as a number.
  uiNumber(hour,21,88,40,70,COL_TEXT);uiNumber(m,131,88,40,70,COL_TEXT);
  gfx->fillCircle(120,114,3,COL_ACCENT);gfx->fillCircle(120,137,3,COL_ACCENT);
  float progress=stepGoal ? std::min(1.0f,(float)activityTodaySteps()/stepGoal):0;
  for(int i=0;i<20;++i)gfx->fillRoundRect(21+i*10,177,7,5,2,i<progress*20?COL_ACCENT:themePanel);
  char label[24];snprintf(label,sizeof(label),"%lu%% цели",(unsigned long)(stepGoal?std::min<uint32_t>(999,activityTodaySteps()*100/stepGoal):0));uiCenter(label,187,UI_MUTED);return;
 }
 if(layout==3){ // EMBER: split chronograph tiles.
  uiCard(14,77,103,100);uiCard(123,77,103,100);uiFontBg=themePanel;
  uiNumber(hour,24,89,39,72,COL_TEXT);uiNumber(m,133,89,39,72,COL_ACCENT);
  if(faceSeconds)for(int i=0;i<30;++i)gfx->fillRoundRect(17+i*7,186,4,3,1,i<=sec/2?COL_ACCENT:themePanel);
  return;
 }
 // ONYX: time first; no decorative progress that could be mistaken for activity.
 uiNumber(hour,21,82,40,88,COL_TEXT);uiNumber(m,131,82,40,88,themeIndex==7?COL_ACCENT2:COL_TEXT);
 if(!faceSeconds || !colonBlink || colonOn){gfx->fillCircle(120,113,3,COL_ACCENT);gfx->fillCircle(120,140,3,COL_ACCENT);}
 if(faceSeconds){char tiny[8];snprintf(tiny,sizeof(tiny),"%02d",sec);uiFontBg=themeBg;uiCenter(tiny,178,COL_ACCENT);}
}

void drawSecondsBar(int) {} // seconds are part of each themed face
#include "NexusOsdUI.h"
#include "NexusFaces.h"
// LayerFace hooks into the clock only; other UI screens stay native.
#include "NexusLayerFace.h"
void drawClockScreen() {
 nxlfLoad();if(nxlfActive){nxlfDraw();return;}
 if(osd.enabled){uiBackground();drawOsdClock();return;}
 if(themeIndex<7){drawReferenceFace();return;}
 uiBackground();drawTopBar();uiFontBg=themeBg;
 if(doNotDisturb)uiCenter("НЕ БЕСПОКОИТЬ",42,COL_ACCENT);else drawDate();
 if(!use24h && timeValid){uiFontBg=themeBg;uiText(hh<12 ? "AM":"PM",198,42,1,UI_MUTED);}
 const char* hints[]={"Удерж: секундомер","Удерж: фокус","Удерж: интервалы","Удерж: второе время","Удерж: дыхание","Удерж: метки","Удерж: система"};
 drawTimeBig(hh,mm,ss);drawStatusRows();uiFooter(hints[specialization]);
}
void updateClockScreen() {if(colonBlink) colonOn=!colonOn;drawClockScreen();}

void uiTimeToolRail() {
 int selected=currentScreen==SC_TIMER ? 5:alarmSlot;
 bool picking=currentMode==MODE_EDIT && (currentScreen==SC_TIMER ? timerCursor:alarmCursor)==0;
 for(int i=0;i<6;++i){int x=14+i*35;char label[4];if(i==5)snprintf(label,sizeof(label),"Т");else snprintf(label,sizeof(label),"%d",i+1);
  if(i==selected){gfx->fillRoundRect(x,71,32,20,7,themePanel);gfx->drawRoundRect(x,71,32,20,7,COL_ACCENT);if(picking)gfx->drawRoundRect(x+1,72,30,18,6,COL_ACCENT);uiFontBg=themePanel;}else uiFontBg=themeBg;
  uiText(label,x+(32-uiWidth(label))/2,72,1,i==selected ? COL_TEXT:UI_MUTED);
 }
}
void drawAlarmScreen() {
  uiBackground();drawTopBar();char title[40];snprintf(title,sizeof(title),"Будильник %d",alarmSlot+1);drawPageTitle(title,COL_TEXT);
  uiTimeToolRail();
  uiCard(14,98,212,87);
  uiNumber(alarmH,26,105,38,70,currentMode==MODE_EDIT && alarmCursor==1 ? COL_ACCENT:COL_TEXT);
  uiNumber(alarmM,130,105,38,70,currentMode==MODE_EDIT && alarmCursor==2 ? COL_ACCENT:COL_TEXT);
  if(currentMode==MODE_EDIT && (alarmCursor==1 || alarmCursor==2)) {int x=alarmCursor==1 ? 22:126;gfx->drawRoundRect(x,101,91,80,9,COL_ACCENT);gfx->fillRoundRect(x+28,178,35,3,1,COL_ACCENT);}
  gfx->fillCircle(117,128,3,COL_ACCENT);gfx->fillCircle(117,150,3,COL_ACCENT);
  uiCard(14,196,212,36,currentMode==MODE_EDIT && alarmCursor==3);
  uiText(alarmEnabled ? "Включён":"Выключен",28,205,1,COL_TEXT);
  gfx->fillRoundRect(174,205,34,17,8,alarmEnabled ? COL_ACCENT:0x528A);gfx->fillCircle(alarmEnabled ? 199:183,213,6,COL_TEXT);
  uiFooter(currentMode==MODE_EDIT ? (alarmCursor==0 ? "1/2: выбор  Удерж: далее":(alarmCursor==3 ? "1/2: вкл/выкл Удерж: сохранить":"1:+ 2:- Удерж: далее")) : "Удерж: настройка");
}
void drawTimerScreen() {
  uiBackground();drawTopBar();drawPageTitle("Таймер",COL_TEXT);
  uiTimeToolRail();
  uint32_t seconds=(timerRemainingMs+999)/1000;
  uiCard(14,98,212,87);
  uiNumber(seconds/60,26,105,38,70,currentMode==MODE_EDIT && timerCursor==1 ? COL_ACCENT:COL_TEXT);
  uiNumber(seconds%60,130,105,38,70,currentMode==MODE_EDIT && timerCursor==2 ? COL_ACCENT:COL_TEXT);
  if(currentMode==MODE_EDIT && (timerCursor==1 || timerCursor==2)) {int x=timerCursor==1 ? 22:126;gfx->drawRoundRect(x,101,91,80,9,COL_ACCENT);gfx->fillRoundRect(x+28,178,35,3,1,COL_ACCENT);}
  gfx->fillCircle(117,128,3,COL_ACCENT);gfx->fillCircle(117,150,3,COL_ACCENT);
  uiCard(14,196,212,36,currentMode==MODE_EDIT && timerCursor==3);
  uiText(timerRunning ? "Идёт отсчёт":(timerRemainingMs ? "Готов / пауза":"Завершён"),28,205);
  uiFooter(currentMode==MODE_EDIT ? (timerCursor==0 ? "1/2: выбор  Удерж: далее":(timerCursor==3 ? "1: пуск/пауза 2: сброс Удерж: сохранить":"1:+ 2:-  Удерж: далее")) : "Удерж: настройка");
}
void drawSettingsSections() {
 uiBackground();drawTopBar();drawPageTitle("Настройки",COL_TEXT);
 int index=settingsSectionCursor,previous=(index+SETTINGS_SECTION_COUNT-1)%SETTINGS_SECTION_COUNT,next=(index+1)%SETTINGS_SECTION_COUNT;
 uiFieldPosition(index,SETTINGS_SECTION_COUNT);
 uiFontBg=themeBg;uiText(SETTINGS_SECTIONS[previous].name,25,87,1,UI_MUTED);
 uiCard(12,116,216,71,true);gfx->fillCircle(39,146,17,COL_ACCENT);uiIcon(SETTINGS_SECTIONS[index].icon,29,135,themeBg);
 uiFit(SETTINGS_SECTIONS[index].name,65,123,149,COL_TEXT);
 uiFit(SETTINGS_SECTIONS[index].hint,65,158,149,UI_MUTED,1);
 uiFontBg=themeBg;uiText(SETTINGS_SECTIONS[next].name,25,203,1,UI_MUTED);
 uiFooter("1: дальше  2: назад Удерж: открыть");
}
void drawSettingsScreen() {
 uiBackground();drawTopBar();
 if(currentMode==MODE_NAV){
  drawPageTitle("Настройки",COL_TEXT);
  uiCard(12,89,216,84);uiIcon(0,27,108,COL_ACCENT);uiText("Настроить часы",63,99,1,COL_TEXT);uiText("Экран, связь, звук",63,123,1,UI_MUTED);uiText("8 разделов",63,146,1,UI_MUTED);
  uiFontBg=themeBg;uiText("Выберите нужный раздел",16,196,1,UI_MUTED);uiText("и измените его параметры",16,213,1,UI_MUTED);
  uiFooter("Удерж: разделы");return;
 }
 drawPageTitle(groupedSettings() ? SETTINGS_SECTIONS[settingsActiveSection].name:"Настройки",COL_TEXT);
 int i=settingsCursor;char value[48];
    if(i==0) snprintf(value,sizeof(value),"%u%%",brightness*100/255);
    else if(i==1) snprintf(value,sizeof(value),"%s",use24h ? "24 часа":"12 часов");
    else if(i==2) snprintf(value,sizeof(value),"%s",colonBlink ? "Вкл":"Выкл");
    else if(i==3) {if(SCREEN_TIMEOUTS[screenTimeoutIndex]) snprintf(value,sizeof(value),"%u сек",SCREEN_TIMEOUTS[screenTimeoutIndex]);else snprintf(value,sizeof(value),"Постоянно");}
    else if(i==4 || i==5 || i==9 || i==12) snprintf(value,sizeof(value),"%s",(i==4 ? quietMode:(i==5 ? privatePreview:(i==9 ? autoReconnect:wallpaperEnabled))) ? "Вкл":"Выкл");
    else if(i==7) snprintf(value,sizeof(value),"%s",themeIndex<7 ? WATCH_LOOKS[themeIndex].name:"Своя тема");
    else if(i==8) snprintf(value,sizeof(value),"%s",ownerName);
    else if(i==10 && forgetFailed) snprintf(value,sizeof(value),"Ошибка");
    else if(i==11 && pairWindowActive) snprintf(value,sizeof(value),"Доступно 2 минуты");
    else if(i==19) snprintf(value,sizeof(value),"%s",activityEnabled ? "Вкл":"Выкл");
    else if(i==16 || i==18) snprintf(value,sizeof(value),"%s",(i==16 ? doNotDisturb:ecoMode) ? "Вкл":"Выкл");
    else if(i==15) snprintf(value,sizeof(value),"%s",SPECIAL_NAMES[specialization]);
    else if(i==13) snprintf(value,sizeof(value),"%s",clockStyle ? "Две строки":"Классический");
    else snprintf(value,sizeof(value),"Двойной клик");
 uiFocusField(SETTINGS_LABELS[i],value,SETTINGS_LABELS[settingsNextField()],true,settingsVisibleIndex(),settingsVisibleCount());
 bool last=settingsVisibleIndex()+1>=settingsVisibleCount();
 const char* action=(i==6 || i==10 || i==11 || i==14 || i==17) ? (last ? "2: выполнить Удерж: сохранить":"2: выполнить Удерж: далее"):(last ? "1/2: изменить Удерж: сохранить":"1/2: изменить Удерж: далее");
 uiFooter(action);
}
void drawMessagesScreen() {
  uiBackground();drawTopBar();drawPageTitle("Сообщения",COL_TEXT);
  if(!messageCount) {uiCard(12,86,216,132);uiText("Пока пусто",32,108,2);uiText("Подключите Chronos",27,155,1,UI_MUTED);uiText("на телефоне",27,176,1,UI_MUTED);}
  else {
    messageIndex=constrain(messageIndex,0,messageCount-1);auto& m=messages[messageIndex];
    if(currentMode==MODE_EDIT && m.unread) {m.unread=false;if(unreadMessages) --unreadMessages;}
    uiCard(8,78,224,157);drawTextLine(m.app,16,84,COL_ACCENT);drawTextLine(m.title,16,100,COL_TEXT);
    int pages=drawTextPage(m.body,messagePage,12,118,6,COL_TEXT);char meta[40];
    snprintf(meta,sizeof(meta),"%s  %d/%d  %d/%d",m.stamp,messageIndex+1,messageCount,messagePage+1,pages);uiText(meta,16,221,1,UI_MUTED);
  }
  uiFooter(currentMode==MODE_EDIT ? "1:лист 2:старше Удерж:выход":"Удерж: читать");
}
void drawNotificationPreview() {
 uiBackground();drawTopBar();gfx->fillCircle(31,54,15,COL_ACCENT);uiIcon(3,21,43,themeBg);uiFontBg=themeBg;uiFit(previewMessage.app,55,42,167,COL_TEXT);
 uiCard(8,82,224,150);
 if(privatePreview)uiText("Откройте сообщения",20,132);
 else{drawTextLine(previewMessage.title,20,94,COL_TEXT);drawTextPage(previewMessage.body,0,12,120,6,COL_TEXT);}
 uiFooter("Нажмите, чтобы закрыть");
}
void drawAlertScreen() {
 uiBackground();drawTopBar();drawPageTitle(alarmRinging ? "Будильник":"Время вышло",COL_TEXT);
 int slot=constrain(ringingAlarmSlot,0,ALARM_COUNT-1);int h=alarmRinging ? alarmHours[slot]:0,m=alarmRinging ? alarmMinutes[slot]:0;
 uiFontBg=themeBg;uiNumber(h,43,102,31,61,COL_ACCENT);uiNumber(m,132,102,31,61,COL_ACCENT);
 gfx->fillCircle(122,123,2,COL_ACCENT);gfx->fillCircle(122,144,2,COL_ACCENT);
 gfx->fillRoundRect(12,190,216,40,20,COL_ACCENT);uiFontBg=COL_ACCENT;uiCenter("Остановить",196,themeBg,2);
 uiFooter("Нажмите для остановки");
}
void drawMenuScreen() {
  uiBackground();drawTopBar();drawPageTitle("Устройство",COL_TEXT);
  char line[40];
  uiCard(12,78,216,66);uiText("Батарея / ёмкость задана",24,84,1,UI_MUTED);
  if(batPercent>=0)snprintf(line,sizeof(line),"%u мАч  /  %.2f V",batteryCapacityMah,batteryVolts);else snprintf(line,sizeof(line),"%u мАч  /  нет данных",batteryCapacityMah);uiFit(line,24,105,192,COL_TEXT,1);
  uiText("Зарядка: нет данных",24,123,1,UI_MUTED);
  uiCard(12,152,216,38);uiText("Телефон",24,152,1,UI_MUTED);
  uiText(chronosConnected ? (peerBonded ? "Подключён, сохранён":"Подключён"):(savedPeer[0] ? "Сохранён, нет связи":"Не подключён"),24,171);
  uiCard(12,198,216,36);uiText("Фон и настройки",24,198,1,UI_MUTED);
  uiText(!themeFsReady ? "Нет раздела LittleFS":(wallpaperPresent ? "Свой фон загружен":"USB: Theme Studio"),24,216);
  uiFooter(prefsOK ? "Удерж: тема и профиль":"Ошибка хранения настроек");
}
void refreshMenuValues() {drawMenuScreen();}
void drawModernProfile() {
  uiBackground();drawTopBar();drawPageTitle("Ваше имя",COL_TEXT);uiCard(10,90,220,91);
  for(int i=0;i<8;++i){uint32_t cp=(uint8_t)ownerDraft[i];premiumTitleDraw(cp,22+i*25+(21-premiumTitleAdvance(cp))/2,117,COL_TEXT,themePanel);if(i==profileCursor){gfx->drawRoundRect(17+i*25,107,24,53,5,COL_ACCENT);gfx->fillRoundRect(21+i*25,154,16,3,1,COL_ACCENT);}}
  uiText("Bluetooth: после перезапуска",12,205,1,UI_MUTED);uiFooter(profileCursor==7 ? "1/2: буква Удерж: сохранить":"1/2: буква Удерж: далее");
}
void drawModernDate() {
  uiBackground();drawTopBar();drawPageTitle("Дата и время",COL_TEXT);
  int values[]={draftH,draftM,draftD,draftMo,2000+draftY};
  if(currentMode==MODE_NAV) {values[0]=hh;values[1]=mm;values[2]=day;values[3]=mon;values[4]=2000+yr;}
  const char* labels[]={"Час","Минута","День","Месяц","Год"};
  int i=currentMode==MODE_EDIT ? timeCursor:0;char value[24];snprintf(value,sizeof(value),"%02d",values[i]);
  uiFocusField(labels[i],value,labels[std::min(i+1,4)],currentMode==MODE_EDIT,i,5);
  uiFooter(currentMode==MODE_EDIT ? (timeCursor==4 ? "1:+ 2:- Удерж: сохранить":"1:+ 2:- Удерж: далее"):"Удерж: установить");
}

void drawSpecialization() {
 uiBackground();drawTopBar();drawPageTitle(SPECIAL_NAMES[specialization],COL_TEXT);
 uiFontBg=themeBg;uiText(SPECIAL_SUBTITLES[specialization],16,76,1,UI_MUTED);
 char value[48];specialSummary(value,sizeof(value));
 uiCard(12,104,216,113);
 if(specialization==5) {
  if(!fieldCount) uiText("Пока нет меток",29,143);
  else for(int i=0;i<fieldCount;++i) uiText(fieldMarks[i],25,113+i*30,1,i==0 ? COL_ACCENT:COL_TEXT);
 } else if(specialization==6) {
  snprintf(value,sizeof(value),"%.2f V  %u KB",batteryVolts,(unsigned)(ESP.getFreeHeap()/1024));uiText(value,25,114);
  uiText(chronosConnected ? "BLE: подключён":"BLE: нет связи",25,144,1,UI_MUTED);uiText(rtcOK ? "RTC: время достоверно":"RTC: нет / ошибка",25,170,1,UI_MUTED);
  snprintf(value,sizeof(value),"RX %lu / SUB %d",(unsigned long)notificationRx,watch.isSubscribed());uiText(value,25,193,1,UI_MUTED);
 } else if(specialization==4) {
  int r=14+(specialElapsed<4000 ? specialElapsed*22/4000:(10000-specialElapsed)*22/6000);
  gfx->fillCircle(120,153,r,COL_ACCENT);gfx->fillCircle(120,153,r-3,UI_CARD);uiCenter(value,182,COL_TEXT);
 } else {
  uiCenter(value,124,COL_ACCENT,2);
  if(specialization==3) {
   int offset=travelOffset,absOffset=offset<0 ? -offset:offset;
   snprintf(value,sizeof(value),"%c%02d:%02d от местного",offset<0 ? '-':'+',absOffset/60,absOffset%60);uiCenter(value,168,UI_MUTED);
   int local=hh*60+mm+travelOffset;if(timeValid && (local<0 || local>=1440)) uiCenter(local<0 ? "Предыдущий день":"Следующий день",190,UI_MUTED);
  } else if(specialization==0 && specialLap) {snprintf(value,sizeof(value),"Круг %lu.%lu сек",(unsigned long)(specialLap/1000),(unsigned long)(specialLap%1000/100));uiCenter(value,170,UI_MUTED);}
  else if(specialization==1) uiCenter(!specialRunning ? (specialElapsed ? "Пауза":"Готов"):(specialElapsed<1500000 ? "Работа":"Перерыв"),171,UI_MUTED);
  else if(specialization==2) {snprintf(value,sizeof(value),"Круг %lu/8 / %s",(unsigned long)std::min<uint32_t>(8u,specialElapsed/60000+1),specialElapsed>=480000 ? "Готово":(!specialRunning ? "Пауза":(specialElapsed%60000<40000 ? "Работа":"Отдых")));uiCenter(value,171,UI_MUTED);}
 }
 uiFontBg=themeBg;
 if(currentMode==MODE_EDIT) {
  if(specialization==3) uiFooter("1/2: +/-30м  Удерж: выход");
  else if(specialization==5) uiFooter("1: метка 2: очистить Удерж: выход");
  else if(specialization==6) uiFooter("1: звук 2: обновить Удерж: выход");
  else uiFooter(specialization==0 && specialRunning ? "1: пауза 2: круг Удерж: выход":"1: пуск/пауза 2: сброс Удерж: выход");
 } else uiFooter("Удерж: управление");
}

void drawStylePicker() {
 uiBackground();drawTopBar();drawPageTitle("Оформление",COL_TEXT);
 const char* names[]={"Numix","Track","Rings","Latte","Poster","Terminal","Стрелочный","Классический"};
 const char* labels[]={"Тема","Вид циферблата","Специализация"};
 for(int i=0;i<3;++i){int y=76+i*53;uiCard(12,y,216,47,styleCursor==i);uiText(labels[i],24,y+1,1,UI_MUTED);uiFit(i==0?(themeIndex<7?WATCH_LOOKS[themeIndex].name:"PERSONAL"):(i==1?names[faceVariant]:SPECIAL_NAMES[specialization]),24,y+20,190,COL_TEXT,1);}
 uiFooter(styleCursor==2?"1/2: выбрать Удерж: сохранить":"1/2: выбрать Удерж: далее");
}
void drawActivityScreen() {
 uiBackground();drawTopBar();drawPageTitle("Активность",COL_TEXT);char text[48];uiFontBg=themeBg;
 const char* status=!activityEnabled ? "Подсчёт выключен":(!activityTaskOK ? "Ошибка задачи датчика":(!imuPresent ? "Датчик недоступен":(!activityHaveSample || (uint32_t)(millis()-activitySampleAt)>2000 ? "Жду данные датчика":(timeValid ? "Шаги сегодня":"Шаги / без даты"))));
 uiFit(status,16,76,208,UI_MUTED,1);
 uint32_t steps=activityTodaySteps();uiRing(56,133,35,8,(float)steps/stepGoal,COL_ACCENT,colorMix(COL_ACCENT,themeBg,1,6));
 uiIcon(4,46,111,COL_ACCENT);snprintf(text,sizeof(text),"%lu%%",(unsigned long)std::min<uint32_t>(999,steps*100/stepGoal));uiFontBg=themeBg;uiText(text,56-uiWidth(text)/2,143,1,COL_TEXT);
 snprintf(text,sizeof(text),"%lu",(unsigned long)steps);int digits=strlen(text),w=digits>=6 ? 15:(digits>=5 ? 18:23);int x=108;
 for(int i=0;i<digits;++i)uiDigit(text[i]-'0',x+i*(w+3),111,w,41,COL_TEXT);
 snprintf(text,sizeof(text),"из %lu",(unsigned long)stepGoal);uiFontBg=themeBg;uiText(text,108,156,1,UI_MUTED);
 uiCard(12,187,103,44);uiText("Дистанция",22,189,1,UI_MUTED);
 if(activityStrideMm()>0)snprintf(text,sizeof(text),"~%.2f км",activityDistanceKm());else snprintf(text,sizeof(text),"Нужен рост");uiFit(text,22,209,84,COL_TEXT,1);
 uiCard(125,187,103,44);uiText("Калории",135,189,1,UI_MUTED);
 if(activityStrideMm()>0 && personWeightDeciKg)snprintf(text,sizeof(text),"~%.0f ккал",activityWalkingKcal());else snprintf(text,sizeof(text),"Нужен вес");uiFit(text,135,209,84,COL_TEXT,1);
 uiFooter("Удерж: профиль / разминка");
}
void drawActivityProfile() {
 uiBackground();drawTopBar();drawPageTitle("Параметры",COL_TEXT);
 const char* labels[]={"Рост","Вес","Длина шага","Чувствительность","Цель шагов","Разминка через","Напоминать","Подсчёт шагов"};
 int i=activityProfileCursor;{char value[48];
  if(i==0){if(personHeightCm)snprintf(value,sizeof(value),"%u см",personHeightCm);else snprintf(value,sizeof(value),"Укажите свой рост");}
  else if(i==1){if(personWeightDeciKg)snprintf(value,sizeof(value),"%.1f кг",personWeightDeciKg/10.0f);else snprintf(value,sizeof(value),"Укажите свой вес");}
  else if(i==2){if(personStrideMm)snprintf(value,sizeof(value),"%.1f см",personStrideMm/10.0f);else snprintf(value,sizeof(value),"Авто по росту");}
  else if(i==3)snprintf(value,sizeof(value),"%s",activitySensitivity==0 ? "Ниже":(activitySensitivity==1 ? "Обычная":"Выше"));
  else if(i==4)snprintf(value,sizeof(value),"%lu",(unsigned long)stepGoal);
  else if(i==5)snprintf(value,sizeof(value),"%u мин",moveIntervalMin);
  else if(i==6)snprintf(value,sizeof(value),"%s",!moveRemindersEnabled ? "Выкл":(NEXUS_HAPTIC_PIN>=0 ? "Экран + вибро":(quietMode ? "Только экран":"Экран + зуммер")));
  else snprintf(value,sizeof(value),"%s",activityEnabled ? "Вкл":"Выкл");
  uiFocusField(labels[i],value,labels[std::min(i+1,7)],true,i,8);
 }
 uiFooter(i==7 ? "1/2: изменить  Удерж:сохранить":"1:+ 2:- Удерж:далее");
}
void drawMovementReminder() {
 uiBackground();drawTopBar();drawPageTitle("Пора размяться",COL_TEXT);uiCard(12,87,216,142);
 gfx->fillCircle(120,113,8,COL_ACCENT);gfx->drawLine(120,122,120,157,COL_ACCENT);gfx->drawLine(120,134,95,145,COL_ACCENT);gfx->drawLine(120,134,146,120,COL_ACCENT);gfx->drawLine(120,157,104,176,COL_ACCENT);gfx->drawLine(120,157,137,176,COL_ACCENT);
 uiCenter("Давно не было ходьбы",187,COL_TEXT);uiFooter("Нажмите, чтобы закрыть");
}
