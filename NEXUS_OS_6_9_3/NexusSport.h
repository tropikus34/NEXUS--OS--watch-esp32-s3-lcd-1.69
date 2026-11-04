#pragma once
// Sport layouts: all gauges use measured/estimated device data, never a fake pulse.
void sportMeter(int x,int y,int w,float p,uint16_t c){nxLine(x,y,x+w,y,5,c,42);if(p>0)nxLine(x,y,x+w*std::min(1.0f,p),y,5,c);}
void sportLoop(int y,float p,uint16_t c,int r){
 nxLine(25,y,185,y,9,c,42);nxArc(185,y+r,r,9,0,180,c,42);nxLine(185,y+2*r,142,y+2*r,9,c,42);
 float curve=3.14159265f*r,left=std::max(0.0f,std::min(1.0f,p))*(203+curve);
 if(left>0)nxLine(25,y,25+std::min(160.0f,left),y,9,c);
 if(left>160)nxArc(185,y+r,r,9,0,std::min(180.0f,(left-160)*180/curve),c);
 if(left>160+curve)nxLine(185,y+2*r,185-std::min(43.0f,left-160-curve),y+2*r,9,c);
}
void sportRunner(int x,int y,uint16_t c){
 nxDisc(x+13,y+3,3,c);nxLine(x+10,y+9,x+5,y+17,3,c);nxLine(x+10,y+9,x+18,y+13,3,c);nxLine(x+10,y+9,x+5,y+7,3,c);nxLine(x+5,y+7,x+1,y+13,3,c);nxLine(x+5,y+17,x+11,y+22,3,c);nxLine(x+5,y+17,x-1,y+24,3,c);
}
void drawSportFace(){
 uiBackground();uiFontBg=themeBg;
 char battery[20],stepsText[24],distance[24],kcal[24],date[24];
 if(batPercent<0)snprintf(battery,sizeof(battery),"--%%");else snprintf(battery,sizeof(battery),"%d%%",batPercent);
 snprintf(stepsText,sizeof(stepsText),"%lu",(unsigned long)activityTodaySteps());
 if(activityStrideMm()>0)snprintf(distance,sizeof(distance),"~%.1f км",activityDistanceKm());else snprintf(distance,sizeof(distance),"км: --");
 if(personWeightDeciKg&&activityStrideMm()>0)snprintf(kcal,sizeof(kcal),"~%.0f",activityWalkingKcal());else snprintf(kcal,sizeof(kcal),"--");
 if(timeValid)snprintf(date,sizeof(date),"%02d/%02d",day,mon);else snprintf(date,sizeof(date),"--/--");
 float steps=stepGoal?std::min(1.0f,(float)activityTodaySteps()/stepGoal):0;
 float bat=batPercent<0?0:std::max(0.0f,std::min(1.0f,batPercent/100.0f));
 if(faceTop){uiText(battery,17,8,1,COL_ACCENT);if(unreadMessages)uiText("СМС",88,8,1,COL_ACCENT2);uiText(chronosConnected?"BT":"--",151,8,1,UI_MUTED);}
 if(faceDate)uiText(date,183,8,1,UI_MUTED);
 if(!timeValid){uiCenter("--:--",95,COL_TEXT,3);uiCenter("Установите время",161,UI_MUTED);faceFields();return;}
 int h=use24h?hh:(hh%12?hh%12:12);
 switch(faceVariant){
 case 0: { // NUMIX: asymmetric typography framed by open corners.
  facePair(h,22,36,48,83,COL_TEXT);facePair(mm,99,136,54,85,COL_ACCENT);
  nxLine(114,27,202,27,6,COL_ACCENT,45);nxArc(202,48,21,6,0,90,COL_ACCENT,45);nxLine(223,48,223,127,6,COL_ACCENT,45);
  float chargePath=bat*200;
  if(chargePath>0)nxLine(114,27,114+std::min(88.0f,chargePath),27,6,COL_ACCENT);
  if(chargePath>88)nxArc(202,48,21,6,0,std::min(90.0f,(chargePath-88)*90/33),COL_ACCENT);
  if(chargePath>121)nxLine(223,48,223,48+chargePath-121,6,COL_ACCENT);
  nxLine(14,143,14,204,6,COL_ACCENT2);nxArc(36,204,22,6,180,90,COL_ACCENT2);sportMeter(36,226,64,steps,COL_ACCENT2);
  uiFit(distance,26,153,72,UI_MUTED);break;}
 case 1: // TRACK: two U-shaped meters below bold stacked digits.
  facePair(h,20,27,53,70,COL_TEXT);facePair(mm,20,101,53,70,UI_MUTED);
  faceArc(190,62,25,-90,360,6,bat,COL_ACCENT);uiText(battery,175,55,1,COL_TEXT);
  nxDisc(190,133,25,themePanel);sportRunner(182,121,COL_ACCENT);
  sportLoop(180,steps,COL_ACCENT2,25);sportLoop(195,bat,COL_ACCENT,12);
  uiText(distance,24,218,1,UI_MUTED);break;
 case 2: // RINGS: slanted numerals and real battery / daily-goal gauges.
  facePair(h,17,28,48,87,COL_TEXT,3);facePair(mm,17,122,48,87,UI_MUTED,3);
  faceArc(190,59,23,-90,360,5,bat,COL_ACCENT);uiText(battery,174,52,1,COL_TEXT);
  faceArc(190,126,23,-90,360,6,steps,COL_ACCENT2);uiText("ЦЕЛЬ",172,106,1,UI_MUTED);
  char goal[16];snprintf(goal,sizeof(goal),"%d%%",(int)(steps*100));uiText(goal,175,128,1,COL_TEXT);
  faceArc(188,213,35,180,180,7,steps,COL_ACCENT2);faceArc(188,213,25,180,180,6,bat,COL_ACCENT);faceArc(188,213,15,180,180,5,steps,COL_TEXT);
  uiText(distance,21,217,1,COL_ACCENT);break;
 case 3: // LATTE: upper and lower curved bands frame the time.
  facePair(h,19,75,48,61,COL_TEXT);facePair(mm,19,143,48,70,COL_ACCENT2);
  faceArc(122,128,94,-90,66,9,steps,COL_ACCENT);faceArc(122,128,79,-90,66,8,bat,COL_ACCENT2);
  faceArc(122,128,100,28,62,9,steps,COL_ACCENT);faceArc(122,128,85,28,62,8,bat,COL_ACCENT2);
  uiText(stepsText,182,96,1,COL_ACCENT);uiText(battery,182,113,1,COL_ACCENT2);
  nxDisc(196,157,21,COL_ACCENT2);sportRunner(188,145,themeBg);break;
 case 4: // POSTER: oversized offset rows, large overlapping bounding boxes.
  facePair(h,18,27,68,109,COL_TEXT);facePair(mm,75,117,68,112,COL_ACCENT);
  uiText("ШАГИ",16,171,1,UI_MUTED);uiText(stepsText,16,188,1,COL_TEXT);uiFit(distance,16,211,64,UI_MUTED);break;
 case 5: // TERMINAL: compact telemetry with static scan lines (no animation).
  uiText("NEXUS",15,44,1,COL_ACCENT);uiNumber(h,103,36,23,39,COL_TEXT);uiNumber(mm,169,36,23,39,COL_TEXT);uiText(":",158,49,1,COL_ACCENT);
  nxRoundRect(13,91,214,83,4,COL_ACCENT,1);
  for(int j=0;j<17;++j)nxLine(24,104+j*3,24+32*bat,104+j*3,2,COL_ACCENT);
  uiText("БАТАРЕЯ",76,101,1,COL_ACCENT);uiText(battery,76,118,1,COL_TEXT);
  uiText("СВЯЗЬ",76,139,1,COL_ACCENT);uiText(chronosConnected?"OK":"НЕТ",157,139,1,COL_TEXT);
  uiText("ШАГИ",16,185,1,COL_ACCENT);uiText(stepsText,16,204,1,COL_TEXT);
  uiText("ЦЕЛЬ",94,185,1,COL_ACCENT);sportMeter(94,207,49,steps,COL_ACCENT);
  uiText("ККАЛ",168,185,1,COL_ACCENT);uiText(kcal,168,204,1,COL_TEXT);
  for(int y=33;y<230;y+=3)for(int x=12;x<228;++x)nxPx(x,y,themeBg,45);
  break;
 }
 // Shared status line stays separate from time and custom complications.
 if(doNotDisturb)uiText("DND",18,226,1,COL_ACCENT2);
 if(!use24h)uiText(hh<12?"AM":"PM",65,226,1,UI_MUTED);
 if(faceSeconds){char sec[8];snprintf(sec,sizeof(sec),"%02d",ss);uiText(sec,210,226,1,UI_MUTED);}
 faceFields();
}
