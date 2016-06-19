/*
  GZX - George's ZX Spectrum emulator
*/

#define TAP_NAME1 "/mnt/dos/ghost128.z80"
#define TAP_NAME2 "/mnt/dos/jetpac.tap"
#define TAP_NAME3 "/mnt/dos/stunt128.tzx"
#define TAP_NAME4 "/mnt/dos/totalecl.tap"
#define SNAP_CRASH "/mnt/dos/spcrash.z80"
#undef LOG

//#define USE_GPU

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "mgfx.h"
#include "global.h"
#include "z80.h"
#include "zx_keys.h"
#include "zx_scr.h"
#include "snap.h"
#include "zx_sound.h"
#include "zx_tape.h"
#include "ay.h"
#include "menus.h"
#include "debug.h"
#include "z80g.h"
#include "sys_all.h"

/*
  Clock comparison is calculated in unsigned long. It works as long
  as u.long has least 32 bits, and as long
  as the clocks don't diverge by more than 2^31 T-states (=~600s).
  (Much more than what is needed.)
*/
#define CLOCK_LT(a,b) ( (((a)-(b)) >> 31) != 0 )
#define CLOCK_GE(a,b) ( (((a)-(b)) >> 31) == 0 )

#define KST_SIZE 128

/* declarations */
u8 zx_key_in(u8 pwr);
void zx_reset(void);
int zx_select_memmodel(int model);

int gfxram_load(char *fname);

void zx_scr_save(void);
int scr_no=0;

FILE *logfi;

u8 *zxram,*zxrom; /* whole memory */
u8 *zxbnk[4];	  /* currently switched in banks */
u8 *zxscr;	  /* selected screen bank */
u8 border;
u8 spk,mic,ear;

static int fl_frames=0;

unsigned ram_size,rom_size;
int has_banksw,bnk_lock48;
int mem_model;

u8 page_reg; /* last data written to the page select port */

int quit=0;
int slow_load=1;

/* sudy(0) ci lichy(1) pulsnimek */
int field_n=0;

/* pracovni adresar v okamziku spusteni */
/* ... zaklad pro hledani ROM souboru */
static char *start_dir;

/*
  case-insensitive strcmp
*/
int strcmpci(char *a, char *b) {
  while(*a && *b && tolower(*a)==tolower(*b)) {
    a++; b++;
  }
  return tolower(*a) - tolower(*b);
}

unsigned fgetu8(FILE *f) {
  u8 tmp;
  
  fread(&tmp,sizeof(tmp),1,f);
    
  return tmp;
}

void fungetu8(FILE *f, u8 c) {
  ungetc(c,f);
}

unsigned fgetu16le(FILE *f) {
  unsigned tmp;
    
  tmp=fgetu8(f);
  tmp=tmp | (fgetu8(f)<<8);
  return tmp;
}

unsigned fgetu16be(FILE *f) {
  unsigned tmp;
    
  tmp=fgetu8(f);
  tmp=fgetu8(f) | (tmp<<8);
  return tmp;
}

unsigned fgetu24le(FILE *f) {
  unsigned tmp;
    
  tmp=fgetu8(f);
  tmp=tmp | (fgetu8(f)<<8);
  tmp=tmp | (fgetu8(f)<<16);
  return tmp;
}

unsigned fgetu32le(FILE *f) {
  unsigned tmp;
    
  tmp=fgetu8(f);
  tmp=tmp | (fgetu8(f)<<8);
  tmp=tmp | (fgetu8(f)<<16);
  tmp=tmp | (fgetu8(f)<<24);
  return tmp;
}

unsigned fgetu32be(FILE *f) {
  unsigned tmp;
    
  tmp=fgetu8(f);
  tmp=fgetu8(f) | (tmp<<8);
  tmp=fgetu8(f) | (tmp<<8);
  tmp=fgetu8(f) | (tmp<<8);
  return tmp;
}

signed fgets16le(FILE *f) {
  signed tmp;
    
  tmp=fgetu8(f);
  tmp=tmp | (fgetu8(f)<<8);
  if(tmp&0x8000) tmp=(tmp&0x7fff)-0x8000;
  return tmp;
}

void fputu8(FILE *f, u8 val) {
  fwrite(&val,sizeof(val),1,f);
}

void fputu16le(FILE *f, u16 val) {
  fputu8(f,val&0xff);
  fputu8(f,val>>8);
}

void fputu16be(FILE *f, u16 val) {
  fputu8(f,val>>8);
  fputu8(f,val&0xff);
}

unsigned long fsize(FILE *f) {
  unsigned long oldpos,flen;
  
  oldpos=ftell(f);
  fseek(f, 0, SEEK_END);
  flen=ftell(f);
  fseek(f, oldpos, SEEK_SET);
  
  return flen;
}

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

/******* execution map **********/

//#define XMAP
#ifdef XMAP

static u8 xmap[8*1024];

/* execution map */
void xmap_clear(void) {
  unsigned u;
  
  for(u=0;u<8*1024;u++)
    xmap[u]=0;
}

void xmap_mark(void) {
  u8 mask;
  unsigned offs;

  mask = 1<<(cpus.PC & 7);
  offs = cpus.PC >> 3;
  xmap[offs] = xmap[offs] | mask;
}

void xmap_save(void) {
  FILE *f;
  unsigned u,v;
  
  f=fopen("xmap.txt","wt");
  for(u=0;u<8*1024;u++) {
    fprintf(f,"%04X ",u*8);
    for(v=0;v<8;v++) {
      fputc((xmap[u]&(1<<v)) ? '1':'0',f);
      fputc(v<7 ? ',' : '\n',f);
    }
  }
  fclose(f);
}

#endif


/************ the keyboard ******************/

u8 zx_keymap[8];
int key_state[KST_SIZE];
u8 key_mask[KST_SIZE*8];
int key_lalt_held;

/* returns the 6 keyboard bits */
u8 zx_key_in(u8 pwr) { /* power mask */
  u8 res;
  int i;
  /* don't simulate matrix behaviour for now.. */
  res=0x00;
  
  for(i=0;i<8;i++)
    if((pwr & (0x01<<i))==0) res |= zx_keymap[i];

  return res^0x1f;
}

/* figure out which crossings are connected */
void zx_keys_recalc(void) {
  int i,j;
  
  for(i=0;i<8;i++) zx_keymap[i]=0x00;
  for(i=0;i<KST_SIZE;i++) {
    if(key_state[i]) {
      for(j=0;j<8;j++) {
        zx_keymap[j] |= key_mask[8*i+j];
      }
    }
  }
}

static void key_unmod(wkey_t *k)
{
   switch(k->key) {
      case WKEY_ESC:
	main_menu();
	break;
	
      case WKEY_F1:
//        zx_load_snap("manicmin.sna");
#ifdef USE_GPU
        zx_load_snap("knlore.sna");
	gfxram_load("out.gfx");
//	gfxram_load("mmclr.gfx");
//        zx_load_snap("jetpac.sna");
//	gfxram_load("jetpac.gfx");
//	zx_load_snap("jetpac.z80");
//	gfxram_load("jetpac.gfx");
//	zx_load_snap("underw.z80");
//	gfxram_load("underw.gfx");
	
	{ int i;
	  for(i=0;i<NGP;i++)
	    gpus[i]=cpus;
	}
        zx_scr_mode(1);
#else
	zx_load_snap("manicmin.sna");
	debugger();
#endif
	break;
      case WKEY_F2: save_snap_dialog(); break;
      // case WKEY_F2: zx_scr_save(); break;
//      case WKEY_F2: zx_tape_selectfile(TAP_NAME2); break;
      case WKEY_F3: load_snap_dialog(); break;
//      case WKEY_F4: zx_tape_selectfile(TAP_NAME4); break;
//      case WKEY_F5: z80_nmi(); break;
      case WKEY_F5: tape_menu(); break;
//      case WKEY_F6: zx_reset(); break;
      case WKEY_F6: zx_select_memmodel(ZXM_ZX81); zx_reset(); break;
      case WKEY_F7: zx_select_memmodel(ZXM_48K); zx_reset(); break;
      case WKEY_F8: zx_select_memmodel(ZXM_128K); zx_reset(); break;
//      case WKEY_F9: select_tapefile_dialog(); break;
      case WKEY_F10: printf("F10 pressed\n"); quit=1; break;
      case WKEY_F11: zx_scr_mode(0); break;
//      case WKEY_F12: zx_scr_mode(1); break;
      case WKEY_NPLUS: zx_tape_play(); break;
      case WKEY_NMINUS: zx_tape_stop(); break;
      case WKEY_NSTAR: zx_tape_rewind(); break;
      case WKEY_NSLASH: slow_load=!slow_load; break;
#ifdef XMAP
      case WKEY_N5: xmap_clear(); break;
#endif
      case WKEY_F12: debugger(); break;
      default: break;
    }
}

static void key_lalt(wkey_t *k)
{
   switch(k->key) {
      case WKEY_R:
	iorec_enable();
	break;
      case WKEY_T:
	iorec_disable();
	break;
    }
}

static void key_handler(wkey_t *k) {
  
  if (k->key == WKEY_LALT) {
    key_lalt_held = k->press;
    return;
  }
  
  if (key_lalt_held && k->press) {
      key_lalt(k);
      return;
  }
  
  if(k->key>=KST_SIZE) {
    printf("warning. got a key with a too high scancode (>=KST_SIZE)\n");
    printf("ignoring key\n");
    return;
  }
  
  key_state[k->key]=k->press?1:0;
  zx_keys_recalc();
  
  if (k->press) {
      key_unmod(k);
  }
}

void zx_key_register(int key, u16 m0,u16 m1,u16 m2,u16 m3,
                              u16 m4,u16 m5,u16 m6,u16 m7) {
  if(key>=KST_SIZE) {
    printf("error: key cannot be registered - scancode too high. enlarge KST_SIZE\n");
    return;
  }
  key_mask[8*key + 0]=m0;
  key_mask[8*key + 1]=m1;
  key_mask[8*key + 2]=m2;
  key_mask[8*key + 3]=m3;
  key_mask[8*key + 4]=m4;
  key_mask[8*key + 5]=m5;
  key_mask[8*key + 6]=m6;
  key_mask[8*key + 7]=m7;
}

int zx_keys_init(void) {
  int i;
  
  for(i=0;i<KST_SIZE;i++) key_state[i]=0;
  for(i=0;i<KST_SIZE*8;i++) key_mask[i]=0;
  
  /* create a zx-mask for each key */
  /* ZX48-like mapping */
  zx_key_register(WKEY_V,	ZX_KEY_V,0,0,0,0,0,0,0);
  zx_key_register(WKEY_C,	ZX_KEY_C,0,0,0,0,0,0,0);
  zx_key_register(WKEY_X,	ZX_KEY_X,0,0,0,0,0,0,0);
  zx_key_register(WKEY_Z,	ZX_KEY_Z,0,0,0,0,0,0,0);
  zx_key_register(WKEY_LSHIFT,	ZX_KEY_CS,0,0,0,0,0,0,0);
  
  zx_key_register(WKEY_G,	0,ZX_KEY_G,0,0,0,0,0,0);
  zx_key_register(WKEY_F,	0,ZX_KEY_F,0,0,0,0,0,0);
  zx_key_register(WKEY_D,	0,ZX_KEY_D,0,0,0,0,0,0);
  zx_key_register(WKEY_S,	0,ZX_KEY_S,0,0,0,0,0,0);
  zx_key_register(WKEY_A,	0,ZX_KEY_A,0,0,0,0,0,0);

  zx_key_register(WKEY_T,	0,0,ZX_KEY_T,0,0,0,0,0);
  zx_key_register(WKEY_R,	0,0,ZX_KEY_R,0,0,0,0,0);
  zx_key_register(WKEY_E,	0,0,ZX_KEY_E,0,0,0,0,0);
  zx_key_register(WKEY_W,	0,0,ZX_KEY_W,0,0,0,0,0);
  zx_key_register(WKEY_Q,	0,0,ZX_KEY_Q,0,0,0,0,0);
  
  zx_key_register(WKEY_5,	0,0,0,ZX_KEY_5,0,0,0,0);
  zx_key_register(WKEY_4,	0,0,0,ZX_KEY_4,0,0,0,0);
  zx_key_register(WKEY_3,	0,0,0,ZX_KEY_3,0,0,0,0);
  zx_key_register(WKEY_2,	0,0,0,ZX_KEY_2,0,0,0,0);
  zx_key_register(WKEY_1,	0,0,0,ZX_KEY_1,0,0,0,0);
  
  zx_key_register(WKEY_6,	0,0,0,0,ZX_KEY_6,0,0,0);
  zx_key_register(WKEY_7,	0,0,0,0,ZX_KEY_7,0,0,0);
  zx_key_register(WKEY_8,	0,0,0,0,ZX_KEY_8,0,0,0);
  zx_key_register(WKEY_9,	0,0,0,0,ZX_KEY_9,0,0,0);
  zx_key_register(WKEY_0,	0,0,0,0,ZX_KEY_0,0,0,0);
  
  zx_key_register(WKEY_Y,	0,0,0,0,0,ZX_KEY_Y,0,0);
  zx_key_register(WKEY_U,	0,0,0,0,0,ZX_KEY_U,0,0);
  zx_key_register(WKEY_I,	0,0,0,0,0,ZX_KEY_I,0,0);
  zx_key_register(WKEY_O,	0,0,0,0,0,ZX_KEY_O,0,0);
  zx_key_register(WKEY_P,	0,0,0,0,0,ZX_KEY_P,0,0);
  
  zx_key_register(WKEY_H,	0,0,0,0,0,0,ZX_KEY_H,0);
  zx_key_register(WKEY_J,	0,0,0,0,0,0,ZX_KEY_J,0);
  zx_key_register(WKEY_K,	0,0,0,0,0,0,ZX_KEY_K,0);
  zx_key_register(WKEY_L,	0,0,0,0,0,0,ZX_KEY_L,0);
  zx_key_register(WKEY_ENTER,	0,0,0,0,0,0,ZX_KEY_ENT,0);
  
  zx_key_register(WKEY_B,	0,0,0,0,0,0,0,ZX_KEY_B);
  zx_key_register(WKEY_N,	0,0,0,0,0,0,0,ZX_KEY_N);
  zx_key_register(WKEY_M,	0,0,0,0,0,0,0,ZX_KEY_M);
  zx_key_register(WKEY_RSHIFT,	0,0,0,0,0,0,0,ZX_KEY_SS);
  zx_key_register(WKEY_SPACE,	0,0,0,0,0,0,0,ZX_KEY_SP);
 
  /* some more keys */
  zx_key_register(WKEY_BS,	ZX_KEY_CS,0,0,0,ZX_KEY_0,0,0,0);
  zx_key_register(WKEY_LEFT,	0,0,0,ZX_KEY_5,0,0,0,0);
  zx_key_register(WKEY_DOWN,	0,0,0,0,ZX_KEY_6,0,0,0);
  zx_key_register(WKEY_UP,	0,0,0,0,ZX_KEY_7,0,0,0);
  zx_key_register(WKEY_RIGHT,	0,0,0,0,ZX_KEY_8,0,0,0);
  zx_key_register(WKEY_N0,	0,0,0,0,ZX_KEY_0,0,0,0);
  zx_key_register(WKEY_LCTRL,   0,0,0,0,0,0,0,ZX_KEY_SS);

  return 0;
}

void zx_scr_save(void) {
  FILE *f;
  char name[32];

//  snprintf(name,31,"scr%04d.bin",scr_no++);
  sprintf(name,"scr%04d.bin",scr_no++);
  f=fopen(name,"wb");
  fwrite(zxscr,1,0x1B00,f);
  fclose(f);
}

static unsigned long snd_t,tapp_t;

int rom_load(char *fname, int bank, int banksize) {
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

int spec_rom_load(char *fname, int bank) {
  return rom_load(fname,bank,0x4000);
}

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

void zx_reset(void) {
  gpu_reset();
  z80_reset();
  ay_reset();
  
  /* select default banks */
  bnk_lock48=0;
  zx_out8(0x7ffd,0x07);
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

//#include <alloc.h>

int zx_init(void) {
#ifdef USE_GPU
  int i;
#endif

//  printf("coreleft:%lu\n",coreleft());

  z80_init_tables();

  /* important! otherwise zx_select_memmodel would crash reallocing */
  zxrom=NULL;
  zxram=NULL;
  zx_select_memmodel(ZXM_48K);

#ifdef USE_GPU  
  for(i=0;i<NGP;i++) {
    gfxrom[i]=NULL;
    gfxram[i]=NULL;
  }
  gfx_select_memmodel(ZXM_48K);
  gfxrom_load("roms/rom0.gfx",0);
#endif  
  printf("load font\n");
  gloadfont("font.bin");
  printf("init screen\n");
  if(zx_scr_init()<0) return -1;
  if(zx_keys_init()<0) return -1;
  printf("sound\n");
  if(zx_sound_init()<0) return -1;
  printf("ay\n");
  if(ay_init(125/*d_t_states*/)<0) return -1;
  if(zx_tape_init(79)<0) return -1;
  //if(zx_tape_selectfile("/mnt/dos/jetpac.tap")<0) return -1;

  zx_reset();
  
  disp_t=0;
  snd_t=0;
  tapp_t=0;

  border=7;
  
  return 0;
}

void writestat_i(int i) {
  int j;
  
  for(j=0;j<64;j++)
    fprintf(logfi,"0x%02x: %10d, %10d, %10d, %10d\n",j*4,
      stat_tab[i][4*j],  stat_tab[i][4*j+1],
      stat_tab[i][4*j+2],stat_tab[i][4*j+3]);
}

void writestat(void) {
  fprintf(logfi,"\nop:\n");     writestat_i(0);
  fprintf(logfi,"\nDDop:\n");   writestat_i(1);
  fprintf(logfi,"\nFDop:\n");   writestat_i(2);
  fprintf(logfi,"\nCBop:\n");   writestat_i(3);
  fprintf(logfi,"\nDDCBop:\n"); writestat_i(4);
  fprintf(logfi,"\nFDCBop:\n"); writestat_i(5);
  fprintf(logfi,"\nEDop:\n");   writestat_i(6);
}

/* Update Spectrum keyboard state in debug mode */
void zx_debug_key(int press, int key) {
  
  if(key>=KST_SIZE) {
    printf("warning. got a key with a too high scancode (>=KST_SIZE)\n");
    printf("ignoring key\n");
    return;
  }
  key_state[key]=press?1:0;
  zx_keys_recalc();
}

/* Machine step for the debugger */
void zx_debug_mstep(void) {
  u8 tape_smp;
  int frmno;

  if(CLOCK_GE(z80_clock-disp_t,70000)) { /* every 50th of a second */
    disp_t+=70000;
      
    frmno=1;
    if(frmno>=1) {
//      unsigned long twstart;
      disp_cbase=disp_t;
      disp_clock=0;
	
        /* sync with time */
/*	twstart = timer_val(&frmt);
        while(CLOCK_LT(timer_val(&frmt), z80_clock)
	   && CLOCK_LT(timer_val(&frmt),twstart+70000)) {
	  printf("sync with time\n");*/
	  /* dulezite pro zpracovani udalosti woutproc */
/*          mgfx_input_update();

          usleep(1000);
        }*/
	
	/* this is not correct because
           frame displaying takes some time */	   
//        zx_scr_disp_fast();	    
#ifdef USE_GPU
      zx_scr_disp_fast();
#endif

      mgfx_updscr();
	
      /* odted se kresli jiny pulsnimek */
      field_n=!field_n;
      mgfx_selln(1<<(field_n)); /* kresli sude/liche */
      frmno=0;
    }
    frmno++;
    fl_frames++;
    if(fl_frames>=16) {
      fl_frames=0;
      fl_rev=!fl_rev;
    }
#ifdef LOG
    if(cpus.iff1) fprintf(logfi,"interrupt\n");
#endif
//    if(cpus.IFF1) printf("interrupt\n");
    z80_int(0xff);
#ifdef USE_GPU
    z80_g_int(0xff);
#endif
  }
    
#ifndef USE_GPU
  while(CLOCK_LT(disp_cbase+disp_clock,z80_clock))
    zx_scr_disp();
#endif      
    
  if(CLOCK_GE(z80_clock-snd_t,125)) { 
    zx_sound_smp(ay_get_sample()+(tape_smp?+16:-16));
    /* build a new sound sample */
    snd_t+=125;
  }
  if(CLOCK_GE(z80_clock-tapp_t,79/*109*/)) { 
    zx_tape_getsmp(&tape_smp);
    ear=tape_smp;
    tapp_t+=79/*109*/;
  }
  if(!slow_load) {
    if(cpus.PC==ZX_LDBYTES_TRAP) {
      printf("load trapped!\n");
      zx_tape_ldbytes();
    }
    if(cpus.PC==ZX_SABYTES_TRAP) {
      printf("save trapped!\n");
      zx_tape_sabytes();
    }
  }
#ifdef XMAP
  xmap_mark();
#endif

#ifdef USE_GPU    
  z80_g_execinstr();
#else
  z80_execinstr();
#endif
}


extern int dln_odd;


int main(int argc, char **argv) {
  int ic;
  int frmno=0;
  int argi;
  timer frmt;
  u8 tape_smp;
  wkey_t k;
  
  //printf("start\n");
  argi = 1;
  
  dbl_ln=0;
  if(argc > argi && !strcmp(argv[argi],"d")) {
    dbl_ln=1;
    argi++;
  }

  uoc=0;
  smc=0;
  
  field_n=0;
 
  logfi=fopen("log.txt","wt");
  
  start_dir = sys_getcwd(NULL,0);
  
  printf("\n\n\n");
  if(zx_init()<0) return -1;
/*  slow_load=1;*/
  /*if(zx_load_snap(SNAP_NAME1)<0) {
    printf("error loading snapshot\n");
    return -1;
  }*/

  if(argc > argi && zx_load_snap(argv[argi])<0) {
    printf("error loading snapshot\n");
    return -1;
  }

  ic=0;
  //printf("inited.\n");
  //fprintf(logfi,"%d: pc=0x%04x, clock=%ld\n",ic,cpus.PC,z80_clock);
  //z80_fprintstatus(logfi);
  
  timer_reset(&frmt);
  
  while(!quit) {

    if(CLOCK_GE(z80_clock-disp_t,70000)) { /* every 50th of a second */
      disp_t+=70000;
      
      if(frmno>=1) {
//        unsigned long twstart;
        disp_cbase=disp_t;
        disp_clock=0;
	
        /* sync with time */
/*	twstart = timer_val(&frmt);
        while(CLOCK_LT(timer_val(&frmt), z80_clock)
	   && CLOCK_LT(timer_val(&frmt),twstart+70000)) {
	  printf("sync with time\n");*/
	  /* dulezite pro zpracovani udalosti woutproc */
/*          mgfx_input_update();

          usleep(1000);
        }*/
	
	/* this is not correct because
           frame displaying takes some time */	   
//        zx_scr_disp_fast();	    
#ifdef USE_GPU
        zx_scr_disp_fast();
#endif

	mgfx_updscr();
	
	/* odted se kresli jiny pulsnimek */
	field_n=!field_n;
	mgfx_selln(1<<(field_n)); /* kresli sude/liche */
	frmno=0;
      }
      frmno++;
      fl_frames++;
      if(fl_frames>=16) {
        fl_frames=0;
	fl_rev=!fl_rev;
      }
      mgfx_input_update();
      while(w_getkey(&k)) key_handler(&k);
#ifdef LOG
      if(cpus.iff1) fprintf(logfi,"interrupt\n");
#endif
//      if(cpus.IFF1) printf("interrupt\n");
      z80_int(0xff);
#ifdef USE_GPU
      z80_g_int(0xff);
#endif
    }
    
#ifndef USE_GPU
    while(CLOCK_LT(disp_cbase+disp_clock,z80_clock))
      zx_scr_disp();
#endif      
    
    if(CLOCK_GE(z80_clock-snd_t,125)) { 
//     putchar('S');
      zx_sound_smp(ay_get_sample()+(tape_smp?+16:-16));
      /* build a new sound sample */
      snd_t+=125;
//     fputs("~S",stdout);
    }
    if(CLOCK_GE(z80_clock-tapp_t,79/*109*/)) { 
//      putchar('T');
      zx_tape_getsmp(&tape_smp);
      ear=tape_smp;
      tapp_t+=79/*109*/;
    }
    ic++;
    if(!slow_load) {
      if(cpus.PC==ZX_LDBYTES_TRAP) {
        printf("load trapped!\n");
	zx_tape_ldbytes();
      }
      if(cpus.PC==ZX_SABYTES_TRAP) {
        printf("save trapped!\n");
	zx_tape_sabytes();
      }
    }
#ifdef XMAP
    xmap_mark();
#endif

#ifdef USE_GPU    
    z80_g_execinstr();
#else
    z80_execinstr();
#endif
  }
  
  /* uzavreni grafiky je atexit */
  
#ifdef XMAP
  xmap_save();
#endif
  
  zx_sound_done();
  zx_tape_done();

  writestat();  
  fclose(logfi);
  printf("uoc:%lu\nsmc:%lu\n",uoc,smc);
  return 0;
}
