/*
  GZX - George's ZX Spectrum emulator
*/

#define TAP_NAME1 "/mnt/dos/ghost128.z80"
#define TAP_NAME2 "/mnt/dos/jetpac.tap"
#define TAP_NAME3 "/mnt/dos/stunt128.tzx"
#define TAP_NAME4 "/mnt/dos/totalecl.tap"
#define SNAP_CRASH "/mnt/dos/spcrash.z80"
#undef LOG

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "intdef.h"
#include "memio.h"
#include "mgfx.h"
#include "gzx.h"
#include "iorec.h"
#include "z80.h"
#include "zx_kbd.h"
#include "zx_scr.h"
#include "snap.h"
#include "zx_sound.h"
#include "zx_tape.h"
#include "ay.h"
#include "menus.h"
#include "debug.h"
#include "z80g.h"
#include "zx.h"
#include "sys_all.h"

/*
  Clock comparison is calculated in unsigned long. It works as long
  as u.long has least 32 bits, and as long
  as the clocks don't diverge by more than 2^31 T-states (=~600s).
  (Much more than what is needed.)
*/
#define CLOCK_LT(a,b) ( (((a)-(b)) >> 31) != 0 )
#define CLOCK_GE(a,b) ( (((a)-(b)) >> 31) == 0 )

/* declarations */
void zx_reset(void);

int gfxram_load(char *fname);

void zx_scr_save(void);
int scr_no=0;

FILE *logfi;

static int fl_frames=0;

int quit=0;
int slow_load=1;

/* even(0) or odd(1) field */
int field_n=0;

/* Start up working directory */
/* ... used as base for finding the ROM files */
char *start_dir;

int key_lalt_held;

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

static void z80_fprintstatus(FILE *logfi) {
  fprintf(logfi,"AF %04x BC %04x DE %04x HL %04x IX %04x PC %04x R%02d iHL%02x\n",
          z80_getAF()&0xffd7,z80_getBC(),z80_getDE(),z80_getHL(),cpus.IX,
	  cpus.PC,cpus.R,zx_memget8(z80_getHL()));
  fprintf(logfi,"AF'%04x BC'%04x DE'%04x HL'%04x IY %04x SP'%04x I%02d\n",
          z80_getAF_()&0xffd7,z80_getBC_(),z80_getDE_(),z80_getHL_(),cpus.IY,
	  cpus.SP,cpus.I);
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
  
  zx_key_state_set(k->key, k->press?1:0);
  
  if (k->press) {
      key_unmod(k);
  }
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

void zx_reset(void) {
#ifdef USE_GPU
  gpu_reset();
#endif
  z80_reset();
  ay_reset(&ay0);
  
  /* select default banks */
  bnk_lock48=0;
  zx_out8(0x7ffd,0x07);
}

static int zx_init(void) {
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
  if(ay_init(&ay0, 125/*d_t_states*/)<0) return -1;
  if(zx_tape_init(79)<0) return -1;
  //if(zx_tape_selectfile("/mnt/dos/jetpac.tap")<0) return -1;

  zx_reset();
  
  disp_t=0;
  snd_t=0;
  tapp_t=0;

  border=7;
  
  return 0;
}

static void writestat_i(int i) {
  int j;
  
  for(j=0;j<64;j++)
    fprintf(logfi,"0x%02x: %10d, %10d, %10d, %10d\n",j*4,
      z80_getstat(i,4*j),  z80_getstat(i,4*j+1),
      z80_getstat(i,4*j+2),z80_getstat(i,4*j+3));
}

static void writestat(void) {
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
  zx_key_state_set(key, press?1:0);
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
	
      /* Next field */
      field_n=!field_n;
      mgfx_selln(1<<(field_n)); /* Enable rendering odd/even lines */
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
    zx_sound_smp(ay_get_sample(&ay0)+(tape_smp?+16:-16));
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
  if (0) z80_fprintstatus(logfi);
  
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
	
	/* Next field */
	field_n=!field_n;
	mgfx_selln(1<<(field_n)); /* Enable rendering odd/even lines */
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
      zx_sound_smp(ay_get_sample(&ay0)+(tape_smp?+16:-16));
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
  
  /* Graphics is closed automatically atexit() */
  
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
