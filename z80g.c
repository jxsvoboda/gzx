/*
  GZX - George's ZX Spectrum Emulator
  Z80 GPU
  Implements graphical co-planes (a la Spec256)
  
  this file is included into z80.c
*/

#include "memio.h"
#include "z80g.h"

z80s gpus[NGP];		/* GPUs */
z80s tmps;		/* temporary place to store the CPU */

u8 *gfxrom[NGP];
u8 *gfxram[NGP];
u8 *gfxscr[NGP];
static u8 *gfxbnk[NGP][4];


/************************************************************************/
/************************************************************************/

void gfx_select_memmodel(int model) {
  int i;
  u8 *tmpram,*tmprom,*tmpscr;
  u8 *tmpbnk[4];
  
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

#define GRANU 16

/* execute instruction using both CPU and GPU */
void z80_g_execinstr(void) {
  int i,j;
  unsigned long tmp_clock;
  u8 *tmpram,*tmprom,*tmpscr;
  u8 *tmpbnk[4];

  tmp_clock=z80_clock;

  /* synchronise GPUs with CPU */
  for(i=0;i<NGP;i++) {
    gpus[i].PC=cpus.PC;
    gpus[i].modifier=cpus.modifier;
    gpus[i].int_lock=cpus.int_lock;
    gpus[i].halted=cpus.halted;
    gpus[i].int_mode=cpus.int_mode;
  }
    
  /* execute instrucion on all GPUs */
  tmps=cpus; /* save CPU for a while */
  
  tmprom=zxrom;
  tmpram=zxram;
  tmpscr=zxscr;
  
  tmpbnk[0]=zxbnk[0];
  tmpbnk[1]=zxbnk[1];
  tmpbnk[2]=zxbnk[2];
  tmpbnk[3]=zxbnk[3];

  for(i=0;i<NGP;i++) {
/*    zxrom=gfxrom[i];
    zxram=gfxram[i];
    zxscr=gfxscr[i];*/
    
    zxbnk[0]=gfxbnk[i][0];
    zxbnk[1]=gfxbnk[i][1];
    zxbnk[2]=gfxbnk[i][2];
    zxbnk[3]=gfxbnk[i][3];
  
    cpus=gpus[i];
    
    for(j=0;j<GRANU;j++)
      z80_execinstr();
    
    gpus[i]=cpus;

/*    gfxrom[i]=zxrom;
    gfxram[i]=zxram;
    gfxscr[i]=zxscr;    */
    
    gfxbnk[i][0]=zxbnk[0];
    gfxbnk[i][1]=zxbnk[1];
    gfxbnk[i][2]=zxbnk[2];
    gfxbnk[i][3]=zxbnk[3];
  }
  cpus=tmps; /* restore CPU */
  
  zxrom=tmprom;
  zxram=tmpram;
  zxscr=tmpscr;
  
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
void z80_g_int(u8 bus) {
  int i;
  unsigned long tmp_clock;
  u8 *tmpram,*tmprom,*tmpscr;
  u8 *tmpbnk[4];
 
  tmp_clock=z80_clock;
    
  /* execute int on all GPUs */
  tmps=cpus; /* save CPU for a while */
  
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
    
    zxbnk[0]=gfxbnk[i][0];
    zxbnk[1]=gfxbnk[i][1];
    zxbnk[2]=gfxbnk[i][2];
    zxbnk[3]=gfxbnk[i][3];
  
    cpus=gpus[i];
    
    z80_int(bus);
    
    gpus[i]=cpus;

    gfxrom[i]=zxrom;
    gfxram[i]=zxram;
    gfxscr[i]=zxscr;    
    
    gfxbnk[i][0]=zxbnk[0];
    gfxbnk[i][1]=zxbnk[1];
    gfxbnk[i][2]=zxbnk[2];
    gfxbnk[i][3]=zxbnk[3];
  }
  cpus=tmps; /* restore CPU */
  
  zxrom=tmprom;
  zxram=tmpram;
  zxscr=tmpscr;
  
  zxbnk[0]=tmpbnk[0];
  zxbnk[1]=tmpbnk[1];
  zxbnk[2]=tmpbnk[2];
  zxbnk[3]=tmpbnk[3];

  z80_clock=tmp_clock;
  
  /* execute on CPU */
  z80_int(bus);
}

int gpu_reset(void) {
  int i;
  
  /* set power on defaults */
  z80_reset();
  
  /* store to all gpus */
  for(i=0;i<NGP;i++) gpus[i]=cpus;
  
  return 0;
}
