#pragma once
// Seven useful tools; no simulated sensor measurements. Active sessions stay in RAM.
const char* SPECIAL_NAMES[7]={"На каждый день","Фокус","Интервалы","Путешествие","Ритм","Полевой журнал","Техник"};
const char* SPECIAL_SUBTITLES[7]={"Секундомер и круг","25 минут / 5 минут","40 / 20 сек, 8 кругов","Второе местное время","Вдох 4 / выдох 6 сек","Три метки времени","Связь, память, питание"};
uint8_t specialization=0;int16_t travelOffset=120;
int specialCursor=0;bool specialRunning=false;
uint32_t specialElapsed=0,specialLast=0,specialLap=0;
char fieldMarks[3][18]={};uint8_t fieldCount=0;
void loadSpecialization() {
 if(!prefsOK) return;
 uint8_t p[4]={};
 if(prefs.getBytesLength("special")==4 && prefs.getBytes("special",p,4)==4 && p[0]==1 && p[1]<7) {
  int16_t offset=(int16_t)((uint16_t)p[2]|((uint16_t)p[3]<<8));
  if(offset>=-720 && offset<=840 && offset%30==0) {specialization=p[1];travelOffset=offset;}
 }
 uint8_t notes[55]={};
 if(prefs.getBytesLength("marks")==sizeof(notes) && prefs.getBytes("marks",notes,sizeof(notes))==sizeof(notes) && notes[0]<=3) {
  bool valid=true;for(int i=0;i<3;++i) if(notes[1+i*18+17]!=0) valid=false;
  if(valid) {fieldCount=notes[0];memcpy(fieldMarks,notes+1,sizeof(fieldMarks));}
 }
}
bool saveSpecialization() {
 if(!prefsOK) return false;
 uint8_t p[4]={1,specialization,(uint8_t)travelOffset,(uint8_t)((uint16_t)travelOffset>>8)};
 return prefs.putBytes("special",p,4)==4;
}
void specialReset() {specialRunning=false;specialElapsed=0;specialLap=0;specialLast=millis();}
void selectSpecialization(int dir) {specialReset();specialization=(specialization+7+dir)%7;markSettingsDirty();}
void specialService() {
 uint32_t now=millis(),elapsed=now-specialLast;specialLast=now;
 if(!specialRunning || !elapsed) return;
 uint32_t before=specialElapsed;
 if(specialization==1) {
  specialElapsed=(uint32_t)(((uint64_t)specialElapsed+elapsed)%1800000u);
  if((before<1500000)!=(specialElapsed<1500000) || elapsed>=1800000) {beep(1500,180);wakeDisplay();}
 } else if(specialization==2) {
  specialElapsed=(uint32_t)std::min<uint64_t>((uint64_t)specialElapsed+elapsed,480000);
  if(specialElapsed>=480000) {specialRunning=false;beep(1700,250);wakeDisplay();}
  else if(before/60000!=specialElapsed/60000 || (before%60000<40000)!=(specialElapsed%60000<40000)) {beep(1300,100);wakeDisplay();}
 } else if(specialization==4) specialElapsed=(specialElapsed+elapsed)%10000;
 else specialElapsed=(uint32_t)std::min<uint64_t>((uint64_t)specialElapsed+elapsed,359999000u);
}
void saveFieldMarks() {
 if(!prefsOK) return;
 uint8_t notes[55]={};notes[0]=fieldCount;memcpy(notes+1,fieldMarks,sizeof(fieldMarks));
 if(prefs.putBytes("marks",notes,sizeof(notes))!=sizeof(notes)) Serial.println("Field marks save failed");
}
void specialAction(int dir) {
 specialService();
 if(specialization==3) {travelOffset=(int16_t)(((travelOffset+720)/30+53+dir)%53*30-720);markSettingsDirty();}
 else if(specialization==5) {
  if(dir<0) {memset(fieldMarks,0,sizeof(fieldMarks));fieldCount=0;}
  else {
   for(int i=2;i>0;--i) memcpy(fieldMarks[i],fieldMarks[i-1],18);
   if(timeValid) snprintf(fieldMarks[0],18,"%02d:%02d %02d.%02d",hh,mm,day,mon);
   else snprintf(fieldMarks[0],18,"UP %lu sec",(unsigned long)(millis()/1000));
   if(fieldCount<3) ++fieldCount;
  }
  saveFieldMarks();
 } else if(specialization==6) {if(dir>0) beep(1600,120);uiDirty=true;}
 else if(dir<0) {if(specialization==0 && specialRunning) specialLap=specialElapsed;else specialReset();}
 else {if(specialization==2 && specialElapsed>=480000) specialElapsed=0;specialRunning=!specialRunning;specialLast=millis();}
 uiDirty=true;
}

bool setSpecialFromUsb(int index) {
 if(index<0 || index>6) return false;
 if(specialization!=index) {specialReset();}
 specialization=index;markSettingsDirty();uiDirty=true;return saveSettings();
}
