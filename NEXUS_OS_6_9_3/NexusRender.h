#pragma once
// NEXUS 7 render core: anti-aliased primitives blended into the RAM frame.
// Every shape is a signed-distance field evaluated only inside its bounding box,
// so cost stays proportional to the drawn area. Without a canvas (no PSRAM)
// the same code blends against the theme background instead of the frame.
#include <math.h>
static inline uint16_t* nxFrame(){return displayBuffered?displayCanvas->getFramebuffer():nullptr;}
static inline uint16_t nxBlend(uint16_t fg,uint16_t bg,int a){ // a: 0..255
 if(a>=255){return fg;}
 if(a<=0){return bg;}
 int r=(((fg>>11)&31)*a+((bg>>11)&31)*(255-a))/255;
 int g=(((fg>>5)&63)*a+((bg>>5)&63)*(255-a))/255;
 int b=((fg&31)*a+(bg&31)*(255-a))/255;return (uint16_t)((r<<11)|(g<<5)|b);
}
extern uint16_t themeBg;
static inline void nxPx(int x,int y,uint16_t c,int a){
 if(a<=0||x<0||y<0||x>=240||y>=280){return;}
 displayPending=true;
 uint16_t* fb=nxFrame();
 if(fb){uint16_t& p=fb[y*240+x];p=nxBlend(c,p,a);}
 else gfx->drawPixel(x,y,nxBlend(c,themeBg,a));
}
static inline int nxCover(float d){ // d: signed distance, negative inside
 if(d<=-0.5f){return 255;}
 if(d>=0.5f){return 0;}
 return (int)((0.5f-d)*255.0f);
}
// Filled disc.
void nxDisc(float cx,float cy,float r,uint16_t c,int alpha=255){
 int x0=floorf(cx-r-1),x1=ceilf(cx+r+1),y0=floorf(cy-r-1),y1=ceilf(cy+r+1);
 for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){float dx=x-cx,dy=y-cy;int a=nxCover(sqrtf(dx*dx+dy*dy)-r);if(a)nxPx(x,y,c,a*alpha/255);}
}
// Thick line with round caps (capsule).
void nxLine(float ax,float ay,float bx,float by,float w,uint16_t c,int alpha=255){
 float r=w*0.5f;int x0=floorf(std::min(ax,bx)-r-1),x1=ceilf(std::max(ax,bx)+r+1),y0=floorf(std::min(ay,by)-r-1),y1=ceilf(std::max(ay,by)+r+1);
 float vx=bx-ax,vy=by-ay,len2=vx*vx+vy*vy;if(len2<0.0001f)len2=0.0001f;
 for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){float px=x-ax,py=y-ay,t=(px*vx+py*vy)/len2;t=t<0?0:(t>1?1:t);float dx=px-vx*t,dy=py-vy*t;int a=nxCover(sqrtf(dx*dx+dy*dy)-r);if(a)nxPx(x,y,c,a*alpha/255);}
}
// Arc band between radii, angles in degrees clockwise from 12 o'clock, round caps.
void nxArc(float cx,float cy,float radius,float thick,float startDeg,float sweepDeg,uint16_t c,int alpha=255){
 if(sweepDeg<=0.2f){return;}
 if(sweepDeg>360)sweepDeg=360;
 float r=thick*0.5f,ro=radius+r+1;int x0=floorf(cx-ro),x1=ceilf(cx+ro),y0=floorf(cy-ro),y1=ceilf(cy+ro);
 float sa=startDeg*0.01745329f,ea=(startDeg+sweepDeg)*0.01745329f;
 float sx=cx+sinf(sa)*radius,sy=cy-cosf(sa)*radius,ex=cx+sinf(ea)*radius,ey=cy-cosf(ea)*radius;
 bool full=sweepDeg>=359.9f;
 for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){
  float dx=x-cx,dy=y-cy,dist=sqrtf(dx*dx+dy*dy);if(dist<radius-r-1||dist>radius+r+1)continue;
  float d;
  if(full)d=fabsf(dist-radius)-r;
  else{float ang=atan2f(dx,-dy)*57.29578f-startDeg;while(ang<0)ang+=360;while(ang>=360)ang-=360;
   if(ang<=sweepDeg)d=fabsf(dist-radius)-r;
   else{float d1=sqrtf((x-sx)*(x-sx)+(y-sy)*(y-sy)),d2=sqrtf((x-ex)*(x-ex)+(y-ey)*(y-ey));d=std::min(d1,d2)-r;}}
  int a=nxCover(d);if(a)nxPx(x,y,c,a*alpha/255);
 }
}
// Rounded rectangle: filled (stroke<=0) or outlined.
void nxRoundRect(float x,float y,float w,float h,float rad,uint16_t c,float stroke=0,int alpha=255){
 rad=std::min(rad,std::min(w,h)*0.5f);float cx=x+w*0.5f,cy=y+h*0.5f,hx=w*0.5f-rad,hy=h*0.5f-rad;
 int X0=floorf(x-1),X1=ceilf(x+w+1),Y0=floorf(y-1),Y1=ceilf(y+h+1);
 for(int py=Y0;py<=Y1;++py){float qy=fabsf(py+0.5f-cy)-hy;
  for(int px=X0;px<=X1;++px){float qx=fabsf(px+0.5f-cx)-hx;float ox=qx>0?qx:0,oy=qy>0?qy:0;
   float d=sqrtf(ox*ox+oy*oy)+std::min(std::max(qx,qy),0.0f)-rad;if(stroke>0)d=fabsf(d+stroke*0.5f)-stroke*0.5f;
   int a=nxCover(d);if(a)nxPx(px,py,c,a*alpha/255);}}
}
// Ordered-dither blend: hides RGB565 banding in soft gradients.
static const uint8_t NX_BAYER[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
static inline void nxPxD(int x,int y,uint16_t c,int a){
 if(a<=0||x<0||y<0||x>=240||y>=280){return;}
 displayPending=true;uint16_t* fb=nxFrame();uint16_t bg=fb?fb[y*240+x]:themeBg;
 int t=NX_BAYER[(y&3)*4+(x&3)]*16+8; // 0..255 threshold
 int R=(((c>>11)&31)*a+((bg>>11)&31)*(255-a)), G=(((c>>5)&63)*a+((bg>>5)&63)*(255-a)), B=((c&31)*a+(bg&31)*(255-a));
 R=(R+t)/255;G=(G+t)/255;B=(B+t)/255;if(R>31)R=31;if(G>63)G=63;if(B>31)B=31;
 uint16_t o=(R<<11)|(G<<5)|B;if(fb)fb[y*240+x]=o;else gfx->drawPixel(x,y,o);
}
// Soft radial glow used for ambient backgrounds (integer, cheap per pixel).
void nxGlow(int cx,int cy,int radius,uint16_t c,int strength){
 int r2=radius*radius;for(int y=std::max(0,cy-radius);y<std::min(280,cy+radius);++y)for(int x=std::max(0,cx-radius);x<std::min(240,cx+radius);++x){
  int d2=(x-cx)*(x-cx)+(y-cy)*(y-cy);if(d2>=r2)continue;int f=255-d2*255/r2;nxPxD(x,y,c,f*f/255*strength/255);}
}
// Watch hand: tapered capsule from a short counterweight to the tip.
void nxHand(float cx,float cy,float deg,float length,float tail,float w,uint16_t c){
 float a=deg*0.01745329f,s=sinf(a),k=-cosf(a);
 nxLine(cx-s*tail,cy-k*tail,cx+s*length,cy+k*length,w,c);
}
// Luminance test so light themes skip effects designed for dark glass.
static inline bool nxDark(uint16_t c){return ((c>>11)&31)*2+((c>>5)&63)+(c&31)*2<120;}
// Dithered vertical wash from the panel tone at the top into the background.
void nxWash(uint16_t top,uint16_t bottom,int fromY,int toY){
 for(int y=fromY;y<toY;++y){int a=(toY-y)*(toY-y)*110/((toY-fromY)*(toY-fromY));for(int x=0;x<240;++x)nxPxD(x,y,top,a);} (void)bottom;
}
