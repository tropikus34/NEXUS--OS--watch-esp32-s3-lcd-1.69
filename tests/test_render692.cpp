#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
#include <cassert>
int main(){
 setup();ecoMode=true;screenTimeoutIndex=4;currentScreen=SC_CLOCK;
 fakeMs=31000;lastTick=fakeMs;lastRtcPoll=fakeMs;lastActivity=fakeMs;
 batPercent=0;uiDirty=true;previewActive=false;alarmRinging=timerRinging=false;
 auto fills=gfx->fullBackgroundCalls;auto frames=displayFrames;
 loop();assert(batPercent>0);assert(gfx->fullBackgroundCalls==fills+1);assert(displayFrames==frames+1);
 // A new battery value must not replace a notification overlay.
 fakeMs+=30000;lastTick=fakeMs;lastRtcPoll=fakeMs;previewActive=true;previewStart=fakeMs;batPercent=0;uiDirty=true;
 frames=displayFrames;loop();assert(previewActive);assert(displayFrames==frames+1);
 puts("PASS RENDER: battery + UI event composed once; notification overlay survives battery refresh");
}
