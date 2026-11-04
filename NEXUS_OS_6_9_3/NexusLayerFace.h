#pragma once
// NEXUS LayerFace v1. Portable bounded 240x280 renderer; built-in faces remain untouched.
// Binary format: "NXLF", version 1, layer count, base face, flags, background RGB565,
// followed by up to 16 packed 24-byte layers. File is committed only after CRC check.
static const int NXLF_MAX=16, NXLF_REC=24, NXLF_HEAD=10, NXLF_LIMIT=NXLF_HEAD+NXLF_MAX*NXLF_REC;
static uint8_t nxlfData[NXLF_LIMIT]={};
static uint16_t nxlfSize=0;
static bool nxlfLoaded=false, nxlfActive=false;
static uint16_t nxlfPendingSize=0;
static uint32_t nxlfPendingCrc=0;
static uint16_t nxlfPendingOffset=0;
static uint32_t nxlfCrc(const uint8_t* d,size_t n){uint32_t c=0xffffffffu;for(size_t i=0;i<n;i++){c^=d[i];for(int b=0;b<8;b++)c=(c>>1)^((c&1)?0xedb88320u:0);}return c^0xffffffffu;}
static uint16_t nxlf16(const uint8_t* p){return (uint16_t)p[0]|((uint16_t)p[1]<<8);}
static bool nxlfValid(const uint8_t* d,size_t n){
 if(n<NXLF_HEAD||memcmp(d,"NXLF",4)||d[4]!=1||d[5]>NXLF_MAX||n!=NXLF_HEAD+d[5]*NXLF_REC)return false;
 for(unsigned i=0;i<d[5];i++){const uint8_t* r=d+NXLF_HEAD+i*NXLF_REC;
  if(r[0]>7||r[1]>13||r[2]>239||r[4]==0||r[5]==0||r[8]>1||r[9]>12)return false;
  if((unsigned)r[2]+r[4]>240||(unsigned)r[3]+r[5]>280)return false;
 }
 return true;
}
static void nxlfLoad(){
 if(nxlfLoaded||!themeFsReady)return;
 nxlfLoaded=true;File file=LittleFS.open("/layerface.nxlf","r");
 if(!file && LittleFS.exists("/layerface.bak")){LittleFS.rename("/layerface.bak","/layerface.nxlf");file=LittleFS.open("/layerface.nxlf","r");}
 if(!file)return;
 size_t n=file.size();if(n>NXLF_LIMIT||n<NXLF_HEAD){file.close();return;}
 size_t got=file.read(nxlfData,n);file.close();
 if(got==n&&nxlfValid(nxlfData,n)){nxlfSize=n;nxlfActive=true;}
}
static void nxlfSource(uint8_t id,const char* literal,char* out,size_t cap){
 switch(id){
 case 0:snprintf(out,cap,"%s",literal);break;
 case 1:snprintf(out,cap,"%02d:%02d",use24h?hh:(hh%12?hh%12:12),mm);break;
 case 2:snprintf(out,cap,"%02d",use24h?hh:(hh%12?hh%12:12));break;
 case 3:snprintf(out,cap,"%02d",mm);break;
 case 4:snprintf(out,cap,"%02d",ss);break;
 case 5:snprintf(out,cap,"%02d/%02d",day,mon);break;
 case 6:if(batPercent<0)snprintf(out,cap,"--%%");else snprintf(out,cap,"%d%%",batPercent);break;
 case 7:snprintf(out,cap,"%lu",(unsigned long)activityTodaySteps());break;
 case 8:snprintf(out,cap,"~%lu kcal",(unsigned long)(activityTodaySteps()*4UL/100UL));break; // estimate, not a sensor reading
 case 9:snprintf(out,cap,"BT %s",chronosConnected?"ON":"OFF");break;
 case 10:snprintf(out,cap,"MSG %u",(unsigned)unreadMessages);break;
 case 11:snprintf(out,cap,"%s",ownerName);break;
 case 12:snprintf(out,cap,"%02lu:%02lu",(unsigned long)(timerRemainingMs/60000UL),(unsigned long)(timerRemainingMs/1000UL%60));break;
 case 13:snprintf(out,cap,"%lu",(unsigned long)stepGoal);break;
 default:out[0]=0;break;
 }
}
static void nxlfDraw(){
 nxlfLoad();if(!nxlfActive)return;
 // Reuse existing wallpaper and the native palette. Custom background overrides only when requested.
 uiBackground();uiFontBg=themeBg;
 const uint8_t* d=nxlfData;
 if(d[7]&1){uint16_t bg=nxlf16(d+8);gfx->fillScreen(bg);uiFontBg=bg;}
 for(int i=0;i<d[5];i++){
  const uint8_t* r=d+NXLF_HEAD+i*NXLF_REC;
  if(!r[8])continue;
  int x=r[2],y=r[3],w=r[4],h=r[5];uint16_t ink=nxlf16(r+6);int thick=r[9];
  char literal[13]={};memcpy(literal,r+12,12);literal[12]=0;char value[48]={};nxlfSource(r[1],literal,value,sizeof(value));
  switch(r[0]){
   case 0: { // scalable UTF-8 text (scale 1..2, safe fit)
    uiFit(value,x,y,w,ink,thick>=2?2:1);break;}
   case 1: { // progress arc around a centre
    float pct=r[1]==6?(batPercent<0?0:batPercent/100.f):r[1]==7?(stepGoal?fminf(1.f,activityTodaySteps()/(float)stepGoal):0.f):r[1]==4?ss/60.f:0.75f;
    nxArc(x+w/2,y+h/2,std::min(w,h)/2-2,std::max(1,thick),r[10],r[11]?r[11]*3:270,ink,45);
    nxArc(x+w/2,y+h/2,std::min(w,h)/2-2,std::max(1,thick),r[10],(r[11]?r[11]*3:270)*pct,ink);break;}
   case 2:{float pct=r[1]==6?(batPercent<0?0:batPercent/100.f):r[1]==7?(stepGoal?fminf(1.f,activityTodaySteps()/(float)stepGoal):0.f):0.65f;
    nxRoundRect(x,y,w,h,std::min(8,h/2),ink,1,55);nxRoundRect(x,y,std::max(1,(int)(w*pct)),h,std::min(8,h/2),ink);break;}
   case 3:nxRoundRect(x,y,w,h,std::min(12,(int)r[10]),ink);break;
   case 4:nxRoundRect(x,y,w,h,std::min(12,(int)r[10]),ink,std::max(1,thick));break;
   case 5:nxDisc(x+w/2,y+h/2,std::min(w,h)/2,ink);break;
   case 6:uiIcon(r[10]%9,x,y,ink);break;
   case 7:{ // clock hand based on realtime minutes/hours/seconds
    float deg=r[1]==2?(hh%12+mm/60.f)*30.f:r[1]==4?ss*6.f:(mm+ss/60.f)*6.f;
    float a=deg*0.01745329252f;nxLine(x+w/2,y+h/2,x+w/2+sinf(a)*(w/2-2),y+h/2-cosf(a)*(h/2-2),std::max(1,thick),ink);break;}
  }
 }
}
static bool nxlfCommand(const char* cmd){
 if(!strcmp(cmd,"NXLFINFO")){nxlfLoad();Serial.printf("LFINFO VERSION=1 ACTIVE=%d COUNT=%u FS=%d MAX=%u\n",nxlfActive,nxlfActive?nxlfData[5]:0,themeFsReady,(unsigned)NXLF_MAX);return true;}
 if(!strcmp(cmd,"NXLFDELETE")){if(!themeFsReady){Serial.println("ERR NO_FILESYSTEM");return true;}LittleFS.remove("/layerface.part");LittleFS.remove("/layerface.bak");bool ok=!LittleFS.exists("/layerface.nxlf")||LittleFS.remove("/layerface.nxlf");if(ok){nxlfLoaded=true;nxlfActive=false;nxlfSize=0;uiDirty=true;}Serial.println(ok?"OK LFDELETE":"ERR LFDELETE");return true;}
 unsigned n=0;unsigned long crc=0;char extra=0;
 if(sscanf(cmd,"NXLFBEGIN %u %lx %c",&n,&crc,&extra)==2){
  if(!themeFsReady||n<NXLF_HEAD||n>NXLF_LIMIT||n!=NXLF_HEAD+((n-NXLF_HEAD)/NXLF_REC)*NXLF_REC){Serial.println("ERR LF_SIZE_OR_FS");return true;}
  LittleFS.remove("/layerface.part");File f=LittleFS.open("/layerface.part","w");if(!f){Serial.println("ERR LF_OPEN");return true;}f.close();nxlfPendingSize=n;nxlfPendingCrc=(uint32_t)crc;nxlfPendingOffset=0;Serial.println("OK LFREADY");return true;
 }
 if(!strncmp(cmd,"NXLFCHUNK ",10)){
  unsigned off=0;char hex[145]={};if(sscanf(cmd+10,"%u %144s",&off,hex)!=2||!nxlfPendingSize||off!=nxlfPendingOffset){Serial.println("ERR LF_OFFSET");return true;}
  size_t len=strlen(hex);if(len==0||len%2||len>128||off+len/2>nxlfPendingSize){Serial.println("ERR LF_CHUNK");return true;}
  uint8_t buf[64];for(size_t j=0;j<len;j+=2){int hi=hex[j]>='0'&&hex[j]<='9'?hex[j]-'0':hex[j]>='a'&&hex[j]<='f'?hex[j]-'a'+10:hex[j]>='A'&&hex[j]<='F'?hex[j]-'A'+10:-1;
   int lo=hex[j+1]>='0'&&hex[j+1]<='9'?hex[j+1]-'0':hex[j+1]>='a'&&hex[j+1]<='f'?hex[j+1]-'a'+10:hex[j+1]>='A'&&hex[j+1]<='F'?hex[j+1]-'A'+10:-1;
   if(hi<0||lo<0){Serial.println("ERR LF_HEX");return true;}buf[j/2]=(hi<<4)|lo;}
  File f=LittleFS.open("/layerface.part","a");if(!f||f.write(buf,len/2)!=len/2){f.close();Serial.println("ERR LF_WRITE");return true;}f.close();nxlfPendingOffset+=len/2;Serial.println("OK LFCHUNK");return true;
 }
 if(!strcmp(cmd,"NXLFEND")){
  if(!nxlfPendingSize||nxlfPendingOffset!=nxlfPendingSize){Serial.println("ERR LF_INCOMPLETE");return true;}
  uint8_t temp[NXLF_LIMIT];File f=LittleFS.open("/layerface.part","r");size_t n=f?f.read(temp,nxlfPendingSize):0;f.close();
  if(n!=nxlfPendingSize||!nxlfValid(temp,n)||nxlfCrc(temp,n)!=nxlfPendingCrc){LittleFS.remove("/layerface.part");nxlfPendingSize=0;Serial.println("ERR LF_CRC_OR_DATA");return true;}
  // Leave the working original in place if the staged file is invalid.
  LittleFS.remove("/layerface.bak");bool old=LittleFS.exists("/layerface.nxlf");
  if(old&&!LittleFS.rename("/layerface.nxlf","/layerface.bak")){Serial.println("ERR LF_BACKUP");return true;}
  bool ok=LittleFS.rename("/layerface.part","/layerface.nxlf");
  if(!ok&&old)LittleFS.rename("/layerface.bak","/layerface.nxlf");nxlfPendingSize=0;
  if(ok){memcpy(nxlfData,temp,n);nxlfSize=n;nxlfActive=true;nxlfLoaded=true;uiDirty=true;}Serial.println(ok?"OK LFSTORED":"ERR LF_COMMIT");return true;
 }
 return false;
}
