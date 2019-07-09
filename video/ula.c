/*
 * GZX - George's ZX Spectrum Emulator
 * ULA video generator
 *
 * Copyright (c) 1999-2019 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "../clock.h"
#include "../memio.h"
#include "../mgfx.h"
#include "../z80.h"
#include "out.h"
#include "ula.h"

#ifdef USE_GPU
#include "../z80g.h"
#endif

#define SCR_SCAN_TOP     16
			  /* 64 total - 48 displayed */
#define SCR_SCAN_BOTTOM  304
			  /* 16 skip + 48 top border+192 screen+48 bottom border */
#define SCR_SCAN_RIGHT   352
			  /* 48 left border + 256 screen + 48 right b. */

static const int zxpal[3 * 16] = { /* taken from X128 */
      0,  0,  0,	  0,  0,159,	223,  0,  0,	224,  0,176,
      0,208,  0,	  0,208,208,	207,207,  0,	192,192,192,

      0,  0,  0,	  0,  0,175,	239,  0,  0,	255,  0,223,
      0,239,  0,	  0,255,255,	255,255,  0,	255,255,255
};

static uint16_t vxswapb(uint16_t ofs)
{
	return (ofs & 0xf81f) | ((ofs & 0x00e0) << 3) | ((ofs & 0x0700) >> 3);
}

/* the video signal is generated 50 times per second */
/* ULA swaps fg/bg colours every 16/50th of a second when flashing */

void video_ula_next_field(video_ula_t *ula)
{
	video_out_end_field(ula->vout);

	ula->clock = 0;
	ula->cbase += ULA_FIELD_TICKS;

	++ula->field_no;
	if (ula->field_no >= 2) {
		ula->field_no = 0;
		++ula->frame_no;
	}

	if (ula->frame_no >= 16) {
		ula->frame_no = 0;
		ula->fl_rev = !ula->fl_rev;
	}

	z80_int(0xff);
#ifdef USE_GPU
	z80_g_int(0xff);
#endif
}

/* crude and fast display routine, called 50 times a second */
void video_ula_disp_fast(video_ula_t *ula)
{
  int x,y,xx,yy;
  uint8_t a,b,fgc,bgc,br,fl;

  /* draw border */

  /* top + corners */
  video_out_rect(ula->vout, 0, 0, scr_xs - 1, ula->mains_y0 - 1, border);

  /* bottom + corners */
  video_out_rect(ula->vout, 0, ula->mains_y1i, scr_xs - 1, scr_ys - 1, border);

  /* left */
  video_out_rect(ula->vout, 0, ula->mains_y0, ula->mains_x0 - 1,
    ula->mains_y1i - 1, border);

  /* right */
  video_out_rect(ula->vout, ula->mains_x1i, ula->mains_y0, scr_xs - 1,
    ula->mains_y1i - 1, border);

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
	  if(fl && ula->fl_rev) b=!b;
	  video_out_pixel(ula->vout, ula->mains_x0+x*8+xx,ula->mains_y0+y*8+yy,
	    b ? fgc : bgc);
	  a<<=1;
	}
      }
    }
  }

  video_ula_next_field(ula);
}

/* line has 1 pixel, col is 8 pixels(1 byte) wide */
static void scr_dispscrelem(video_ula_t *ula, int col, int line) {
  uint8_t attr;
  uint8_t pix;
  uint8_t rev;
  uint8_t fgc,bgc,br;
  uint8_t color;
  int x,y,i;
  
  attr=zxscr[ZX_ATTR_START+(line>>3)*32+col];
  pix=zxscr[ZX_PIXEL_START+vxswapb(line*32+col)];
  rev=((attr>>7) && ula->fl_rev)?0x80:0;
  br=(attr>>6)&1;
  fgc=(attr&7)|(br<<3);
  bgc=((attr>>3)&7)|(br<<3);
  
  x=ula->scan_x0+8*(col+6);
  y=ula->scan_y0+(line+48);
  
  for(i=0;i<8;i++) {
    color = ((pix&0x80)^rev) ? fgc : bgc;
    
    if(x+i>=0 && y>=0 && x+i<scr_xs && y<scr_ys)
      video_out_pixel(ula->vout, x+i,y, color);
     
    pix<<=1;
  }
}

/* line has 1 pixel, col is 8 pixels(1 byte) wide */
static void scr_dispbrdelem(video_ula_t *ula, int col, int line) {
  int x,y;
  
  x=ula->scan_x0+8*col;
  y=ula->scan_y0+line;
  
  video_out_rect(ula->vout, x, y, x+7, y, border);
}

/* slow and fine display routine, called after each instruction! */
void video_ula_disp(video_ula_t *ula)
{
  unsigned line,col;
  
  line=ula->clock/224;
  col=(ula->clock%224)/4;
  if(line>=SCR_SCAN_TOP && line<SCR_SCAN_BOTTOM && col<SCR_SCAN_RIGHT) {
    if(col>=6 && col<(6+32) && line>=64 && line<(64+192))
      scr_dispscrelem(ula,col-6,line-64);
    else
      scr_dispbrdelem(ula,col,line-16);
  }
    
  ula->clock+=4;
  if (ula->clock >= ULA_FIELD_TICKS)
    video_ula_next_field(ula);
}

int video_ula_init(video_ula_t *ula, unsigned long clock, video_out_t *vout)
{
  ula->vout = vout;
  video_ula_setpal(ula);
//  scr_ys>>=1;
  
  ula->mains_x0=(scr_xs>>1)-(256>>1);
  ula->mains_y0=(scr_ys>>1)-(192>>1);
  ula->mains_x1i=ula->mains_x0+256;
  ula->mains_y1i=ula->mains_y0+192;
  
  ula->scan_x0=(scr_xs>>1)-(352>>1);
  ula->scan_y0=(scr_ys>>1)-(288>>1);
  ula->scan_x1i=ula->mains_x0+352;
  ula->scan_y1i=ula->mains_y0+288;
  
//  scr_ys<<=1;
  
  ula->clock = 0;
  ula->cbase = clock;

  return 0;
}

void video_ula_setpal(video_ula_t *ula)
{
  int i;
  uint8_t pal[3*16];

  for(i=0;i<3*16;i++) pal[i] = zxpal[i]>>2;
  
  video_out_set_palette(ula->vout, 16, pal);
}

unsigned long video_ula_get_clock(video_ula_t *ula)
{
	return ula->cbase + ula->clock;
}
