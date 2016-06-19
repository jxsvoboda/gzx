/*
  GZX - George's ZX Spectrum Emulator
  snapshot loading/saving
  
  !!!! otestovat zda zapsane SNA a Z80 lze nahrat v jinych emulatorech !!!!
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "z80.h"
#include "global.h"
#include "ay.h"
#include "snap_ay.h"

/*
  prevede cislovani stranek v 48k (8,4,5) na nase
  cislovani (0,1,2)
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
  Musime dostat CPU do stavu, kteremu rozumi
  ostatni emulatory
*/
static void prepare_cpu(void) {
  if(cpus.modifier) /* DD/FD prefix - presun se zpet */
    cpus.PC--;
  /* cpus.halted .. nejde nic delat, proste to neumi */
  /* cpus.int_lock .. zatim na to kasleme,
     chtelo by to prejit k prvni instrukci bez int_lock
     ... to je ale muze byt teoreticky za dlouho */
}

/* G.A.Lunter's highly emulator-specific snapshot format .Z80 */

static void snap_z80_read_mem_page(FILE *f, u8 *b0, unsigned page_bytes) {
  unsigned cnt;
  u8 nrep,cbyte;
  u8 curb,nxtb;
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
  u8 flags1,flags2,flags3,hw,i1rp;
  u8 page,ay_r;
  int compressed;
  int hdr_len,hdr_end;
  int pages;
  int page_len,page_end,page_n;
  int i;
  
  f=fopen(name,"rb");
  if(!f) {
    printf("cannot open snapshot file '%s'\n",name);
    return -1;
  }
  
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
	zx_mem_page_select(page);
	pages=8;
	break;
	
      default:
        printf("unknown hw number\n");
	return -1;
    }
    
    i1rp=fgetu8(f);
    flags3=fgetu8(f); /* totally useless */
    ay_r=fgetu8(f); /* sound chip register number */
    ay_reg_select(ay_r);
    /* contents of sound registers */
    for(i=0;i<16;i++) {
      ay_reg_select(i);
      ay_reg_write(fgetu8(f));
    }
    ay_reg_select(ay_r);
    
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

static void z80_write_page_data(FILE *f, u8 *b0) {
  unsigned cnt,now;
  u8 curb,nxtb;
  int no_run;
  unsigned bytes_left;
  
  bytes_left = 16384;
  no_run=0;
  
  while(bytes_left>0) {
    
    /* get a run */
    curb=*b0++; cnt=1;
    nxtb=*b0; bytes_left--;
    while(nxtb==curb && bytes_left>0 && !no_run) {
      ++cnt; b0++; nxtb=*b0; bytes_left--;
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
  int page_len,page_end,page_start;
  
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
int zx_save_snap_z80(char *name) {
  FILE *f;
  u8 flags1,flags2,flags3,hw,i1rp;
  int hdr_len,hdr_end;
  int i;
  
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
/*  cpus.int_lock=0; na tyto pozor!!
  cpus.modifier=0;
  cpus.halted=0;*/
  
  flags2 = cpus.int_mode; /* Normal sync, no double int. freq, no Issue 2 */
  fputu8(f,flags2);
  
  hdr_len=23; /* additional header length in bytes */
  fputu16le(f,hdr_len);
  hdr_end=ftell(f)+hdr_len;
    
  fputu16le(f,cpus.PC);
  
  switch(mem_model) {
    case ZXM_48K: hw=0; break;
    case ZXM_128K: hw=3; break;
    default: printf("HW mode not supported\n"); return -1;
  }
  fputu8(f,hw);
  fputu8(f,page_reg); /* 128k:last out to 7ffd, samram:something else */
    
  i1rp=0x00; /* Interface 1 not paged in */
  fputu8(f,i1rp);
  flags3 = 0x07; /* R-reg & LDIR emulation on, AY always */
  fputu8(f,flags3);
  
  fputu8(f,ay_cur_reg); /* sound chip register number */
  
  /* contents of sound registers */
  for(i=0;i<16;i++) {
    fputu8(f,ay_reg[i]);
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
static int zx_load_snap_sna(char *name) {
  FILE *f;
  u8 inter;
  unsigned size;
  int type;
  int curpaged;
  unsigned pageout;
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
  cpus.IFF1=cpus.IFF2;		/* nevim, jestli je tam nekde ulozeny */
  
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
  
    /* pop PC (fujtajbl!)*/
    cpus.PC=zx_memget16(cpus.SP);
    zx_memset16(cpus.SP,0);	/* pry to obcas pomuze */
    cpus.SP+=2;
  } else { /* 128k SNA */
    zx_select_memmodel(ZXM_128K);
    
    /* read PC and paging info */
    fseek(f,49179,SEEK_SET);
    cpus.PC=fgetu16le(f);
    pageout=fgetu8(f);
    zx_mem_page_select(pageout);
    fgetu8(f); /* ??? Spectrum 128k prece TR-DOS nema! */
    
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
  u8 inter;
  int curpaged;
  int i;
  
  f=fopen(name,"wb");
  if(!f) {
    printf("cannot open snapshot file '%s'\n",name);
    return -1;
  }
  
  prepare_cpu();

  if(mem_model == ZXM_48K) {
    /* des a hruza */
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
  
  /* podle dokumentace tam ma byt IFF2, jenze CPU hledi na IFF1,
     zatimco IFF2 zajima jen NMI obsluhu! */
  inter=cpus.IFF1 ? 0x04 : 0x00;
  
  fputu8(f,inter);
  
  fputu8(f,cpus.R);
  
  fputu8(f,cpus.F);
  fputu8(f,cpus.r[rA]);
  fputu16le(f,cpus.SP);
  
  fputu8(f,cpus.int_mode);
  
  /* myslim ze spravne to ma byt posledni vystup na prislusny port */
  fputu8(f,border);
  
  /* filepos: 27 bytes */
  				   
  /* na tyto pozor! */
/*  cpus.int_lock;
  cpus.modifier;
  cpus.halted; */
  
  switch(mem_model) {
    case ZXM_48K:      
      /* write memory dump */
      fwrite(zxram,1,48*1024,f);
      
      break;
    case ZXM_128K:
      curpaged = page_reg & 7;;
      
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
    /* uklidime tu spoust... */
    zx_memset16(cpus.SP,0); /* jestli to rozbije snapshot,
      at se ucinky projevi hned! */
    cpus.SP+=2;
  }


  fclose(f);
 
  return 0;
}


/* returns 0 when ok, -1 on error -> reset ZX */
/* determines snapshot type by extension */
int zx_load_snap(char *name) {
  char *ext;
  
  ext=strrchr(name,'.');
  if(!ext) {
    printf("filename has no extension\n");
    return -1;
  }
  
  if(!strcmpci(ext,".z80")) return zx_load_snap_z80(name);
  if(!strcmpci(ext,".sna")) return zx_load_snap_sna(name);
  if(!strcmpci(ext,".ay")) return zx_load_snap_ay(name);
  
  printf("unknown extension\n");
  return -1;
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
