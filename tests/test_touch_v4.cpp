#include <cassert>
#include "stubs/Arduino.h"
#include "stubs/Wire.h"
#include "../NEXUS_OS_6_9_3/NexusTouch.h"
static int nexts=0,prevs=0,taps=0,holds=0;
void n(){++nexts;} void p(){++prevs;} void t(){++taps;} void h(){++holds;}
static void contact(bool press,int x,int y){
 auto& d=Wire.devices[0x15]; d[0x02]=press?1:0;d[0x03]=(x>>8)&15;d[0x04]=x&255;d[0x05]=(y>>8)&15;d[0x06]=y&255;
}
static bool tick(uint32_t ms,bool awake=true){fakeMs+=ms;return NexusTouch::service(awake,true,false,n,p,t,h);}
int main(){
 Wire.emulate=true;Wire.devices[0x15]={};NexusTouch::begin();assert(NexusTouch::present);
 // short tap
 contact(true,100,100);tick(50);contact(false,100,100);assert(tick(60));assert(taps==1);
 // swipe left, with earlier tap debounce expired
 tick(250);contact(true,200,130);tick(50);contact(true,140,131);tick(50);contact(false,140,131);assert(tick(50));assert(nexts==1);
 // right
 tick(250);contact(true,40,130);tick(50);contact(true,100,130);tick(50);contact(false,100,130);assert(tick(50));assert(prevs==1);
 // no tap when LCD off
 tick(250);contact(true,110,100);assert(!tick(50,false));contact(false,110,100);tick(50);assert(taps==1);
 // hold delivered once
 tick(250);contact(true,120,120);tick(50);assert(tick(760));tick(90);contact(false,120,120);tick(50);assert(holds==1 && taps==1);
 // disconnect after repeated I2C errors
 Wire.fail=true;for(int i=0;i<8;i++)tick(40);assert(!NexusTouch::present);
 Wire.fail=false;fakeMs+=10000;NexusTouch::serviceReconnect(true);assert(NexusTouch::present);
}
