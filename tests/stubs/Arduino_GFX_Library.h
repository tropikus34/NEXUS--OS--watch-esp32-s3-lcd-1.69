#pragma once
#include "Arduino.h"
#include "HostFont.h"
#include <fstream>
#include <sstream>
class Arduino_DataBus{};
class Arduino_ESP32SPI:public Arduino_DataBus {public:Arduino_ESP32SPI(int,int,int,int){} };
class Arduino_GFX {public:
 std::vector<uint16_t> framebuffer=std::vector<uint16_t>(240*280);int cx=0,cy=0,scale=1;uint16_t ink=0xffff;
 bool displaySleeping=false;int sleepCount=0,wakeCount=0;void displayOn(){displaySleeping=false;++wakeCount;}void displayOff(){displaySleeping=true;++sleepCount;}
 virtual bool begin(){return true;}virtual void flush(bool=false){}void setTextWrap(bool){}void setTextSize(int n){scale=n;}void setTextColor(uint16_t c){ink=c;}void setCursor(int x,int y){cx=x;cy=y;}
 void drawPixel(int x,int y,uint16_t c){if(x>=0&&x<240&&y>=0&&y<280)framebuffer[y*240+x]=c;}
 unsigned fillScreenCalls=0, fullBackgroundCalls=0;
 void fillScreen(uint16_t c){++fillScreenCalls;std::fill(framebuffer.begin(),framebuffer.end(),c);}
 void fillRect(int x,int y,int w,int h,uint16_t c){if(x==0&&y==0&&w==240&&h==280)++fullBackgroundCalls;for(int j=std::max(y,0);j<std::min(y+h,280);++j)for(int i=std::max(x,0);i<std::min(x+w,240);++i)drawPixel(i,j,c);}
 void drawRect(int x,int y,int w,int h,uint16_t c){fillRect(x,y,w,1,c);fillRect(x,y+h-1,w,1,c);fillRect(x,y,1,h,c);fillRect(x+w-1,y,1,h,c);}
 void drawCircle(int x,int y,int r,uint16_t c){for(int j=-r;j<=r;++j)for(int i=-r;i<=r;++i){int d=i*i+j*j;if(d<=r*r&&d>(r-1)*(r-1))drawPixel(x+i,y+j,c);}}
 void fillCircle(int x,int y,int r,uint16_t c){for(int j=-r;j<=r;++j)for(int i=-r;i<=r;++i)if(i*i+j*j<=r*r)drawPixel(x+i,y+j,c);}
 void fillRoundRect(int x,int y,int w,int h,int r,uint16_t c){r=std::min(r,std::min(w,h)/2);for(int j=0;j<h;++j)for(int i=0;i<w;++i){int dx=i<r?r-i-1:(i>=w-r?i-(w-r):0),dy=j<r?r-j-1:(j>=h-r?j-(h-r):0);if(dx*dx+dy*dy<=r*r)drawPixel(x+i,y+j,c);}}
 void drawRoundRect(int x,int y,int w,int h,int r,uint16_t c){for(int j=0;j<h;++j)for(int i=0;i<w;++i){int dx=i<r?r-i-1:(i>=w-r?i-(w-r):0),dy=j<r?r-j-1:(j>=h-r?j-(h-r):0);bool outer=dx*dx+dy*dy<=r*r;int ix=i-1,iy=j-1,ir=std::max(r-1,0),iw=w-2,ih=h-2;int ax=ix<ir?ir-ix-1:(ix>=iw-ir?ix-(iw-ir):0),ay=iy<ir?ir-iy-1:(iy>=ih-ir?iy-(ih-ir):0);bool inner=ix>=0&&iy>=0&&ix<iw&&iy<ih&&ax*ax+ay*ay<=ir*ir;if(outer&&!inner)drawPixel(x+i,y+j,c);}}
 void drawLine(int x0,int y0,int x1,int y1,uint16_t c){int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;while(true){drawPixel(x0,y0,c);if(x0==x1&&y0==y1)break;int e2=2*err;if(e2>=dy){err+=dy;x0+=sx;}if(e2<=dx){err+=dx;y0+=sy;}}}
 void drawFastHLine(int x,int y,int w,uint16_t c){fillRect(x,y,w,1,c);}
 void draw16bitRGBBitmap(int x,int y,const uint16_t*p,int w,int h){for(int j=0;j<h;++j)for(int i=0;i<w;++i)drawPixel(x+i,y+j,p[j*w+i]);}
 void write(uint8_t ch){if(ch>=32&&ch<127){for(int y=0;y<8;++y)for(int x=0;x<6;++x)if(hostFont[ch-32][y]&(1<<(5-x)))fillRect(cx+x*scale,cy+y*scale,scale,scale,ink);}cx+=6*scale;}
 template<class T>void print(T value){std::ostringstream s;s<<value;for(auto c:s.str())write(c);}
 void save(const char*path){std::ofstream f(path,std::ios::binary);f<<"P6\n240 280\n255\n";for(auto c:framebuffer){char rgb[]={(char)(((c>>11)&31)*255/31),(char)(((c>>5)&63)*255/63),(char)((c&31)*255/31)};f.write(rgb,3);}}
};
class Arduino_ST7789:public Arduino_GFX {public:template<class...T>Arduino_ST7789(T...){} };

inline bool fakeCanvasAvailable=true;
class Arduino_Canvas:public Arduino_GFX {public:uint16_t* getFramebuffer(){return framebuffer.data();}Arduino_GFX* output;int presentations=0;Arduino_Canvas(int,int,Arduino_GFX* p):output(p){}bool begin()override{return fakeCanvasAvailable&&output->begin();}void flush(bool=false)override{++presentations;output->draw16bitRGBBitmap(0,0,framebuffer.data(),240,280);}};
