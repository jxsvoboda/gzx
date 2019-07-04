/*
 * GZX - George's ZX Spectrum Emulator
 * Main module
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

#define TAP_NAME1 "/mnt/dos/ghost128.z80"
#define TAP_NAME2 "/mnt/dos/jetpac.tap"
#define TAP_NAME3 "/mnt/dos/stunt128.tzx"
#define TAP_NAME4 "/mnt/dos/totalecl.tap"
#define SNAP_CRASH "/mnt/dos/spcrash.z80"
#undef LOG

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "clock.h"
#include "memio.h"
#include "midi.h"
#include "mgfx.h"
#include "gzx.h"
#include "iorec.h"
#include "z80.h"
#include "zx_kbd.h"
#include "zx_scr.h"
#include "rs232.h"
#include "snap.h"
#include "tape/quick.h"
#include "zx_sound.h"
#include "ay.h"
#include "menus.h"
#include "midi.h"
#include "debug.h"
#include "z80g.h"
#include "zx.h"
#include "sys_all.h"
#include "sysmidi.h"

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

static unsigned long disp_t;

int quit=0;
int slow_load=1;

/** User interface lock */
bool ui_lock = false;

/* even(0) or odd(1) field */
int field_n=0;

/* Start up working directory */
/* ... used as base for finding the ROM files */
char *start_dir;

/** MIDI device specification */
const char *midi_dev;

/** Audio capture file */
const char *acap_file;

/** I/O recording */
iorec_t *iorec;

int key_lalt_held;
int key_lshift_held;

/******* execution map **********/

//#define XMAP
#ifdef XMAP

static uint8_t xmap[8*1024];

/* execution map */
void xmap_clear(void) {
  unsigned u;
  
  for(u=0;u<8*1024;u++)
    xmap[u]=0;
}

void xmap_mark(void) {
  uint8_t mask;
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

/** Lock down user interface */
void gzx_ui_lock(void)
{
	ui_lock = true;
}

/** Notify on 48K mode change.
 *
 * 48K mode is on if we're Spectrum 48K (or lower) or if we are Spectrum
 * 128K or higher locked in 48K emulation mode.
 */
void gzx_notify_mode_48k(bool mode48k)
{
	/* Tape needs to know to handle Stop the tape if in 48K mode */
	if (tape_deck != NULL)
		tape_deck_set_48k(tape_deck, mode48k);
}

static void key_unmod(wkey_t *k)
{
   switch(k->key) {
      case WKEY_ESC:
	main_menu();
	break;
      case WKEY_F1:
	zx_load_snap("test.sna");
	break;
      case WKEY_F2: save_snap_dialog(); break;
      // case WKEY_F2: zx_scr_save(); break;
      case WKEY_F3: load_snap_dialog(); break;
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
      case WKEY_NPLUS: tape_deck_play(tape_deck); break;
      case WKEY_NMINUS: tape_deck_stop(tape_deck); break;
      case WKEY_NSTAR: tape_deck_rewind(tape_deck); break;
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
	if (iorec == NULL)
	  (void) iorec_open("out.ior", &iorec);
	break;
      case WKEY_T:
        if (iorec != NULL) {
	  iorec_close(iorec);
	  iorec = NULL;
	}
	break;
      case WKEY_N:
        zx_scr_prev_bg();
        break;
      case WKEY_M:
        zx_scr_next_bg();
        break;
    }
}

static void key_lalt_lshift(wkey_t *k)
{
   switch(k->key) {
      case WKEY_L:
	ui_lock = true;
	break;
      case WKEY_U:
	ui_lock = false;
	break;
    }
}

static void key_handler(wkey_t *k) {
  
  if (k->key == WKEY_LALT) {
    key_lalt_held = k->press;
    return;
  }

  if (k->key == WKEY_LSHIFT)
    key_lshift_held = k->press;

  if (key_lalt_held && key_lshift_held && k->press) {
      key_lalt_lshift(k);
      return;
  }

  if (key_lalt_held && k->press && !ui_lock) {
      key_lalt(k);
      return;
  }

  if(k->key>=KST_SIZE) {
    printf("warning. got a key with a too high scancode (>=KST_SIZE)\n");
    printf("ignoring key\n");
    return;
  }
  
  zx_key_state_set(&keys, k->key, k->press?1:0);
  
  if (k->press && !ui_lock) {
      key_unmod(k);
  }
}

void zx_scr_save(void) {
  FILE *f;
  char name[32];

//  snprintf(name,31,"scr%04d.bin",scr_no++);
  snprintf(name, 32, "scr%04d.bin",scr_no++);
  f=fopen(name,"wb");
  fwrite(zxscr,1,0x1B00,f);
  fclose(f);
}

/** Value was written to AY I/O port.
 *
 * @param arg Callback argument
 * @param val Value
 */
static void gzx_ay_ioport_write(void *arg, uint8_t val)
{
	rs232_write(&rs232, val);
}

/** Character was sent via RS-232 port */
static void gzx_rs232_sendchar(void *arg, uint8_t val)
{
	midi_port_write(&midi, val);
}

/** Midi event was sent via MIDI port */
static void gzx_midi_msg(void *arg, midi_msg_t *msg)
{
#ifdef WITH_MIDI
	sysmidi_send_msg(z80_clock, msg);
#endif
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
  if(zx_scr_init(0)<0) return -1;
  if(zx_keys_init(&keys)<0) return -1;
  printf("sound\n");
  if(zx_sound_init()<0) return -1;
  if (acap_file != NULL && zx_sound_start_capture(acap_file) < 0) {
	printf("Failed starting audio capture.\n");
	return -1;
  }
#ifdef WITH_MIDI
  if (sysmidi_init(midi_dev)<0) {
	printf("Note: MIDI not available.\n");
  }
#endif

  printf("ay\n");
  if(ay_init(&ay0, ZX_SOUND_TICKS_SMP)<0) return -1;
  ay0.ioport_write = gzx_ay_ioport_write;
  ay0.ioport_write_arg = &ay0;

  rs232_init(&rs232, Z80_CLOCK / MIDI_BAUD);
  rs232.sendchar = gzx_rs232_sendchar;
  rs232.sendchar_arg = &rs232;

  midi_port_init(&midi);
  midi.midi_msg = gzx_midi_msg;
  midi.midi_msg_arg = &midi;

  if(tape_deck_create(&tape_deck, true) != 0) return -1;
  tape_deck->delta_t = ZX_TAPE_TICKS_SMP;

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
  zx_key_state_set(&keys, key, press?1:0);
}

/* Machine step for the debugger */
void zx_debug_mstep(void) {
  uint8_t tape_smp;
  int frmno;

  if(CLOCK_GE(z80_clock-disp_t,ULA_FIELD_TICKS)) { /* every 50th of a second */
    disp_t+=ULA_FIELD_TICKS;
      
    frmno=1;
    if(frmno>=1) {
//      unsigned long twstart;
	
        /* sync with time */
/*	twstart = timer_val(&frmt);
        while(CLOCK_LT(timer_val(&frmt), z80_clock)
	   && CLOCK_LT(timer_val(&frmt),twstart+ULA_FIELD_TICKS)) {
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
  while(CLOCK_LT(zx_scr_get_clock(),z80_clock))
    zx_scr_disp();
#endif      
    
  if(CLOCK_GE(z80_clock-snd_t,ZX_SOUND_TICKS_SMP)) { 
    zx_sound_smp(ay_get_sample(&ay0)+(tape_smp?+16:-16));
    /* build a new sound sample */
    snd_t+=ZX_SOUND_TICKS_SMP;
  }
  if(CLOCK_GE(z80_clock-tapp_t,ZX_TAPE_TICKS_SMP)) {
    tape_deck_getsmp(tape_deck, &tape_smp);
    ear=tape_smp;
    tapp_t+=ZX_TAPE_TICKS_SMP;
  }
  if(!slow_load) {
    if(cpus.PC==TAPE_LDBYTES_TRAP) {
      printf("load trapped!\n");
      tape_quick_ldbytes(tape_deck);
    }
    if(cpus.PC==TAPE_SABYTES_TRAP) {
      printf("save trapped!\n");
      tape_quick_sabytes(tape_deck);
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

#include "tape/tape.h"
#include "tape/tzx.h"
static void tzx_tape_test(void)
{
	tape_t *tape = NULL;
	int rc;

	rc = tzx_tape_load("test.tzx", &tape);
	if (rc != 0) {
		printf("tzx_tape_load -> %d\n", rc);
		exit(1);
	}

	rc = tzx_tape_save(tape, "test-out.tzx");
	if (rc != 0) {
		printf("tzx_tape_save -> %d\n", rc);
		exit(1);
	}

	printf("destroy tape..\n");
	tape_destroy(tape);
	exit(0);
}

#include "tape/tap.h"
static void tap_tape_test(void)
{
	tape_t *tape = NULL;
	int rc;

	rc = tap_tape_load("test.tap", &tape);
	if (rc != 0) {
		printf("tap_tape_load -> %d\n", rc);
		exit(1);
	}

	rc = tap_tape_save(tape, "test-out.tap");
	if (rc != 0) {
		printf("tap_tape_save -> %d\n", rc);
		exit(1);
	}

	printf("destroy tape..\n");
	tape_destroy(tape);
	exit(0);
}

#include "tape/wav.h"
static void wav_tape_test(void)
{
	tape_t *tape = NULL;
	int rc;

	rc = wav_tape_load("test.wav", &tape);
	if (rc != 0) {
		printf("tap_tape_load -> %d\n", rc);
		exit(1);
	}

	rc = wav_tape_save(tape, "test-out.wav");
	if (rc != 0) {
		printf("tap_tape_save -> %d\n", rc);
		exit(1);
	}

	printf("destroy tape..\n");
	tape_destroy(tape);
	exit(0);
}

int main(int argc, char **argv) {
  int ic;
  int frmno=0;
  int argi;
  timer frmt;
  uint8_t tape_smp;
  wkey_t k;

  if (0) tzx_tape_test();
  if (0) tap_tape_test();
  if (0) wav_tape_test();
  
  //printf("start\n");
  argi = 1;
  
  dbl_ln=0;

  while (argc > argi && argv[argi][0] == '-') {
    if (!strcmp(argv[argi],"-dl")) {
      dbl_ln=1;
      argi++;
    } else if (!strcmp(argv[argi],"-midi")) {
	    if (argc <= argi + 1) {
		    printf("Option -midi missing argument.\n");
		    exit(1);
	    }
	    midi_dev = argv[argi + 1];
	    argi+=2;
    } else if (!strcmp(argv[argi],"-acap")) {
	    if (argc <= argi + 1) {
		    printf("Option -acap missing argument.\n");
		    exit(1);
	    }
	    acap_file = argv[argi + 1];
	    argi+=2;
    } else {
	    printf("Invalid option '%s'.\n", argv[argi]);
	    exit(1);
    }
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

    if(CLOCK_GE(z80_clock-disp_t,ULA_FIELD_TICKS)) { /* every 50th of a second */
      disp_t+=ULA_FIELD_TICKS;
      
      if(frmno>=1) {
//        unsigned long twstart;
	
        /* sync with time */
/*	twstart = timer_val(&frmt);
        while(CLOCK_LT(timer_val(&frmt), z80_clock)
	   && CLOCK_LT(timer_val(&frmt),twstart+ULA_FIELD_TICKS)) {
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
#ifdef WITH_MIDI
	sysmidi_poll(z80_clock);
#endif
	mgfx_updscr();
	
	/* Next field */
	field_n=!field_n;
	mgfx_selln(1<<(field_n)); /* Enable rendering odd/even lines */
	frmno=0;
      }
      frmno++;
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
    while(CLOCK_LT(zx_scr_get_clock(),z80_clock))
      zx_scr_disp();
#endif      
    
    if(CLOCK_GE(z80_clock-snd_t,ZX_SOUND_TICKS_SMP)) { 
//     putchar('S');
      zx_sound_smp(ay_get_sample(&ay0)+(tape_smp?+16:-16));
      /* build a new sound sample */
      snd_t+=ZX_SOUND_TICKS_SMP;
//     fputs("~S",stdout);
    }
    if(CLOCK_GE(z80_clock-tapp_t,ZX_TAPE_TICKS_SMP)) {
//      putchar('T');
      tape_deck_getsmp(tape_deck, &tape_smp);
      ear=tape_smp;
      tapp_t+=ZX_TAPE_TICKS_SMP;
    }
    ic++;
    if(!slow_load) {
      if(cpus.PC==TAPE_LDBYTES_TRAP) {
        printf("load trapped!\n");
	tape_quick_ldbytes(tape_deck);
      }
      if(cpus.PC==TAPE_SABYTES_TRAP) {
        printf("save trapped!\n");
	tape_quick_sabytes(tape_deck);
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
  tape_deck_destroy(tape_deck);
  tape_deck = NULL;

  writestat();  
  fclose(logfi);
  printf("uoc:%lu\nsmc:%lu\n",uoc,smc);
  return 0;
}
