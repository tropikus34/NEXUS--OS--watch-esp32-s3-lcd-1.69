#pragma once
// A single bounded, versioned blob makes layout writes atomic at the NVS-key level.
constexpr int OSD_COUNT=13,OSD_BYTES=3+OSD_COUNT*6+4;
struct OsdItem {uint16_t x,y;uint8_t size,on,color;};
struct OsdLayout {bool enabled;OsdItem items[OSD_COUNT];};
OsdLayout osd={false,{{49,88,1,1,0},{172,144,1,1,1},{12,47,1,1,2},{132,12,1,1,0},{12,12,1,1,2},{12,177,1,1,0},{132,177,1,0,1},{132,177,1,1,0},{132,218,1,1,1},{12,218,1,1,2},{12,177,1,0,1},{12,254,1,1,2},{180,68,1,0,2}}};
int osdWidth(int id,int size){return id==0?(size==1?142:206):((id==1||id==12)?36*size:(id==11?216:108*size));}
int osdHeight(int id,int size){return id==0?(size==1?44:64):18*size;}
uint32_t osdCrc(const uint8_t* data,size_t n){uint32_t crc=0xffffffff;for(size_t j=0;j<n;++j){crc^=data[j];for(int i=0;i<8;++i)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);}return crc^0xffffffff;}
bool osdValid(const OsdLayout& value){
 for(int id=0;id<OSD_COUNT;++id){const OsdItem& item=value.items[id];if(item.size<1||item.size>2||(id==11&&item.size!=1)||item.on>1||item.color>3||item.x+osdWidth(id,item.size)>240||item.y+osdHeight(id,item.size)>280)return false;}
 return true;
}
void osdEncode(const OsdLayout& value,uint8_t* data){data[0]=1;data[1]=value.enabled;data[2]=OSD_COUNT;for(int id=0;id<OSD_COUNT;++id){const auto& item=value.items[id];int p=3+id*6;data[p]=item.x;data[p+1]=item.y&255;data[p+2]=item.y>>8;data[p+3]=item.size;data[p+4]=item.on;data[p+5]=item.color;}uint32_t crc=osdCrc(data,OSD_BYTES-4);for(int i=0;i<4;++i)data[OSD_BYTES-4+i]=(crc>>(i*8))&255;}
bool osdDecode(const uint8_t* data,OsdLayout& value){
 if(data[0]!=1||data[1]>1||data[2]!=OSD_COUNT)return false;
 uint32_t crc=0;for(int i=0;i<4;++i)crc|=(uint32_t)data[OSD_BYTES-4+i]<<(i*8);if(crc!=osdCrc(data,OSD_BYTES-4))return false;
 OsdLayout next={};next.enabled=data[1];for(int id=0;id<OSD_COUNT;++id){int p=3+id*6;next.items[id]={data[p],(uint16_t)(data[p+1]|((uint16_t)data[p+2]<<8)),data[p+3],data[p+4],data[p+5]};}if(!osdValid(next))return false;value=next;return true;
}
void osdLoad(){uint8_t data[OSD_BYTES];if(prefsOK&&prefs.getBytesLength("osd")==sizeof(data)&&prefs.getBytes("osd",data,sizeof(data))==sizeof(data))osdDecode(data,osd);}
