/*
 * GZX - George's ZX Spectrum Emulator
 * Snapshot loading/saving
 *
 * Copyright (c) 1999-2025 Jiri Svoboda
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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileutil.h"
#include "iospace.h"
#include "memio.h"
#include "strutil.h"
#include "ay.h"
#include "gzx.h"
#include "snap.h"
#include "snap_ay.h"
#include "z80.h"
#include "zx.h"
#include "z80g.h"
#include "zx_scr.h"

static int gfxram_load(char *);

/*
  Translate 48k page numbers (8,4,5) to our numbering system (0,1,2)
*/
static int map48k(int page_n) {
  switch(page_n) {
    case 8: return 0;
    case 4: return 1;
    case 5: return 2;
    default: return -1;
  }
}

/*
  We need to get the CPU to a state representable in others' snapshot
  formats.
*/
static void prepare_cpu(void) {
  if(cpus.modifier) /* DD/FD prefix - go back */
    cpus.PC--;
  /* cpus.halted .. too bad, there's just nothing we can do */
  /* cpus.int_lock .. XXX we should advance to the first instruction
   * that does not enable int_lock. But that could theoretically take
   * a long time. So just forget it. */
}

/* G.A.Lunter's highly emulator-specific snapshot format .Z80 */

static void snap_z80_read_mem_page(FILE *f, uint8_t *b0, unsigned page_bytes) {
  unsigned cnt;
  uint8_t nrep,cbyte;
  uint8_t curb,nxtb;
  int j;
  
  printf("reading page len %d\n",page_bytes);
  cnt=0;
  while(cnt<page_bytes) {
    curb=fgetu8(f);
    nxtb=fgetu8(f);
    if(curb==0xed && nxtb==0xed) { /* compressed */
      nrep=fgetu8(f);
      cbyte=fgetu8(f);
      for(j=0;j<nrep;j++)
        b0[cnt++]=cbyte;
	  
      } else { /* not compressed */
        fungetu8(f,nxtb);
        b0[cnt++]=curb;
      }
  }
}

static void snap_z80_read_48k_page(FILE *f, int page_n) {
  int page_i;
  
  page_i = map48k(page_n);
  if(page_i<0) printf("page type %d - ignoring\n",page_n);
    else snap_z80_read_mem_page(f,zxram+0x4000*page_i,0x4000);
}

static void snap_z80_read_128k_page(FILE *f, int page_n) {
  if(page_n>=3 && page_n<=10)
    snap_z80_read_mem_page(f,zxram+(page_n-3)*0x4000,0x4000);
      else printf("page type %d - ignoring\n",page_n);
}

/* returns 0 when ok, -1 on error -> reset ZX */
int zx_load_snap_z80(char *name) {
  FILE *f;
  uint8_t flags1,flags2,flags3,hw,i1rp;
  uint8_t page,ay_r;
  int compressed;
  uint16_t hdr_len;
  long hdr_end;
  int pages;
  uint8_t page_n;
  uint16_t page_len;
  long page_end;
  int i;
  
  f=fopen(name,"rb");
  if(!f) {
    printf("cannot open snapshot file '%s'\n",name);
    return -1;
  }
  
  zx_reset();
  
  cpus.r[rA]=fgetu8(f);
  cpus.F=fgetu8(f);
  cpus.r[rC]=fgetu8(f);
  cpus.r[rB]=fgetu8(f);
  cpus.r[rL]=fgetu8(f);
  cpus.r[rH]=fgetu8(f);
  cpus.PC=fgetu16le(f);
  cpus.SP=fgetu16le(f);
  cpus.I=fgetu8(f);
  cpus.R=fgetu8(f)&0x7f;
  flags1=fgetu8(f);
  if(flags1==0xff) flags1=0x01; /* Do I deserve this?
				   Did I say anything bad about G.A.Lunter? */
  cpus.R = cpus.R | ((flags1&1)<<7);  /* what the... */
  border=(flags1>>1)&0x07;
  /* bit4 = samrom?! igroring for now.. */
  compressed=(flags1&0x20)!=0;
				   
  cpus.r[rE]=fgetu8(f);
  cpus.r[rD]=fgetu8(f);
  
  cpus.r_[rC]=fgetu8(f);
  cpus.r_[rB]=fgetu8(f);
  cpus.r_[rE]=fgetu8(f);
  cpus.r_[rD]=fgetu8(f);
  cpus.r_[rL]=fgetu8(f);
  cpus.r_[rH]=fgetu8(f);
  cpus.r_[rA]=fgetu8(f);
  cpus.F_=fgetu8(f);
  
  cpus.IY=fgetu16le(f);
  cpus.IX=fgetu16le(f);
  
  cpus.IFF1=fgetu8(f)?1:0;
  cpus.IFF2=fgetu8(f)?1:0;
  
  /* Z80 does not implement or save this */
  cpus.int_lock=0;
  cpus.modifier=0;
  cpus.halted=0;
  
  flags2=fgetu8(f);
  cpus.int_mode=flags2&0x03;
  if(cpus.int_mode==3) {
    printf("error in Z80 snapshot: int_mode==3\n");
    return -1;
  }
  /* other bits of flags2 just make no sense to this emulator... */
  
  if(cpus.PC==0) { /* version >=2.0 */
    hdr_len=fgetu16le(f);
    hdr_end=ftell(f)+hdr_len; /* to handle any possible new version */
    
    cpus.PC=fgetu16le(f);
    hw=fgetu8(f);
    page=fgetu8(f); /* 128k:last out to 7ffd, samram:something else */
    switch(hw) {
      case 0:
      case 1:
        zx_select_memmodel(ZXM_48K);
	pages=3;
	break;
	
      case 2:
        printf("samram not supported\n");
	return -1;

      case 3:
      case 4:
        zx_select_memmodel(ZXM_128K);
	zx_mem_page_select(ZXPLUS_PAGESEL_PORT, page);
	pages=8;
	break;
	
      default:
        printf("unknown hw number\n");
	return -1;
    }
    
    i1rp=fgetu8(f);
    (void) i1rp;
    flags3=fgetu8(f); /* totally useless */
    (void) flags3;
    ay_r=fgetu8(f); /* sound chip register number */
    ay_reg_select(&ay0, ay_r);
    /* contents of sound registers */
    for(i=0;i<16;i++) {
      ay_reg_select(&ay0, i);
      ay_reg_write(&ay0, fgetu8(f));
    }
    ay_reg_select(&ay0, ay_r);
    
    fseek(f,hdr_end,SEEK_SET); /* just to be sure .. */
    
    compressed=1; /* always! */
    printf("Z80 NEW version snapshot\n");
    printf("page data starts at offset %ld\n",ftell(f));
    
    /* new version is always compressed */
    for(i=0;i<pages;i++) {
      page_len=fgetu16le(f);
      page_n=fgetu8(f);
      page_end=ftell(f)+page_len;
      printf("page_n: %d\n",page_n);
      
      switch(hw) {
	case 0:
        case 1:
	  snap_z80_read_48k_page(f,page_n);
          break;
	
        case 3:
        case 4:
	  snap_z80_read_128k_page(f,page_n);
          break;
      }
      fseek(f,page_end,SEEK_SET);
    }
    
  } else {
    printf("Z80 OLD version snapshot\n");
    printf("page data starts at offset %ld\n",ftell(f));
    zx_select_memmodel(ZXM_48K); /* always ZX-48k */
    
    if(compressed) {
      printf("compressed\n");
      snap_z80_read_mem_page(f,zxram,48*1024);
    } else {
      printf("uncompressed\n");
      fread(zxram,1,48*1024,f);
    }
  }
  
  fclose(f);
  return 0;
}

static void z80_write_page_data(FILE *f, uint8_t *b0) {
  unsigned cnt,now;
  uint8_t curb;
  int no_run;
  unsigned bytes_left;
  
  bytes_left = 16384;
  no_run=0;
  
  while(bytes_left>0) {
    
    /* get a run */
    curb=*b0++; cnt=1;
    bytes_left--;
    while(bytes_left>0 && *b0==curb && !no_run) {
      ++cnt; b0++; bytes_left--;
    }
    
    /* put run codes */
    while(cnt>=5 || (curb==0xED && cnt>=2)) {
      if(cnt>255) now=255;
        else now=cnt;
      fputu16le(f,0xEDED);
      fputu8(f,now);
      fputu8(f,curb);
      cnt -= now;
    }
    
    /* first byte after ED is not run-length compressed */
    no_run = curb==0xED && cnt>0;
    
    while(cnt>0) {
      fputu8(f,curb);
      cnt--;
    }
  }
}

static void z80_write_page(FILE *f, int page_i, int page_n) {    
  uint16_t page_len;
  long page_end,page_start;
  
  fputu16le(f,0); /* don't know the length yet */
  fputu8(f,page_n);
  page_start = ftell(f);

  z80_write_page_data(f,zxram+0x4000*page_i);
    
  page_end = ftell(f);
  page_len = page_end-page_start;
    
  /* go back and write the page length */
  fseek(f,page_start-3,SEEK_SET);
  fputu16le(f,page_len);
  fseek(f,page_end,SEEK_SET);
}

/* returns 0 when ok, -1 on error */
static int zx_save_snap_z80(char *name) {
  FILE *f;
  uint8_t flags1,flags2,flags3,hw,i1rp;
  uint16_t hdr_len;
  long hdr_end;
  int i;
  
  switch(mem_model) {
    case ZXM_48K: hw=0; break;
    case ZXM_128K: hw=3; break;
    default: printf("Invalid model for Z80 snapshot.\n"); return -1;
  }

  prepare_cpu();
  
  f=fopen(name,"wb");
  if(!f) {
    printf("cannot open snapshot file '%s'\n",name);
    return -1;
  }
  
  fputu8(f,cpus.r[rA]);
  fputu8(f,cpus.F);
  fputu8(f,cpus.r[rC]);
  fputu8(f,cpus.r[rB]);
  fputu8(f,cpus.r[rL]);
  fputu8(f,cpus.r[rH]);
  fputu16le(f,0); /* would be PC in version < 2.0 of Z80 */
  fputu16le(f,cpus.SP);
  fputu8(f,cpus.I);
  fputu8(f,cpus.R);
  flags1 = (cpus.R>>7)|(border<<1)|0x20;
    /* Samrom not switched in, data is compressed */
  fputu8(f,flags1);

  fputu8(f,cpus.r[rE]);
  fputu8(f,cpus.r[rD]);
  
  fputu8(f,cpus.r_[rC]);
  fputu8(f,cpus.r_[rB]);
  fputu8(f,cpus.r_[rE]);
  fputu8(f,cpus.r_[rD]);
  fputu8(f,cpus.r_[rL]);
  fputu8(f,cpus.r_[rH]);
  fputu8(f,cpus.r_[rA]);
  fputu8(f,cpus.F_);
  
  fputu16le(f,cpus.IY);
  fputu16le(f,cpus.IX);
  
  fputu8(f,cpus.IFF1);
  fputu8(f,cpus.IFF2);
  
  /* Z80 does not implement or save this */
/*  cpus.int_lock=0; better watch out for these!!
  cpus.modifier=0;
  cpus.halted=0;*/
  
  flags2 = cpus.int_mode; /* Normal sync, no double int. freq, no Issue 2 */
  fputu8(f,flags2);
  
  hdr_len=23; /* additional header length in bytes */
  fputu16le(f,hdr_len);
  hdr_end=ftell(f)+hdr_len;
    
  fputu16le(f,cpus.PC);
  
  fputu8(f,hw);
  fputu8(f,page_reg); /* 128k:last out to 7ffd, samram:something else */
    
  i1rp=0x00; /* Interface 1 not paged in */
  fputu8(f,i1rp);
  flags3 = 0x07; /* R-reg & LDIR emulation on, AY always */
  fputu8(f,flags3);
  
  fputu8(f,ay_get_sel_regn(&ay0)); /* sound chip register number */
  
  /* contents of sound registers */
  for(i=0;i<16;i++) {
    fputu8(f,ay_get_reg_contents(&ay0, i));
  }
    
  fseek(f,hdr_end,SEEK_SET); /* just to be sure .. */
    
  printf("save Z80 NEW version snapshot\n");
  printf("page data starts at offset %ld\n",ftell(f));

  switch(mem_model) {
    case ZXM_48K:
      z80_write_page(f,1,4);
      z80_write_page(f,2,5);
      z80_write_page(f,0,8);
      break;
      
    case ZXM_128K:
      for(i=0;i<8;i++)
        z80_write_page(f,i,3+i);
      break;

    default:
      return -1;
  }    
  
  fclose(f);
  return 0;
}


static void snap_sna_read_128k_page(FILE *f, int page_n) {
  fread(zxram+page_n*0x4000,1,0x4000,f);
}

static void snap_sna_write_128k_page(FILE *f, int page_n) {
  fwrite(zxram+page_n*0x4000,1,0x4000,f);
}


/* returns 0 when ok, -1 on error -> reset ZX */
int zx_load_snap_sna(char *name) {
  FILE *f;
  uint8_t inter;
  long size;
  int type;
  uint8_t curpaged;
  uint8_t pageout;
  int i;
  
  f=fopen(name,"rb");
  if(!f) {
    printf("cannot open snapshot file '%s'\n",name);
    return -1;
  }
  
  size=fsize(f);
  switch(size) {
    case 49179: type=0; break;
    case 131103:
    case 147487: type=1; break;
    default:
      printf("SNA has incorrect length\n");
      return -1;
  }
   
  zx_reset();
  
  cpus.I=fgetu8(f);
  
  cpus.r_[rL]=fgetu8(f);
  cpus.r_[rH]=fgetu8(f);
  cpus.r_[rE]=fgetu8(f);
  cpus.r_[rD]=fgetu8(f);
  cpus.r_[rC]=fgetu8(f);
  cpus.r_[rB]=fgetu8(f);
  cpus.F_=fgetu8(f);
  cpus.r_[rA]=fgetu8(f);
  
  cpus.r[rL]=fgetu8(f);
  cpus.r[rH]=fgetu8(f);
  cpus.r[rE]=fgetu8(f);
  cpus.r[rD]=fgetu8(f);
  cpus.r[rC]=fgetu8(f);
  cpus.r[rB]=fgetu8(f);
  cpus.IY=fgetu16le(f);
  cpus.IX=fgetu16le(f);
  
  inter=fgetu8(f);
  
  cpus.IFF2=inter ? 1:0;
  cpus.IFF1=cpus.IFF2;		/* don't know if this is stored anywhere */
  
  cpus.R=fgetu8(f);
  
  cpus.F=fgetu8(f);
  cpus.r[rA]=fgetu8(f);
  cpus.SP=fgetu16le(f);
  
  cpus.int_mode=fgetu8(f);
  if(cpus.int_mode>2) {
    printf("error in SNA snapshot: int_mode>2\n");
    return -1;
  }
  
  border=fgetu8(f)&0x07;
  				   
  /* not supported by SNA */  
  cpus.int_lock=0;
  cpus.modifier=0;
  cpus.halted=0;
 
  if(type==0) {  /* 48k SNA */
    zx_select_memmodel(ZXM_48K);

    /* read memory dump */
    fseek(f,27,SEEK_SET);  
    fread(zxram,1,48*1024,f);
  
    /* pop PC (yuck!)*/
    cpus.PC=zx_memget16(cpus.SP);
    zx_memset16(cpus.SP,0);	/* this is supposed to help sometimes */
    cpus.SP+=2;
  } else { /* 128k SNA */
    zx_select_memmodel(ZXM_128K);
    
    /* read PC and paging info */
    fseek(f,49179,SEEK_SET);
    cpus.PC=fgetu16le(f);
    pageout=fgetu8(f);
    zx_mem_page_select(ZXPLUS_PAGESEL_PORT, pageout);
    fgetu8(f); /* ??? I thought 128k didn't have TR-DOS? */
    
    curpaged=pageout & 0x07;
    
    /* read "48k" banks */
    fseek(f,27,SEEK_SET);
    snap_sna_read_128k_page(f,5);
    snap_sna_read_128k_page(f,2);
    snap_sna_read_128k_page(f,curpaged);
    
    /* read other banks */
    fseek(f,49183,SEEK_SET);
    for(i=0;i<8;i++) {
      if((i!=2) && (i!=5) && (i!=curpaged)) {
        snap_sna_read_128k_page(f,i);
      }
    }
  }

  fclose(f);
 
  return 0;
}


/* returns 0 when ok, -1 on error */
static int zx_save_snap_sna(char *name) {
  FILE *f;
  uint8_t inter;
  uint8_t curpaged;
  int i;
  
  switch(mem_model) {
    case ZXM_48K: break;
    case ZXM_128K: break;
    default: printf("Invalid model for SNA snapshot.\n"); return -1;
  }

  f=fopen(name,"wb");
  if(!f) {
    printf("cannot open snapshot file '%s'\n",name);
    return -1;
  }
  
  prepare_cpu();

  if(mem_model == ZXM_48K) {
    /* ah! the horror! */
    cpus.SP-=2;
    zx_memset16(cpus.SP,cpus.PC);
  }
  
  fputu8(f,cpus.I);
  
  fputu8(f,cpus.r_[rL]);
  fputu8(f,cpus.r_[rH]);
  fputu8(f,cpus.r_[rE]);
  fputu8(f,cpus.r_[rD]);
  fputu8(f,cpus.r_[rC]);
  fputu8(f,cpus.r_[rB]);
  fputu8(f,cpus.F_);
  fputu8(f,cpus.r_[rA]);
  
  fputu8(f,cpus.r[rL]);
  fputu8(f,cpus.r[rH]);
  fputu8(f,cpus.r[rE]);
  fputu8(f,cpus.r[rD]);
  fputu8(f,cpus.r[rC]);
  fputu8(f,cpus.r[rB]);
  fputu16le(f,cpus.IY);
  fputu16le(f,cpus.IX);
  
  /* The docs say IFF2 goes here. But, IFF1 is what's important.
   * Nobody cares aobut IFF2 except the NMI handler! */
  inter=cpus.IFF1 ? 0x04 : 0x00;
  
  fputu8(f,inter);
  
  fputu8(f,cpus.R);
  
  fputu8(f,cpus.F);
  fputu8(f,cpus.r[rA]);
  fputu16le(f,cpus.SP);
  
  fputu8(f,cpus.int_mode);
  
  /* XXX I think this should really be the last byte written to the ULA port */
  fputu8(f,border);
  
  /* filepos: 27 bytes */
  				   
  /* better watch out for these! */
/*  cpus.int_lock;
  cpus.modifier;
  cpus.halted; */
  
  switch(mem_model) {
    case ZXM_48K:      
      /* write memory dump */
      fwrite(zxram,1,48*1024,f);
      
      break;
    case ZXM_128K:
      curpaged = page_reg & 7;
      
      /* write "48k" banks */
      snap_sna_write_128k_page(f,5);
      snap_sna_write_128k_page(f,2);
      snap_sna_write_128k_page(f,curpaged);
        
      /* write other banks */
      for(i=0;i<8;i++) {
        if((i!=2) && (i!=5) && (i!=curpaged)) {
          snap_sna_write_128k_page(f,i);
        }
      }
      
      /* read PC and paging info */
      fputu16le(f,cpus.PC);
      fputu8(f,page_reg);
      fputu8(f,0); /* TR-DOS not paged in */

      break;
    default:
      fprintf(stderr,"Error: Unsupported memory model for SNA!\n");
      break;
  }
  
  if(mem_model == ZXM_48K) {
    /* XXX The idea here is that if the snapshot is broken due to 
     * the stack being clobbered, we'd better find out immediately. */
    zx_memset16(cpus.SP,0);
    cpus.SP+=2;
  }

  fclose(f);
 
  return 0;
}


/* returns 0 when ok, -1 on error -> reset ZX */
/* determines snapshot type by extension */
int zx_load_snap(char *name) {
  char *ext;
  char *gext;
  char *gfxname;
  int i;
  int rc;
  
  ext=strrchr(name,'.');
  if(!ext) {
    printf("filename has no extension\n");
    return -1;
  }
  
  if(!strcmpci(ext,".z80"))
    rc = zx_load_snap_z80(name);
  else if(!strcmpci(ext,".sna"))
    rc = zx_load_snap_sna(name);
  else if(!strcmpci(ext,".ay"))
    rc = zx_load_snap_ay(name);
  else {
    printf("unknown extension\n");
    rc = -1;
  }
  
  if (rc != 0)
    return rc;
  
  if (strcmpci(ext, ".ay") != 0) {
    gfxname = malloc(strlen(name) + 1);
    if (gfxname == NULL)
      return -1;
    memcpy(gfxname, name, strlen(name) + 1);
    gext = strrchr(gfxname, '.');
    assert(gext != 0);
    memcpy(gext + 1, "gfx", strlen("gfx"));

    if (gpu_allow && gfxram_load(gfxname)) {
      memcpy(gext + 1, "GFX", strlen("GFX"));
      if (gfxram_load(gfxname)) {
        free(gfxname);
        return 0;
      }
    }

    zx_scr_clear_bg();

    i = 0;
    while (i < 100) {
      /* Try 'bNN' extension */
      gext[1] = 'b';
      gext[2] = '0' + i / 10;
      gext[3] = '0' + i % 10;

      printf("try loading '%s'\n", gfxname);
      if (zx_scr_load_bg(gfxname, i)) {
        /* Try 'BNN' extension */
        gext[1] = 'B';
        if (zx_scr_load_bg(gfxname, 0)) {
          /* Not found, assuming there are no more backgrounds */
          break;
        }
      }
      ++i;
    }

    free(gfxname);

    { int i;
       for(i=0;i<NGP;i++)
         gpus[i]=cpus;
    }
    printf("Setting screen mode 1\n");
    zx_scr_mode(1);
  }

  return 0;
}

/* returns 0 when ok, -1 on error */
/* determines snapshot type by extension */
int zx_save_snap(char *name) {
  char *ext;
  
  ext=strrchr(name,'.');
  if(!ext) {
    printf("filename has no extension\n");
    return -1;
  }
  
  if(!strcmpci(ext,".z80")) return zx_save_snap_z80(name);
  if(!strcmpci(ext,".sna")) return zx_save_snap_sna(name);
  
  printf("unknown extension\n");
  return -1;
}

static int gfxram_load(char *fname) {
  FILE *f;
  unsigned u,v,w;
  uint8_t buf[8];
  uint8_t b;

  f=fopen(fname,"rb");
  if(!f) {
    printf("gfxram_load: cannot open file '%s'\n",fname);
    return -1;
  }

  if (gpu_enable() < 0) {
    fclose(f);
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
