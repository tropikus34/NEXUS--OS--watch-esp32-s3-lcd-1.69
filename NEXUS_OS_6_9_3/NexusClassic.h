#pragma once
void legacyAnalog(bool atelier) {
 uiFontBg=themeBg;uiOrbit(120,136,79,colorMix(COL_ACCENT,themeBg,1,3),4);
 for(int i=0;i<60;++i) {
  float a=i*0.1047197551f;int r=i%5==0 ? 68:74;
  gfx->drawLine(120+sinf(a)*r,136-cosf(a)*r,120+sinf(a)*76,136-cosf(a)*76,i%5==0 ? COL_ACCENT:colorMix(UI_MUTED,themeBg,1,2));
 }
 uiText(atelier ? "12":"XII",109,71,1,COL_TEXT);uiText(atelier ? "3":"III",177,126,1,COL_TEXT);uiText(atelier ? "6":"VI",115,181,1,COL_TEXT);uiText(atelier ? "9":"IX",48,126,1,COL_TEXT);
 if(timeValid) {
  uiHand((hh%12+mm/60.0f)*30,39,COL_TEXT,3);uiHand((mm+ss/60.0f)*6,58,COL_ACCENT,2);if(faceSeconds)uiHand(ss*6,65,UI_MUTED,1);
  gfx->fillCircle(120,136,4,COL_ACCENT);gfx->fillCircle(120,136,1,themeBg);
  char date[12];snprintf(date,sizeof(date),"%02d",day);if(faceDate)uiText(date,145,150,1,UI_MUTED);
 }
}
void legacyTime(int h,int m,int sec) {
 int layout=themeIndex<7 ? WATCH_LOOKS[themeIndex].layout:(clockStyle ? 2:0);
 if(!timeValid) {uiCard(20,91,200,88);uiCenter("--:--",106,COL_TEXT,3);uiCenter("Установите время",160,UI_MUTED);return;}
 int hour=use24h ? h:(h%12 ? h%12:12);
 if(layout==1 || layout==5) {legacyAnalog(layout==5);return;}
 if(layout==2) {
  uiCard(23,79,194,118);uiNumber(hour,46,88,55,51,COL_TEXT);uiNumber(m,90,139,42,47,COL_ACCENT);
  uiText("H",174,97,1,UI_MUTED);uiText("M",67,152,1,UI_MUTED);return;
 }
 if(layout==4) {
  uiOrbit(120,138,81,colorMix(COL_ACCENT,themeBg,1,3),5);uiOrbit(120,138,73,colorMix(COL_ACCENT2,themeBg,1,4),5);
  float t=sec*0.1047197551f;if(faceSeconds)gfx->fillCircle(120+sinf(t)*81,138-cosf(t)*81,4,COL_ACCENT);
  uiCard(38,106,164,67);uiNumber(hour,48,111,29,55,COL_TEXT);uiNumber(m,129,111,29,55,COL_ACCENT2);
  gfx->fillCircle(119,132,2,COL_ACCENT);gfx->fillCircle(119,148,2,COL_ACCENT);return;
 }
 uiCard(14,88,212,94);uiNumber(hour,25,96,38,76,COL_TEXT);uiNumber(m,130,96,38,76,COL_ACCENT2);
 if(!colonBlink || colonOn) {gfx->fillCircle(117,117,2,COL_ACCENT);gfx->fillCircle(117,139,2,COL_ACCENT);}
 if(!faceSeconds)return;
 if(layout==3) {
  for(int i=0;i<30;++i) gfx->fillRect(23+i*6,186,i%5==0 ? 2:1,i%5==0 ? 8:4,i<=sec/2 ? COL_ACCENT:0x6B4D);
 } else if(layout==6) {
  for(int i=0;i<12;++i) gfx->fillRect(22+i*16,187,11,3,i<=sec/5 ? COL_ACCENT:0x2145);
 } else {gfx->fillRoundRect(22,189,142,2,1,colorMix(COL_ACCENT,themeBg,1,4));if(sec>0)gfx->fillRoundRect(22,189,142*sec/59,2,1,COL_ACCENT);}
 char tiny[20];snprintf(tiny,sizeof(tiny),"%02d%s",sec,use24h ? "":(h<12 ? "AM":"PM"));uiFontBg=themeBg;uiText(tiny,182,178,1,COL_ACCENT);
}
