/*
 * GZX - George's ZX Spectrum Emulator
 * Z80 GPU (a.k.a. Spec256)
 *
 * Copyright (c) 1999-2017 Jiri Svoboda
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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "memio.h"
#include "z80g.h"
#include "zx_scr.h"

/** Allow probing for GFX and turning on GPU when needed */
bool gpu_allow = true;

z80s gpus[NGP];		/* GPUs */
z80s tmps;		/* temporary place to store the CPU */

uint8_t *gfxrom[NGP];
uint8_t *gfxram[NGP];
uint8_t *gfxscr[NGP];
static uint8_t *gfxbnk[NGP][4];
static bool gpu_on;

/************************************************************************/
/************************************************************************/

void gpu_set_allow(bool allow)
{
	gpu_allow = allow;
	if (!allow && gpu_on)
		gpu_disable();
}

void gpu_init(void)
{
  int i;

  for(i=0;i<NGP;i++) {
    gfxrom[i]=NULL;
    gfxram[i]=NULL;
  }

  gpu_on = false;
}

int gpu_enable(void)
{
  if (mem_model != ZXM_48K)
    return -1;
  gfx_select_memmodel(ZXM_48K);
  if (zx_scr_init_spec256_pal() < 0)
    return -1;
  gfxrom_load("roms/rom0.gfx",0);
  gpu_on = true;
  return 0;
}

void gpu_disable(void)
{
  int i;

  for(i=0;i<NGP;i++) {
    free(gfxrom[i]);
    gfxrom[i]=NULL;
    free(gfxram[i]);
    gfxram[i]=NULL;
  }

  gpu_on = false;
  zx_scr_mode(0);
}

bool gpu_is_on(void)
{
  return gpu_on;
}

void gfx_select_memmodel(int model) {
  int i;
  uint8_t *tmpram,*tmprom,*tmpscr;
  uint8_t *tmpbnk[4];
  
  tmprom=zxrom;
  tmpram=zxram;
  tmpscr=zxscr;
  
  tmpbnk[0]=zxbnk[0];
  tmpbnk[1]=zxbnk[1];
  tmpbnk[2]=zxbnk[2];
  tmpbnk[3]=zxbnk[3];
  
  for(i=0;i<NGP;i++) {
    zxrom=gfxrom[i];
    zxram=gfxram[i];
    zxscr=gfxscr[i];
    
    zx_select_memmodel(model);
    
    gfxrom[i]=zxrom;
    gfxram[i]=zxram;
    gfxscr[i]=zxscr;
    
    gfxbnk[i][0]=zxbnk[0];
    gfxbnk[i][1]=zxbnk[1];
    gfxbnk[i][2]=zxbnk[2];
    gfxbnk[i][3]=zxbnk[3];
  }
  
  zxrom=tmprom;
  zxram=tmpram;
  zxscr=tmpscr;
  
  zxbnk[0]=tmpbnk[0];
  zxbnk[1]=tmpbnk[1];
  zxbnk[2]=tmpbnk[2];
  zxbnk[3]=tmpbnk[3];
}

#define GRANU 1

/* execute instruction using both CPU and GPU */
void z80_g_execinstr(void) {
  int i,j;
  unsigned long tmp_clock;
  uint8_t *tmpbnk[4];

  tmp_clock=z80_clock;

  /*
   * Synchronize GPUs with CPU
   *
   * Synchronize everything but Gfx registers
   */
  for(i=0;i<NGP;i++) {
    gpus[i].PC=cpus.PC;
    gpus[i].SP=cpus.SP;
    gpus[i].I=cpus.I;
    gpus[i].R=cpus.R;
    gpus[i].IFF1=cpus.IFF1;
    gpus[i].IFF2=cpus.IFF2;
    gpus[i].modifier=cpus.modifier;
    gpus[i].int_lock=cpus.int_lock;
    gpus[i].halted=cpus.halted;
    gpus[i].int_mode=cpus.int_mode;
    /* Sync all flags but Carry */
    gpus[i].F=(gpus[i].F&fC)|(cpus.F&~fC);
  }
    
  /* execute instrucion on all GPUs */
  tmps=cpus; /* save CPU for a while */
  rcpus = &tmps;

  tmpbnk[0]=zxbnk[0];
  tmpbnk[1]=zxbnk[1];
  tmpbnk[2]=zxbnk[2];
  tmpbnk[3]=zxbnk[3];

  for(i=0;i<NGP;i++) {
    zxbnk[0]=gfxbnk[i][0];
    zxbnk[1]=gfxbnk[i][1];
    zxbnk[2]=gfxbnk[i][2];
    zxbnk[3]=gfxbnk[i][3];
  
    cpus=gpus[i];
    
    for(j=0;j<GRANU;j++)
      z80_execinstr();
    
    gpus[i]=cpus;
  }
  cpus=tmps; /* restore CPU */
  rcpus=&cpus;
  
  zxbnk[0]=tmpbnk[0];
  zxbnk[1]=tmpbnk[1];
  zxbnk[2]=tmpbnk[2];
  zxbnk[3]=tmpbnk[3];

  z80_clock=tmp_clock;
  
  /* execute on CPU */
  for(j=0;j<GRANU;j++)
    z80_execinstr();
}

/* execute instruction using both CPU and GPU */
void z80_g_int(void) {
  int i;
  unsigned long tmp_clock;
  uint8_t *tmpbnk[4];
 
  tmp_clock=z80_clock;
    
  /* execute int on all GPUs */
  tmps=cpus; /* save CPU for a while */
  
  tmpbnk[0]=zxbnk[0];
  tmpbnk[1]=zxbnk[1];
  tmpbnk[2]=zxbnk[2];
  tmpbnk[3]=zxbnk[3];

  for(i=0;i<NGP;i++) {
    zxbnk[0]=gfxbnk[i][0];
    zxbnk[1]=gfxbnk[i][1];
    zxbnk[2]=gfxbnk[i][2];
    zxbnk[3]=gfxbnk[i][3];
  
    cpus=gpus[i];
    
    z80_int();
    
    gpus[i]=cpus;
  }
  cpus=tmps; /* restore CPU */
  
  zxbnk[0]=tmpbnk[0];
  zxbnk[1]=tmpbnk[1];
  zxbnk[2]=tmpbnk[2];
  zxbnk[3]=tmpbnk[3];

  z80_clock=tmp_clock;
  
  /* execute on CPU */
  z80_int();
}

int gpu_reset(void) {
  int i;
  
  /* set power on defaults */
  z80_reset();
  
  /* store to all gpus */
  for(i=0;i<NGP;i++) gpus[i]=cpus;
  
  return 0;
}
