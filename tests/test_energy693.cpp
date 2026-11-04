#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
#include <cassert>
int main(){setup();assert(batteryCapacityMah==600);assert(batterySetCapacity(450));loadBatteryCapacity();assert(batteryCapacityMah==450);
 assert(MOTION_POLL_MS<1000/21); // do not poll slower than configured sensor output
 auto& a=NimBLEDevice::adv;auto& server=NimBLEDevice::server;server.count=0;ecoMode=true;screenAwake=false;autoReconnect=true;pairWindowActive=false;bleLastConnectedAt=millis();
 fakeMs+=5000;bleService();assert(a.active&&a.minInterval==160);int starts=a.starts;
 fakeMs+=5000;bleService();assert(a.starts==starts);
 fakeMs+=25000;bleService();assert(a.active&&a.minInterval==1600&&a.maxInterval==2000);starts=a.starts;
 fakeMs+=5000;bleService();assert(a.starts==starts);
 pairWindowActive=true;pairWindowStart=millis();fakeMs+=5000;bleService();assert(a.active&&a.minInterval==160);
 pairWindowActive=false;autoReconnect=false;fakeMs+=5000;bleService();assert(!a.active);
 autoReconnect=true;server.count=1;fakeMs+=5000;bleService();assert(server.count==1&&!a.active); // never restart radio advertising on connected link
 puts("PASS ENERGY: preserved capacity, bounded IMU polling, fast/slow advertising, pairing, no repeated restarts, connected link preserved");}
