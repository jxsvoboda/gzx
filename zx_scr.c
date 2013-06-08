/*
  GZX - George's ZX Spectrum Emulator
  
  Spectrum Screen
*/

#include <stdio.h>
#include <stdlib.h>
#include "mgfx.h"
#include "global.h"
#include "zx_scr.h"
#include "z80.h"

#define SCR_SCAN_TOP     16
			  /* 64 total - 48 displayed */
#define SCR_SCAN_BOTTOM  304
			  /* 16 skip + 48 top border+192 screen+48 bottom border */
#define SCR_SCAN_RIGHT   352
			  /* 48 left border + 256 screen + 48 right b. */

int zxpal[3*16]={ /* taken from X128 */
      0,  0,  0,	  0,  0,159,	223,  0,  0,	224,  0,176,
      0,208,  0,	  0,208,208,	207,207,  0,	192,192,192,
      
      0,  0,  0,	  0,  0,175,	239,  0,  0,	255,  0,223,
      0,239,  0,	  0,255,255,	255,255,  0,	255,255,255
};

int gfxpal[3*256];

static u16 vxswapb(u16 ofs) {
  return (ofs & 0xf81f) | ((ofs & 0x00e0)<<3) | ((ofs & 0x0700)>>3);
}

unsigned long disp_t;
int fl_rev;

unsigned mains_x0,mains_x1i,mains_y0,mains_y1i;
unsigned scan_x0,scan_x1i,scan_y0,scan_y1i;

unsigned long disp_clock,disp_cbase;

static void g_scr_disp_fast(void);

static void n_scr_disp_fast(void);
static void n_scr_disp(void);

void (*zx_scr_disp_fast)(void)=n_scr_disp_fast;
void (*zx_scr_disp)(void)     =n_scr_disp;

/* the video signal is generated 50 times per second */
/* ULA swaps fg/bg colours every 16/50th of a second when flashing */

/* crude and fast display routine, called 50 times a second */
static void n_scr_disp_fast(void) {
  int x,y,xx,yy;
  u8 a,b,fgc,bgc,br,fl;
  
  mgfx_setcolor(border);
  
  /* draw border */
  
  /* top + corners */
  for(y=0;y<mains_y0;y++)
    for(x=0;x<scr_xs;x++)
      mgfx_drawpixel(x,y);
      
  /* bottom + corners */
  for(y=mains_y1i;y<scr_ys;y++)
    for(x=0;x<scr_xs;x++)
      mgfx_drawpixel(x,y);
      
  /* left */
  for(y=mains_y0;y<mains_y1i;y++)
    for(x=0;x<mains_x0;x++)
      mgfx_drawpixel(x,y);
      
  /* right */
  for(y=mains_y0;y<mains_y1i;y++)
    for(x=mains_x1i;x<scr_xs;x++)
      mgfx_drawpixel(x,y);

  /* draw main screen */
  
  for(y=0;y<24;y++) {
    for(x=0;x<32;x++) {
      a=zxscr[ZX_ATTR_START+y*32+x];
      br=(a>>6)&1;
      fgc=(a&7)|(br<<3);
      bgc=((a>>3)&7)|(br<<3);
      fl=a>>7;
      for(yy=0;yy<8;yy++) {
        a=zxscr[ZX_PIXEL_START+vxswapb((y*8+yy)*32+x)];
        for(xx=0;xx<8;xx++) {
	  b=(a&0x80);
	  if(fl && fl_rev) b=!b;
	  mgfx_setcolor(b ? fgc : bgc);
	  mgfx_drawpixel(mains_x0+x*8+xx,mains_y0+y*8+yy);
	  a<<=1;
	}
      }
    }
  }
}

#include "z80g.h"

u8 *background;

static void g_scr_disp_fast(void) {
  int x,y,i,j;
  unsigned buf;
  u8 b;
  
  mgfx_setcolor(border);
  
  /* draw border */
  
  /* top + corners */
  for(y=0;y<mains_y0;y++)
    for(x=0;x<scr_xs;x++)
      mgfx_drawpixel(x,y);
      
  /* bottom + corners */
  for(y=mains_y1i;y<scr_ys;y++)
    for(x=0;x<scr_xs;x++)
      mgfx_drawpixel(x,y);
      
  /* left */
  for(y=mains_y0;y<mains_y1i;y++)
    for(x=0;x<mains_x0;x++)
      mgfx_drawpixel(x,y);
      
  /* right */
  for(y=mains_y0;y<mains_y1i;y++)
    for(x=mains_x1i;x<scr_xs;x++)
      mgfx_drawpixel(x,y);

  /* draw main screen */
  
  for(y=0;y<24*8;y++) {
    for(x=0;x<32;x++) {
      buf=vxswapb(y*32+x);
      for(i=0;i<8;i++) {
        b=0;
	for(j=0;j<8;j++) {
	  if(gfxscr[j][buf]&(1<<(7-i))) b |= (1<<j);
	}
	if(b!=0) mgfx_setcolor(b);
	  else mgfx_setcolor(background[(mains_y0+y)*320+mains_x0+x*8+i]);
	mgfx_drawpixel(mains_x0+x*8+i,mains_y0+y);
      }
    }
  }
}





/* line has 1 pixel, col is 8 pixels(1 byte) wide */
static void scr_dispscrelem(int col, int line) {
  u8 attr;
  u8 pix;
  u8 rev;
  u8 fgc,bgc,br;
  int x,y,i;
  
  attr=zxscr[ZX_ATTR_START+(line>>3)*32+col];
  pix=zxscr[ZX_PIXEL_START+vxswapb(line*32+col)];
  rev=((attr>>7) && fl_rev)?0x80:0;
  br=(attr>>6)&1;
  fgc=(attr&7)|(br<<3);
  bgc=((attr>>3)&7)|(br<<3);
  
  x=scan_x0+8*(col+6);
  y=scan_y0+(line+48);
  
  for(i=0;i<8;i++) {
    mgfx_setcolor(((pix&0x80)^rev) ? fgc : bgc);
    
    if(x+i>=0 && y>=0 && x+i<scr_xs && y<scr_ys)
      mgfx_drawpixel(x+i,y);
     
    pix<<=1;
  }
}

/* line has 1 pixel, col is 8 pixels(1 byte) wide */
static void scr_dispbrdelem(int col, int line) {
  int x,y,i;
  
  x=scan_x0+8*col;
  y=scan_y0+line;
  
  mgfx_setcolor(border);
  
  for(i=0;i<8;i++) {
    if(x+i>=0 && y>=0 && x+i<scr_xs && y<scr_ys)
      mgfx_drawpixel(x+i,y);
  }
}

/* slow and fine display routine, called after each instruction! */
static void n_scr_disp(void) {
  unsigned line,col;
  
  line=disp_clock/224;
  col=(disp_clock%224)/4;
  if(line>=SCR_SCAN_TOP && line<SCR_SCAN_BOTTOM && col<SCR_SCAN_RIGHT) {
    if(col>=6 && col<(6+32) && line>=64 && line<(64+192))
      scr_dispscrelem(col-6,line-64);
    else
      scr_dispbrdelem(col,line-16);
  }
    
  disp_clock+=4;
}

void zx_scr_mode(int mode) {
  if(mode) {
    zx_scr_disp_fast=g_scr_disp_fast;
    mgfx_setpal(0,256,gfxpal);
  } else {
    zx_scr_disp_fast=n_scr_disp_fast;
    mgfx_setpal(0,16,zxpal);
  }
}

int zx_scr_init(void) {
  int i;
  int b;
  FILE *f;
  
  f=fopen("sp256.pal","rt");
  if(!f) return -1;
  
  for(i=0;i<3*256;i++) {
    fscanf(f,"%d",&b);
    gfxpal[i]=b>>2;
  }
  fclose(f);
  
  background=malloc(64000);
  if(!background) return -1;
  f=fopen("knlore.b00","rb");
  if(!f) return -1;
  fread(background,1,64000,f);
  fclose(f);
  
  for(i=0;i<3*16;i++) zxpal[i]>>=2;
  
  if(mgfx_init()) return -1;
  mgfx_setpal(0,16,zxpal);
  
//  scr_ys>>=1;
  
  mains_x0=(scr_xs>>1)-(256>>1);
  mains_y0=(scr_ys>>1)-(192>>1);
  mains_x1i=mains_x0+256;
  mains_y1i=mains_y0+192;
  
  scan_x0=(scr_xs>>1)-(352>>1);
  scan_y0=(scr_ys>>1)-(288>>1);
  scan_x1i=mains_x0+352;
  scan_y1i=mains_y0+288;
  
//  scr_ys<<=1;
  
  disp_clock=0;

  return 0;
}
