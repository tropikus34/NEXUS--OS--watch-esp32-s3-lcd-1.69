#pragma once
// Compose in RAM. Never expose background clearing and individual glyph draws on SPI.
Arduino_Canvas* displayCanvas=new Arduino_Canvas(LCD_WIDTH,LCD_HEIGHT,lcd);
Arduino_GFX* gfx=displayCanvas;
bool displayBuffered=false,displayPending=false;
uint32_t displayFrames=0;
bool displayBegin(){
 if(displayCanvas->begin()){gfx=displayCanvas;displayBuffered=true;return true;}
 // A missing canvas allocation must not put a functioning watch in a reboot loop.
 gfx=lcd;displayBuffered=false;Serial.println("Canvas unavailable: enable PSRAM; direct display fallback");return lcd->begin();
}
// 6.9.4: send only the changed row band. A shadow copy of the last sent frame
// (PSRAM) is compared row-by-row; GRAM survives SLPIN so it stays valid across sleep.
uint16_t* displayShadow=nullptr;bool displayShadowValid=false;uint32_t displayRowsSent=0;
void displayPresent(){
 if(!displayPending)return;displayPending=false;++displayFrames;
 if(!displayBuffered)return;
 uint16_t* fb=displayCanvas->getFramebuffer();
 if(!displayShadow){displayShadow=(uint16_t*)ps_malloc(LCD_WIDTH*LCD_HEIGHT*2);displayShadowValid=false;}
 if(!fb||!displayShadow){gfx->flush();displayRowsSent+=LCD_HEIGHT;return;}
 int y0=0,y1=LCD_HEIGHT-1;
 if(displayShadowValid){
  const size_t row=LCD_WIDTH*2;
  while(y0<LCD_HEIGHT&&!memcmp(fb+y0*LCD_WIDTH,displayShadow+y0*LCD_WIDTH,row))++y0;
  if(y0==LCD_HEIGHT)return; // nothing changed: no bus traffic
  while(y1>y0&&!memcmp(fb+y1*LCD_WIDTH,displayShadow+y1*LCD_WIDTH,row))--y1;
 }
 int rows=y1-y0+1;
 lcd->draw16bitRGBBitmap(0,y0,fb+y0*LCD_WIDTH,LCD_WIDTH,rows);
 memcpy(displayShadow+y0*LCD_WIDTH,fb+y0*LCD_WIDTH,(size_t)rows*LCD_WIDTH*2);
 displayShadowValid=true;displayRowsSent+=rows;
}
void displayInvalidate(){displayShadowValid=false;displayPending=true;}
