#include <cassert>
#include "../NEXUS_OS_6_9_3/NEXUS_OS_6_9_3.ino"
std::string send611(const std::string& command){Serial.output.clear();for(char c:command+"\n")Serial.input.push_back(c);while(Serial.available())themeSerialService();return Serial.output;}
int main(){setup();assert(displayBuffered&&gfx!=lcd&&displayFrames>0);assert(gfx->framebuffer==lcd->framebuffer);
 // Drawing a new background/time affects RAM only. The visible panel changes once,
 // after the complete composed frame is ready, and is not flushed again while idle.
 auto old=lcd->framebuffer;themeIndex=7;customBg=0x07e0;applyTheme();currentScreen=SC_CLOCK;timeValid=true;hh=12;mm=48;ss=37;drawClockScreen();assert(displayPending&&lcd->framebuffer==old&&gfx->framebuffer!=old);auto frames=displayFrames;displayPresent();assert(displayFrames==frames+1&&gfx->framebuffer==lcd->framebuffer);displayPresent();assert(displayFrames==frames+1);
 suspendDisplay();assert(lcd->displaySleeping);wakeDisplay();assert(!lcd->displaySleeping);
 // A complete RGB565 image is actually rendered behind both ordinary and OSD faces.
 std::vector<uint8_t> pixels(WALL_BYTES);for(size_t j=0;j<pixels.size();j+=2){pixels[j]=0x1f;pixels[j+1]=0xf8;}
 auto wall=LittleFS.open("/wall.rgb","w");assert(wall.write(pixels.data(),pixels.size())==pixels.size());wall.close();wallpaperPresent=wallpaperEnabled=themeFsReady=true;wallpaperMix=100;uiBackground();for(auto pixel:gfx->framebuffer)assert(pixel==0xf81f);assert(lcd->framebuffer!=gfx->framebuffer);displayPresent();assert(lcd->framebuffer==gfx->framebuffer);
 themeReceiving=true;uiBackground();assert(gfx->framebuffer[0]==0xf81f);themeReceiving=false;
 wallpaperMix=25;uiBackground();assert(gfx->framebuffer[0]==colorMix(0xf81f,themeBg,25,100));osd.enabled=true;wallpaperMix=100;drawClockScreen();assert(gfx->framebuffer[0]==0xf81f);displayPresent();assert(lcd->framebuffer[0]==0xf81f);
 // No memory-induced reboot: a failed canvas allocation selects the direct fallback.
 fakeCanvasAvailable=false;assert(displayBegin()&&!displayBuffered&&gfx==lcd);fakeCanvasAvailable=true;assert(displayBegin()&&displayBuffered&&gfx==displayCanvas);
 // Mount failures are never automatically formatted. Missing partition and insufficient
 // free space have distinct errors and do not destroy the currently saved image.
 int formats=LittleFS.formatCalls;fakeFsPartition=false;loadThemeSettings();assert(!themeFsReady&&LittleFS.formatCalls==formats);assert(send611("NXFORMAT CONFIRM").find("ERR FS_NO_PARTITION")!=std::string::npos&&LittleFS.formatCalls==formats);assert(send611("NXBG1 134400 0").find("ERR FS_NO_PARTITION")!=std::string::npos);
 fakeFsPartition=true;LittleFS.ready=false;loadThemeSettings();assert(!themeFsReady&&LittleFS.formatCalls==formats);assert(send611("NXBG1 134400 0").find("ERR NO_FILESYSTEM")!=std::string::npos);assert(send611("NXFORMAT CONFIRM").find("OK FILESYSTEM")!=std::string::npos&&LittleFS.formatCalls==formats+1);
 wall=LittleFS.open("/wall.rgb","w");wall.write(pixels.data(),pixels.size());wall.close();wallpaperPresent=true;LittleFS.capacity=WALL_BYTES+1024;assert(send611("NXBG1 134400 0").find("ERR FS_NO_SPACE")!=std::string::npos&&!themeReceiving);assert(LittleFS.open("/wall.rgb","r").size()==WALL_BYTES);LittleFS.capacity=0x170000;
 auto config=send611("NXGETCONFIG");assert(config.find("\"buffered\":1")!=std::string::npos&&config.find("\"fsPartitionBytes\":1507328")!=std::string::npos);
 puts("PASS DISPLAY/STORAGE: off-screen composition, one complete presentation/no idle flush, physical LCD sleep, image rendering/mixing in native and OSD, allocation fallback, no autoformat, missing partition/unformatted/no-space errors");
}
