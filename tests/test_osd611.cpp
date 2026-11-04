#include <cassert>
#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
std::string sendOsd(const std::string& command){Serial.output.clear();for(char c:command+"\n")Serial.input.push_back(c);while(Serial.available())themeSerialService();return Serial.output;}
std::string osdCommand(const uint8_t* data){std::string result="NXOSD ";char part[3];for(int j=0;j<OSD_BYTES;++j){snprintf(part,sizeof(part),"%02x",data[j]);result+=part;}return result;}
bool sameOsd(const OsdLayout& a,const OsdLayout& b){uint8_t first[OSD_BYTES],second[OSD_BYTES];osdEncode(a,first);osdEncode(b,second);return memcmp(first,second,sizeof(first))==0;}
int main(){
 setup();assert(osdValid(osd));const OsdLayout initial=osd;assert(sendOsd("NXHELLO").find("OSD=1")!=std::string::npos);
 OsdLayout next=osd;next.enabled=true;next.items[0]={34,216,2,1,1};next.items[5]={0,0,2,1,3};uint8_t data[OSD_BYTES];osdEncode(next,data);
 assert(sendOsd(osdCommand(data)).find("OK OSD")!=std::string::npos&&sameOsd(osd,next));osd=initial;osdLoad();assert(sameOsd(osd,next));
 assert(sendOsd("NXGETCONFIG").find("\"osd\":{\"version\":1,\"enabled\":1")!=std::string::npos);
 // Bad checksum, wrong length, nonhex, out-of-bounds coordinates and versions never apply.
 data[8]^=1;assert(sendOsd(osdCommand(data)).find("ERR OSD_RANGE_OR_CRC")!=std::string::npos&&sameOsd(osd,next));
 assert(sendOsd("NXOSD 00").find("ERR OSD_LENGTH")!=std::string::npos);osdEncode(next,data);auto badHex=osdCommand(data);badHex[20]='z';assert(sendOsd(badHex).find("ERR OSD_HEX")!=std::string::npos);
 OsdLayout outside=next;outside.items[0].x=35;osdEncode(outside,data);assert(sendOsd(osdCommand(data)).find("ERR OSD_RANGE_OR_CRC")!=std::string::npos&&sameOsd(osd,next));
 outside=next;outside.items[11].size=2;osdEncode(outside,data);assert(sendOsd(osdCommand(data)).find("ERR OSD_RANGE_OR_CRC")!=std::string::npos);
 osdEncode(initial,data);prefs.failWrites=true;assert(sendOsd(osdCommand(data)).find("ERR SAVE")!=std::string::npos&&sameOsd(osd,next));prefs.failWrites=false;osd=initial;osdLoad();assert(sameOsd(osd,next));
 // A corrupt persisted layout is ignored; screen state remains a valid known layout.
 osdEncode(next,data);data[OSD_BYTES-1]^=1;prefs.putBytes("osd",data,sizeof(data));osd=initial;osdLoad();assert(sameOsd(osd,initial));
 // The actual raster renderer must keep every individual item within its declared rectangle,
 // including both sizes, the bottom/right boundaries and long dynamic strings.
 wallpaperEnabled=false;themeIndex=0;applyTheme();timeValid=true;hh=23;mm=59;ss=58;use24h=false;faceSeconds=true;batPercent=100;day=29;mon=10;
 for(int id=0;id<OSD_COUNT;++id)for(int size=1;size<=2;++size){if(id==11&&size==2)continue;osd=initial;osd.enabled=true;for(auto& item:osd.items)item.on=0;int w=osdWidth(id,size),h=osdHeight(id,size);osd.items[id]={(uint16_t)(240-w),(uint16_t)(280-h),(uint8_t)size,1,0};assert(osdValid(osd));currentScreen=SC_CLOCK;uiBackground();auto clean=gfx->framebuffer;drawClockScreen();int colored=0;for(int y=0;y<280;++y)for(int x=0;x<240;++x)if(gfx->framebuffer[y*240+x]!=clean[y*240+x]){++colored;assert(x>=240-w&&y>=280-h);}assert(colored>0);}
 osd=initial;osd.enabled=true;for(auto& item:osd.items)item.on=0;assert(!osdNeedsTick());osd.items[1].on=1;assert(osdNeedsTick());faceSeconds=false;assert(!osdNeedsTick());osd.items[6].on=1;timerRunning=true;assert(osdNeedsTick());timerRunning=false;osd.items[10].on=1;specialRunning=true;assert(osdNeedsTick());
 osd=initial;osd.enabled=false;assert(!osdNeedsTick());faceSeconds=true;assert(osdNeedsTick());
 puts("PASS OSD: atomic layout write/readback, NVS reload/failure/corruption, validation/CRC, all 13 item raster bounds and sizes, dynamic refresh gating");
}
