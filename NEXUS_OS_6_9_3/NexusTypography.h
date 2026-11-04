#pragma once
#include "NexusFont.h"
int premiumIndex(uint32_t cp) {
 if(cp>=32 && cp<127) return cp-32;
 if(cp>=0x400 && cp<0x460) return 95+cp-0x400;
 if(cp==0x490 || cp==0x491) return 191+cp-0x490;
 return '?'-32;
}
int premiumAdvance(uint32_t cp) {return PREMIUM_ADV[premiumIndex(cp)];}
void premiumDraw(uint32_t cp,int x,int y,int scale,uint16_t color,uint16_t bg) {
 int index=premiumIndex(cp);
 for(int row=0;row<18;++row) {
  for(int col=0;col<16;) {
   int alpha=(PREMIUM_FONT[index][row*4+col/4]>>(6-2*(col%4)))&3;
   int start=col++;
   while(col<16 && ((PREMIUM_FONT[index][row*4+col/4]>>(6-2*(col%4)))&3)==alpha) ++col;
   if(alpha) gfx->fillRect(x+start*scale,y+row*scale,(col-start)*scale,scale,colorMix(color,bg,alpha));
  }
 }
}
int premiumTitleAdvance(uint32_t cp) {return PREMIUM_TITLE_ADV[premiumIndex(cp)];}
void premiumTitleDraw(uint32_t cp,int x,int y,uint16_t color,uint16_t bg) {
 int index=premiumIndex(cp);
 for(int row=0;row<30;++row) {
  for(int col=0;col<28;) {
   int alpha=(PREMIUM_TITLE[index][row*7+col/4]>>(6-2*(col%4)))&3;
   int start=col++;
   while(col<28 && ((PREMIUM_TITLE[index][row*7+col/4]>>(6-2*(col%4)))&3)==alpha) ++col;
   if(alpha) gfx->fillRect(x+start,y+row,col-start,1,colorMix(color,bg,alpha));
  }
 }
}
