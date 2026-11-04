#pragma once
#include "NexusBold.h"
#include "NexusClassic.h"
int faceDesign(){const uint8_t map[8]={11,12,13,14,15,16,1,10};return map[faceVariant<=7?faceVariant:0];}
// Draw glyph masks directly into the existing frame, preserving uploaded photos.
int faceAlpha(int d,int x,int y,int w,int h){if(x<0||y<0||x>=w||y>=h)return 0;int sx=x*48/w,sy=y*72/h;return (FACE_BOLD[d][sy*12+sx/4]>>(6-2*(sx%4)))&3;}
float faceAlphaF(int d,float fx,float fy){int sx=floorf(fx),sy=floorf(fy);float tx=fx-sx,ty=fy-sy;auto A=[&](int X,int Y){if(X<0||Y<0||X>=48||Y>=72)return 0;return (FACE_BOLD[d][Y*12+X/4]>>(6-2*(X%4)))&3;};
 return ((A(sx,sy)*(1-tx)+A(sx+1,sy)*tx)*(1-ty)+(A(sx,sy+1)*(1-tx)+A(sx+1,sy+1)*tx)*ty)/3.0f;}
void faceDigit(int d,int x,int y,int w,int h,uint16_t ink,int effect=0){
 displayPending=true;
 for(int j=0;j<h;++j)for(int i=0;i<w;++i){float fx=(i+0.5f)*48.0f/w-0.5f,fy=(j+0.5f)*72.0f/h-0.5f;float a=faceAlphaF(d,fx,fy);
  int v=(int)((a-0.5f)*2.2f*255+128);if(v<=0)continue;if(v>255)v=255;
  if(effect==1){float sx=2.0f*48/w,sy=2.0f*72/h;float in=std::min(std::min(faceAlphaF(d,fx-sx,fy),faceAlphaF(d,fx+sx,fy)),std::min(faceAlphaF(d,fx,fy-sy),faceAlphaF(d,fx,fy+sy)));int iv=(int)((in-0.5f)*2.2f*255+128);if(iv>0)v=std::max(0,v-std::min(255,iv));if(!v)continue;}
  if(effect==2){int m=j%6;if(m>3)continue;if(m==3)v/=3;}
  nxPx(x+i+(effect==3?(h-j)/6:0),y+j,ink,v);
 }
}
void facePair(int value,int x,int y,int w,int h,uint16_t ink,int effect=0){faceDigit(value/10,x,y,w,h,ink,effect);faceDigit(value%10,x+w+5,y,w,h,ink,effect);}
void faceArc(int cx,int cy,int r,int start,int span,int thick,float progress,uint16_t ink){
 progress=std::max(0.0f,std::min(1.0f,progress));
 // Legacy angles are screen-space (0 = 3 o'clock); convert to clock degrees.
 float s=start+90;nxArc(cx,cy,r,thick,s,span,ink,46);nxArc(cx,cy,r,thick,s,span*progress,ink);
}
void faceFields(){
 char label[32],value[40];
 for(int i=0;i<2;++i){uint8_t id=i?faceCompB:faceCompA;if(id==6)continue;
  if(id)uiComplication(id,label,sizeof(label),value,sizeof(value));
  else if(!i){const char* names[]={"Шаги","Фокус","Интервалы","Второе время","Ритм","Метки","Система"};snprintf(label,sizeof(label),"%s",names[specialization]);specialSummary(value,sizeof(value));if(specialization==0)snprintf(value,sizeof(value),"%lu",(unsigned long)activityTodaySteps());}
  else {if(timerRunning)uiComplication(2,label,sizeof(label),value,sizeof(value));else if(unreadMessages)uiComplication(4,label,sizeof(label),value,sizeof(value));else uiComplication(3,label,sizeof(label),value,sizeof(value));}
  int x=16+i*112,y=240;
  if(faceDesign()==0)y=210;
  if(faceDesign()==3){uiCard(x-4,211,104,48);y=213;}
  if(faceDesign()==4)y=234;
  if(faceDesign()==5){x=16+i*112;y=237;}
  if(faceDesign()==6){nxRoundRect(x-3,215,106,46,10,COL_ACCENT,1.2f);y=217;}
  uiFontBg=faceDesign()==3?themePanel:themeBg;uiFit(label,x,y,100,UI_MUTED,1);uiFit(value,x,y+17,100,COL_TEXT,1);
 }
}
void faceHand(float angle,int length,uint16_t ink,int width){nxHand(120,145,angle,length,length*0.16f,width+0.5f,ink);}
void faceSegmentDigit(int d,int x,int y,int w,int h,uint16_t ink){
 static const uint8_t masks[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};int t=5,mid=h/2;
 int boxes[7][4]={{t,0,w-2*t,t},{w-t,t,t,mid-t},{w-t,mid+t,t,h-mid-2*t},{t,h-t,w-2*t,t},{0,mid+t,t,h-mid-2*t},{0,t,t,mid-t},{t,mid,w-2*t,t}};
 for(int i=0;i<7;++i){auto &b=boxes[i];nxRoundRect(x+b[0],y+b[1],b[2],b[3],2.5f,masks[d]&(1<<i)?ink:colorMix(ink,themeBg,1,14));}
}
void faceSegmentPair(int v,int x,int y,int w,int h,uint16_t ink){faceSegmentDigit(v/10,x,y,w,h,ink);faceSegmentDigit(v%10,x+w+6,y,w,h,ink);}
#include "NexusSport.h"
void drawReferenceFace(){
 if(faceVariant<6){drawSportFace();return;}
 uiBackground();uiFontBg=themeBg;
 if(faceTop&&(faceDesign()==2||faceDesign()==3))drawTopBar();
 else if(faceTop) {uiText(faceDesign()==0?ownerName:WATCH_LOOKS[themeIndex].name,16,15,1,UI_MUTED);char b[16];if(batPercent<0)snprintf(b,sizeof(b),"--%%");else snprintf(b,sizeof(b),"~%d%%",batPercent);uiText(b,177,15,1,UI_MUTED);if(unreadMessages)gfx->fillCircle(161,24,3,COL_ACCENT);}

 if(doNotDisturb)uiCenter("НЕ БЕСПОКОИТЬ",42,COL_ACCENT);else if(faceDate)drawDate();
 if(!use24h&&timeValid)uiText(hh<12?"AM":"PM",198,42,1,UI_MUTED);
 if(!timeValid){uiCenter("--:--",112,COL_TEXT,3);uiCenter("Установите время",165,UI_MUTED);faceFields();return;}
 int h=use24h?hh:(hh%12?hh%12:12);float steps=stepGoal?std::min(1.0f,(float)activityTodaySteps()/stepGoal):0;
 switch(faceDesign()){
 case 10:legacyTime(hh,mm,ss);break;
 case 0: // ONYX: restrained horizontal typography and generous negative space.
  uiNumber(h,23,108,39,62,COL_TEXT);uiNumber(mm,132,108,39,62,COL_TEXT);
  nxDisc(120,129,2.6f,COL_ACCENT);nxDisc(120,151,2.6f,COL_ACCENT);
  nxLine(106,187,134,187,2.5f,COL_ACCENT);
  if(faceSeconds){char s[8];snprintf(s,sizeof(s),"%02d",ss);uiCenter(s,254,UI_MUTED);}break;
 case 1: // AURUM: square dress-watch chapter ring and Roman cardinal marks.
  nxRoundRect(28,67,184,158,26,COL_ACCENT,1.6f);nxRoundRect(33,72,174,148,22,COL_ACCENT,1.0f,90);
  uiText("XII",109,77,1,COL_ACCENT);uiText("III",181,135,1,COL_ACCENT);uiText("VI",112,198,1,COL_ACCENT);uiText("IX",41,135,1,COL_ACCENT);
  for(int i=0;i<12;++i){if(i%3==0)continue;float a=i*0.523598776f;nxDisc(120+sinf(a)*77,145-cosf(a)*62,1.6f,COL_ACCENT);}
  faceHand((hh%12+mm/60.0f)*30,39,COL_TEXT,5);faceHand((mm+(faceSeconds?ss/60.0f:0))*6,58,COL_ACCENT,3);if(faceSeconds)faceHand(ss*6,62,UI_MUTED,1);nxDisc(120,145,4.5f,COL_ACCENT);nxDisc(120,145,1.6f,themeBg);break;

 case 2: // GLACIER / broad C-shaped battery and activity meters.
  facePair(h,16,75,51,65,COL_TEXT);facePair(mm,16,151,51,65,COL_ACCENT);
  faceArc(125,145,91,-68,136,10,steps,COL_ACCENT);
  faceArc(125,145,74,-65,130,8,batPercent<0?0:batPercent/100.0f,COL_ACCENT2);
  uiText("ШАГ",162,132,1,COL_ACCENT);break;
 case 3: // EMBER: technical dashboard with segmented time.
  for(int x=18;x<224;x+=16)for(int y=76;y<199;y+=16)gfx->fillRect(x,y,1,1,themePanel);
  uiText("CHRONO",17,73,1,COL_ACCENT);uiText("NEXUS",163,73,1,UI_MUTED);
  faceSegmentPair(h,18,104,40,67,COL_TEXT);faceSegmentPair(mm,132,104,40,67,COL_ACCENT);
  nxDisc(118.5f,126.5f,2.8f,COL_ACCENT);nxDisc(118.5f,149.5f,2.8f,COL_ACCENT);
  for(int i=0;i<30;++i)nxRoundRect(18+i*7,185,4,i%5==0?10:5,2,faceSeconds&&i<=ss/2?COL_ACCENT:themePanel);
  break;
 case 7: // ONYX / poster: diagonal, oversized, clearly separated hour and minute.
  facePair(h,18,67,67,76,COL_TEXT);facePair(mm,79,149,67,76,COL_ACCENT);
  if(faceSeconds){char s[8];snprintf(s,sizeof(s),"%02d",ss);uiText(s,24,196,1,UI_MUTED);}break;
 case 8: // NEBULA / outlined editorial numerals and ruled accents.
  nxRoundRect(16,73,5,145,2.5f,COL_ACCENT2);nxRoundRect(27,73,3,145,1.5f,COL_ACCENT);
  facePair(h,47,70,69,72,COL_TEXT,1);facePair(mm,72,151,69,72,COL_ACCENT,1);
  nxRoundRect(47,146,161,3,1.5f,COL_ACCENT2);break;
 case 9: // AURUM / striped numeral sculpture and contrasting hands.
  facePair(12,27,70,90,151,colorMix(COL_ACCENT,themeBg,3,5),2);
  for(int i=0;i<12;++i){float a=i*0.523598776f;nxDisc(120+sinf(a)*91,145-cosf(a)*78,i%3?1.5f:2.8f,COL_ACCENT);}
  faceHand((hh%12+mm/60.0f)*30,44,COL_TEXT,5);faceHand((mm+(faceSeconds?ss/60.0f:0))*6,69,COL_TEXT,3);if(faceSeconds)faceHand(ss*6,74,COL_ACCENT,1);nxDisc(120,145,5.5f,COL_ACCENT);nxDisc(120,145,2,themeBg);break;

 }
 faceFields();
}
