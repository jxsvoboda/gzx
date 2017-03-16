/*
 * Memory and I/O port access
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ay.h"
#include "global.h"
#include "iorec.h"
#include "memio.h"
#include "sys_all.h"
#include "z80.h"
#include "z80g.h"
#include "zx_kbd.h"

u8 *zxram,*zxrom; /* whole memory */
u8 *zxbnk[4];	  /* currently switched in banks */
u8 *zxscr;	  /* selected screen bank */
u8 border;
u8 spk,mic,ear;

unsigned ram_size,rom_size;
int has_banksw,bnk_lock48;
int mem_model;

u8 page_reg; /* last data written to the page select port */

static int rom_load(char *fname, int bank, int banksize);
static int spec_rom_load(char *fname, int bank);

/*
  memory access routines
  any wraps as MMIOs should be placed here
*/
u8 zx_memget8(u16 addr) {
  if(mem_model==ZXM_ZX81) {
    if(addr<=0x7fff)
      return zxbnk[addr>>14][addr&0x1fff];
    else return 0xff;
  } else
    return zxbnk[addr>>14][addr&0x3fff];
}

void zx_memset8(u16 addr, u8 val) {
  if(mem_model==ZXM_ZX81) {
    if(addr>=8192 && addr<=0x7fff) zxbnk[addr>>14][addr&0x1fff]=val;
  } else {
    if(addr>=16384) zxbnk[addr>>14][addr&0x3fff]=val;
      else {
  //      printf("%4x: memory protecion error, write to 0x%04x\n",
  //             cpus.PC, addr);
      }
  }
}

/** Write byte without ROM protection */
void zx_memset8f(u16 addr, u8 val) {
  zxbnk[addr >> 14][addr & 0x3fff] = val;
}

u16 zx_memget16(u16 addr) {
  return (u16)zx_memget8(addr)+(((u16)zx_memget8(addr+1))<<8);
}

void zx_memset16(u16 addr, u16 val) {
  zx_memset8(addr, val & 0xff);
  zx_memset8(addr+1, val >> 8);
}

void zx_mem_page_select(u8 val) {
  page_reg = val; /* needed for snapshot saving */
  
  zxbnk[3]=zxram + ((u32)(val&0x07)<<14);        /* RAM select */
  zxbnk[0]=zxrom + ((val&0x10)?0x4000:0);        /* ROM select */
  zxscr   =zxram + ((val&0x08)?0x1c000:0x14000); /* screen select */
//  printf("bnk select 0x%02x: ram=%d,rom=%d,scr=%d\n",val,val&7,val&0x10,val&0x08);
  if(val&0x20) { /* 48k lock */
    bnk_lock48=1;
    /* co ted? */
    printf("48k lock - not implemented!\n");
  }
}

/*********************************/
/* 
  IO routines
  all wraps should be placed here
*/

u8 zx_in8(u16 a) {
  u8 res;

//  printf("in 0x%04x\n",a);
//  z80_printstatus();
//  getchar();
  if(a==AY_REG_READ_PORT) return ay_reg_read();
  if(a==0x7ffd) printf("bnk sw port read!!!!!!\n");
  switch(a&0xff) {
    /* ULA */
    case 0xfe: res=zx_key_in(a>>8) | 0xa0 | (ear?0x40:0x00); break;
    
    default:  // printf("in 0x%04x\n (no device there)",a);
//               res=0xff;          /* no device attached -> idle bus */
	       //res=0x00;
	       res=0xff;
               break;
  }

  return res;
}

void zx_out8(u16 addr, u8 val) {
//  printf("out (0x%04x),0x%02x\n",addr,val);
  iorec_out(z80_clock, addr, val);
  val=val;
  if((addr&0xff)==0xfe) {  /* the ULA (border/speaker/mic) */
    border=val&7;
    spk=(val&0x10)==0;
    mic=(val&0x18)==0;
//    printf("border %d, spk:%d, mic:%d\n",border,(val>>4)&1,(val>>3)&1);
//    z80_printstatus();
//    getchar();
  } else if(addr==0x7ffd && has_banksw && !bnk_lock48)
    zx_mem_page_select(val);
  else if(addr==AY_REG_WRITE_PORT) {
    ay_reg_write(val);
  } if(addr==AY_REG_SEL_PORT) {
    ay_reg_select(val);
  } else {
//    printf("out (0x%04x),0x%02x (no device there)\n",addr,val);
  }
  /* no device attached */
}

int zx_select_memmodel(int model) {
  int i;
  char *cur_dir;
  
  mem_model=model;
  switch(model) {
    case ZXM_48K:
      ram_size=48*1024; /* 48K spectrum */
      rom_size=16*1024;
      has_banksw=0;
      break;
      
    case ZXM_128K:
      ram_size=128*1024; /* 128K spectrum */
      rom_size=32*1024;
      has_banksw=1;
      break;
      
    case ZXM_PLUS2:
      ram_size=128*1024; /*  spectrum +2 */
      rom_size=32*1024;
      has_banksw=1;
      break;
      
    case ZXM_PLUS3:
      ram_size=128*1024; /*  spectrum +3 */
      rom_size=64*1024;
      has_banksw=1;
      break;
      
    case ZXM_ZX81:
      ram_size=24*1024; /*  ZX81 */
      rom_size=16*1024;
      has_banksw=0;
      break;
  }
  
  /* reallocate memory */
  zxram=realloc(zxram,ram_size);
  zxrom=realloc(zxrom,rom_size);
  if(!zxram || !zxrom) {
    printf("malloc failed\n");
    return -1;
  }
  
  /* fill RAM with random stuff */
  srand(time(NULL));
  for(i=0;i<ram_size;i++)
    zxram[i]=rand();
    
  /* load ROM */
  cur_dir = sys_getcwd(NULL,0);
  if(start_dir) sys_chdir(start_dir);

  switch(mem_model) {
    case ZXM_48K:
      if(spec_rom_load("roms/zx48.rom",0)<0) return -1;
      break;
      
    case ZXM_128K:
      if(spec_rom_load("roms/zx128_0.rom",0)<0) return -1;
      if(spec_rom_load("roms/zx128_1.rom",1)<0) return -1;
      break;
      
    case ZXM_PLUS2:
      if(spec_rom_load("roms/zxp2_0.rom",0)<0) return -1;
      if(spec_rom_load("roms/zxp2_1.rom",1)<0) return -1;
      break;
      
    case ZXM_PLUS3:
      if(spec_rom_load("roms/zxp3_0.rom",0)<0) return -1;
      if(spec_rom_load("roms/zxp3_1.rom",1)<0) return -1;
      break;
      
    case ZXM_ZX81:
      if(rom_load("roms/zx81.rom",0,0x2000)<0) return -1;
      break;
  }
  if(cur_dir) {
    sys_chdir(cur_dir);
    free(cur_dir);
  }
  
  /* setup memory banks */
  switch(mem_model) {
    case ZXM_48K:
      zxbnk[0]=zxrom;
      zxbnk[1]=zxram;
      zxbnk[2]=zxram+16*1024;
      zxbnk[3]=zxram+32*1024;
      zxscr   =zxram;
      break;

    case ZXM_128K:
    case ZXM_PLUS2:
      zxbnk[0]=zxrom;
      zxbnk[1]=zxram+5*0x4000;
      zxbnk[2]=zxram+2*0x4000;
      zxbnk[3]=zxram+7*0x4000;
      zxscr   =zxram+5*0x4000;
      break;
      
    case ZXM_PLUS3:
      zxbnk[0]=zxrom;
      zxbnk[1]=zxram+5*0x4000;
      zxbnk[2]=zxram+2*0x4000;
      zxbnk[3]=zxram+7*0x4000;
      zxscr   =zxram+5*0x4000;
      break;
      
    case ZXM_ZX81: /* 8k pages */
      zxbnk[0]=zxrom;
      zxbnk[1]=zxram;
      zxbnk[2]=zxram+16*1024;
      zxbnk[3]=zxram+32*1024;
      zxscr   =zxram;
      break;
  }

  return 0;
}

static int rom_load(char *fname, int bank, int banksize) {
  FILE *f;

  f=fopen(fname,"rb");
  if(!f) {
    printf("rom_load: cannot open file '%s'\n",fname);
    return -1;
  }
  if(fread(zxrom+(bank*banksize),1,banksize,f)!=banksize) {
    printf("rom_load: unexpected end of file\n");
    return -1;
  }
  fclose(f);
  return 0;
}

static int spec_rom_load(char *fname, int bank) {
  return rom_load(fname,bank,0x4000);
}

#ifdef USE_GPU
int gfxrom_load(char *fname, unsigned bank) {
  FILE *f;
  unsigned u,v,w;
  u8 buf[8];
  u8 b;

  f=fopen(fname,"rb");
  if(!f) {
    printf("gfxrom_load: cannot open file '%s'\n",fname);
    return -1;
  }
  for(u=0;u<16384;u++) {
    fread(buf,1,8,f);
    for(v=0;v<8;v++) {
      b=0;
      for(w=0;w<8;w++) {
        if(buf[w]&(1<<v)) b|=(1<<w);
      }
      gfxrom[v][bank*0x4000 + u]=b;
    }
  }
  fclose(f);
  return 0;
}
#endif

int gfxram_load(char *fname) {
  FILE *f;
  unsigned u,v,w;
  u8 buf[8];
  u8 b;

  f=fopen(fname,"rb");
  if(!f) {
    printf("gfxram_load: cannot open file '%s'\n",fname);
    return -1;
  }
  for(u=0;u<3*16384U;u++) {
    fread(buf,1,8,f);
    for(v=0;v<8;v++) {
      b=0;
      for(w=0;w<8;w++) {
        if(buf[w]&(1<<v)) b|=(1<<w);
      }
      gfxram[v][u]=b;
    }
  }
  fclose(f);
  return 0;
}
