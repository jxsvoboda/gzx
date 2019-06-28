/*
 * GZX - George's ZX Spectrum Emulator
 * Spec256 video generator
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
//#include "video/defs.h"
#include "../clock.h"
#include "../mgfx.h"
#include "../memio.h"
#include "../z80g.h"
#include "spec256.h"

static uint16_t vxswapb(uint16_t ofs) {
  return (ofs & 0xf81f) | ((ofs & 0x00e0)<<3) | ((ofs & 0x0700)>>3);
}


void video_spec256_disp_fast(video_spec256_t *spec) {
  int x,y,i,j;
  unsigned buf;
  uint8_t b;
  
  mgfx_setcolor(border);
  
  /* draw border */
  
  /* top + corners */
  for(y=0;y<spec->mains_y0;y++)
    for(x=0;x<scr_xs;x++)
      mgfx_drawpixel(x,y);
      
  /* bottom + corners */
  for(y=spec->mains_y1i;y<scr_ys;y++)
    for(x=0;x<scr_xs;x++)
      mgfx_drawpixel(x,y);
      
  /* left */
  for(y=spec->mains_y0;y<spec->mains_y1i;y++)
    for(x=0;x<spec->mains_x0;x++)
      mgfx_drawpixel(x,y);
      
  /* right */
  for(y=spec->mains_y0;y<spec->mains_y1i;y++)
    for(x=spec->mains_x1i;x<scr_xs;x++)
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
	  else mgfx_setcolor(spec->background[(spec->mains_y0+y)*320+spec->mains_x0+x*8+i]);
	mgfx_drawpixel(spec->mains_x0+x*8+i,spec->mains_y0+y);
      }
    }
  }

  spec->clock += ULA_FIELD_TICKS;
}

int video_spec256_init(video_spec256_t *spec) {
  int b;
  int i;
  FILE *f;
  
  f=fopen("sp256.pal","rt");
  if(!f) return -1;
  
  for(i=0;i<3*256;i++) {
    fscanf(f,"%d",&b);
    spec->gfxpal[i]=b>>2;
  }
  fclose(f);
  
  spec->background=calloc(64000, 1);
  if(!spec->background) return -1;

//  scr_ys>>=1;
  
  spec->mains_x0=(scr_xs>>1)-(256>>1);
  spec->mains_y0=(scr_ys>>1)-(192>>1);
  spec->mains_x1i=spec->mains_x0+256;
  spec->mains_y1i=spec->mains_y0+192;
  
//  scr_ys<<=1;
  
  spec->clock=0;

  return 0;
}

int video_spec256_load_bg(video_spec256_t *spec, const char *fname)
{
  FILE *f;
  f=fopen(fname,"rb");
  if(!f) return -1;
  fread(spec->background,1,64000,f);
  fclose(f);
  return 0;
}

void video_spec256_setpal(video_spec256_t *spec)
{
	mgfx_setpal(0,256,spec->gfxpal);
}

unsigned long video_spec256_get_clock(video_spec256_t *spec)
{
	return spec->clock;
}
