// Large force fields retain their source scale while raster work stays clipped
// to the LCD. Opaque sample coverage and guard words check both paths.
#include "draw.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
using namespace nova;
static struct {uint32_t first;uint16_t pixels[ScreenW*ScreenH];uint32_t last;} frame;
int main(){
 // Packed RGB565 arithmetic must match the original channel-wise floors.
 for(unsigned a=0;a<=255;a++)for(unsigned d=0;d<65536;d+=127)for(unsigned s=0;s<65536;s+=257){
  unsigned inv=256-a;
  uint16_t expected=a==255?(uint16_t)s:(uint16_t)(((((d>>11)*inv+(s>>11)*a)>>8)<<11)|
    (((((d>>5)&63)*inv+((s>>5)&63)*a)>>8)<<5)|(((d&31)*inv+(s&31)*a)>>8));
  assert(Canvas::blend((uint16_t)d,(uint16_t)s,a)==expected);
 }
 Canvas canvas;canvas.bind(frame.pixels);const uint16_t art[4]={0xf800,0x07e0,0x001f,0xffff};
 frame.first=frame.last=0xa5c39e71u;canvas.clear(0);canvas.sprite(10,20,art,nullptr,2,2,0,0);
 assert(frame.pixels[20*ScreenW+10]==art[0]&&frame.pixels[20*ScreenW+11]==art[1]);
 assert(frame.pixels[21*ScreenW+10]==art[2]&&frame.pixels[21*ScreenW+11]==art[3]);
 const uint8_t alpha[4]={255,127,0,255};canvas.clear(0);
 canvas.sprite(-1,0,art,alpha,2,2,0,1,256,0,256,65535,255,false,0,true);
 assert(frame.pixels[0]==art[3]);assert(frame.pixels[ScreenW]==Canvas::blend(0,art[1],127));
 const uint16_t white[4]={65535,65535,65535,65535};
 const int scales[]={2048,16384,65536,0x3fffffff};
 for(int scale:scales)for(unsigned diagonal=0;diagonal<2;diagonal++){
  canvas.clear(0);canvas.sprite(160,120,white,nullptr,2,2,1,1,diagonal?181:256,diagonal?181:0,scale);
  assert(frame.first==0xa5c39e71u&&frame.last==0xa5c39e71u);assert(frame.pixels[120*ScreenW+160]==65535);
  if(scale>=65536)for(uint16_t pixel:frame.pixels)assert(pixel==65535);
 }
 // Offscreen anchors and asymmetric source pivots cannot write outside LCD.
 for(int side=-1;side<=1;side+=2){canvas.sprite(side*1000,side*1000,white,nullptr,2,2,12,-7,181,181,16384,65535,128,true,0,false,32768);assert(frame.first==0xa5c39e71u&&frame.last==0xa5c39e71u);}
 puts("Renderer: unchanged unit sampling; large rotated/asymmetric fields clipped to LCD PASS");
}
