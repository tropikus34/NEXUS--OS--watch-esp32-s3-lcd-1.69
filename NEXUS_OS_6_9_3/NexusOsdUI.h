#pragma once
// Free-layout rendering is restricted to the clock face. Alerts/settings stay operable.
bool osdNeedsTick(){return osd.enabled?((osd.items[1].on&&faceSeconds)||(osd.items[6].on&&timerRunning)||(osd.items[10].on&&specialRunning)):faceSeconds;}
void osdText(const char* text,const OsdItem& item,int width,uint16_t color){
 int used=0;while(*text){uint32_t cp=nextCodepoint(text);int advance=premiumAdvance(cp)*item.size;if(used+std::max(advance,16*(int)item.size)>width)break;premiumDraw(cp,item.x+used,item.y,item.size,color,themeBg);used+=advance;}
}
void drawOsdClock(){
 for(int id=0;id<OSD_COUNT;++id){const auto& item=osd.items[id];if(!item.on||(id==1&&!faceSeconds)||(id==12&&use24h))continue;
  uint16_t colors[]={COL_TEXT,COL_ACCENT,UI_MUTED,COL_ACCENT2};uint16_t color=colors[item.color];char text[100]={};
  if(id==0){
   if(!timeValid){osdText("--:--",item,osdWidth(id,item.size),color);continue;}
   int hour=use24h?hh:(hh%12?hh%12:12),w=item.size==1?30:46,h=osdHeight(id,item.size);uiFontBg=themeBg;
   uiNumber(hour,item.x,item.y,w,h,color);uiNumber(mm,item.x+2*w+16,item.y,w,h,color);
   if(!faceSeconds||!colonBlink||colonOn){gfx->fillCircle(item.x+2*w+10,item.y+h/3,2,color);gfx->fillCircle(item.x+2*w+10,item.y+h*2/3,2,color);}continue;
  }
  if(id==1)snprintf(text,sizeof(text),timeValid?"%02d":"--",ss);
  else if(id==2){if(!timeValid)snprintf(text,sizeof(text),"-- / --");else snprintf(text,sizeof(text),"%02d / %02d",day,mon);}
  else if(id==3){if(batPercent<0)snprintf(text,sizeof(text),"АКБ --%%");else snprintf(text,sizeof(text),"АКБ ~%d%%",batPercent);}
  else if(id==4)snprintf(text,sizeof(text),"%s",ownerName);
  else if(id==5)snprintf(text,sizeof(text),"Ш %lu",(unsigned long)activityTodaySteps());
  else if(id==6){uint32_t seconds=(timerRemainingMs+999)/1000;snprintf(text,sizeof(text),"Т %02lu:%02lu",(unsigned long)(seconds/60),(unsigned long)(seconds%60));}
  else if(id==7){int a=uiNextAlarm();if(a<0)snprintf(text,sizeof(text),"Б --:--");else snprintf(text,sizeof(text),"Б %02u:%02u",alarmHours[a],alarmMinutes[a]);}
  else if(id==8)snprintf(text,sizeof(text),"СМС %d",unreadMessages);
  else if(id==9)snprintf(text,sizeof(text),chronosConnected?"BT вкл":"BT выкл");
  else if(id==10)specialSummary(text,sizeof(text));
  else if(id==11)snprintf(text,sizeof(text),"Удерж: инструмент");
  else if(id==12&&timeValid)snprintf(text,sizeof(text),hh<12?"AM":"PM");
  osdText(text,item,osdWidth(id,item.size),color);
 }
}
