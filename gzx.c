/*
 * GZX - George's ZX Spectrum Emulator
 * Main module
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
#include "ui/display.h"
#include "ui/fdlg.h"
#include "ui/hwopts.h"
#include "ui/mainmenu.h"
#include "ui/tapemenu.h"
#include "zx_sound.h"
#include "ay.h"
#include "midi.h"
#include "debug.h"
#include "xmap.h"
#include "xtrace.h"
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

static void zx_scr_save(void);
static void zx_proc_instr(void);

int scr_no=0;

FILE *logfi;

static unsigned long disp_t;

int quit=0;
int slow_load=0;

/** User interface lock */
static bool ui_lock = false;

/* Start up working directory */
/* ... used as base for finding the ROM files */
char *start_dir;

/** MIDI device specification */
const char *midi_dev;

/** I/O recording */
iorec_t *iorec;

int key_lalt_held;
int key_lshift_held;

static uint8_t tape_smp;

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

void gzx_toggle_dbl_ln(void)
{
	mgfx_toggle_dbl_ln();
	zx_scr_disp_fast();
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
      case WKEY_F2:
        save_snap_dialog();
        if (0) zx_scr_save(); // XXX
        break;
      case WKEY_F4: display_menu(); break;
      case WKEY_F3: load_snap_dialog(); break;
      case WKEY_F5: tape_menu(); break;
      case WKEY_F6: hwopts_menu(); break;
      case WKEY_F7: zx_select_memmodel(ZXM_48K); zx_reset(); break;
      case WKEY_F8: zx_select_memmodel(ZXM_128K); zx_reset(); break;
      case WKEY_F9: select_tapefile_dialog(); break;
      case WKEY_F10: printf("F10 pressed\n"); quit=1; break;
      case WKEY_F11: mgfx_toggle_fs(); break;
      case WKEY_NPLUS: tape_deck_play(tape_deck); break;
      case WKEY_NMINUS: tape_deck_stop(tape_deck); break;
      case WKEY_NSTAR: tape_deck_rewind(tape_deck); break;
      case WKEY_NSLASH: slow_load=!slow_load; break;
#ifdef XMAP
      case WKEY_N5: xmap_clear(); break;
#endif
      case WKEY_F12: debugger_run(&dbg); break;
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
      case WKEY_W:
        key_lalt_held = 0;
        rec_audio_dialog();
        break;
      case WKEY_E:
        printf("Stopping audio capture.\n");
        zx_sound_stop_capture();
        break;
      case WKEY_N:
        zx_scr_prev_bg();
        break;
      case WKEY_M:
        zx_scr_next_bg();
        break;
      case WKEY_9:
        zx_scr_mode(0);
        break;
      case WKEY_0:
        zx_scr_mode(1);
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

/** Currently we always map cursor keys to Kempston joystick */
static void key_joy_state_set(int key, int press)
{
	switch (key) {
	case WKEY_UP:
		kempston_joy_set_reset(&kjoy0, kempston_up, press);
		break;
	case WKEY_DOWN:
		kempston_joy_set_reset(&kjoy0, kempston_down, press);
		break;
	case WKEY_LEFT:
		kempston_joy_set_reset(&kjoy0, kempston_left, press);
		break;
	case WKEY_RIGHT:
		kempston_joy_set_reset(&kjoy0, kempston_right, press);
		break;
	case WKEY_INS:
		kempston_joy_set_reset(&kjoy0, kempston_button_1, press);
		break;
	case WKEY_DEL:
		kempston_joy_set_reset(&kjoy0, kempston_button_2, press);
		break;
	case WKEY_HOME:
		kempston_joy_set_reset(&kjoy0, kempston_button_3, press);
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
  key_joy_state_set(k->key, k->press?1:0);
  
  if (k->press && !ui_lock) {
      key_unmod(k);
  }
}

static void zx_scr_save(void) {
  FILE *f;
  char name[32];

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
#ifdef XTRACE
  xtrace_reset();
#endif
  if (gpu_is_on()) {
    gpu_reset();
    gpu_disable();
  }
  zx_scr_reset();
  z80_reset();
  ay_reset(&ay0);
  
  /* select default banks */
  bnk_lock48=0;
  zx_out8(0x7ffd,0x07);
}

static int zx_init(void) {
//  printf("coreleft:%lu\n",coreleft());

  z80_init_tables();

  /* important! otherwise zx_select_memmodel would crash reallocing */
  zxrom=NULL;
  zxram=NULL;
  zx_select_memmodel(ZXM_48K);

  gpu_init();
//  if (gpu_enable() < 0)
//    return -1;
  printf("load font\n");
  gloadfont("font.bin");
  printf("init screen\n");
  if(zx_scr_init(0)<0) return -1;
  if(zx_keys_init(&keys)<0) return -1;
  printf("sound\n");
  if(zx_sound_init()<0) return -1;
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

  kempston_joy_init(&kjoy0);

  if(tape_deck_create(&tape_deck, ZX_TAPE_TICKS_SMP, true) != 0) return -1;

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

/** Process an instruction and anything that we check for every instruction. */
static void zx_proc_instr(void)
{
    if (!gpu_is_on()) {
      while(CLOCK_LT(zx_scr_get_clock(),z80_clock)) {
        zx_scr_disp();
      }
    } else {
      while(CLOCK_LT(zx_scr_get_clock(),z80_clock)) {
        zx_scr_disp_fast();
      }
    }
    
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
    if (dbg.stop_enabled && cpus.PC == dbg.stop_addr) {
      debugger_run(&dbg);
    }
#ifdef XMAP
    xmap_mark();
#endif
#ifdef XTRACE
    xtrace_instr();
#endif

    if (gpu_is_on())
      z80_g_execinstr();
    else
      z80_execinstr();

    /* Instruction trap? */
    if (dbg.itrap_enabled) {
      /*
       * Normally we don't want to break into debugger during int_lock
       * (i.e. after DD/CB prefix. However, if there are more DD/CB
       * prefixes in sequence, we will break into debugger.
       */
      if (!cpus.int_lock || dbg.prev_int_lock)
        debugger_run(&dbg);
      dbg.prev_int_lock = cpus.int_lock;
    }
}

int main(int argc, char **argv) {
  int argi;
  timer frmt;
  wkey_t k;
  
  argi = 1;
  
  dbl_ln=0;

  while (argc > argi && argv[argi][0] == '-') {
    if (!strcmp(argv[argi],"-midi")) {
	    if (argc <= argi + 1) {
		    printf("Option -midi missing argument.\n");
		    exit(1);
	    }
	    midi_dev = argv[argi + 1];
	    argi+=2;
    } else {
	    printf("Invalid option '%s'.\n", argv[argi]);
	    exit(1);
    }
  }

  uoc=0;
  smc=0;
  
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

  //printf("inited.\n");
  
  timer_reset(&frmt);
  
  while(!quit) {
    if(CLOCK_GE(z80_clock-disp_t,ULA_FIELD_TICKS)) { /* every 50th of a second */
      disp_t+=ULA_FIELD_TICKS;
#ifdef WITH_MIDI
      sysmidi_poll(z80_clock);
#endif
      mgfx_updscr();

      mgfx_input_update();
      while(w_getkey(&k)) key_handler(&k);
#ifdef LOG
      if(cpus.iff1) fprintf(logfi,"interrupt\n");
#endif
    }
    
    zx_proc_instr();
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
