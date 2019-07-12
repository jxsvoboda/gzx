/*
 * GZX - George's ZX Spectrum Emulator
 * Menus
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gzx.h"
#include "memio.h"
#include "mgfx.h"
#include "snap.h"
#include "menus.h"
#include "sys_all.h"
#include "tape/deck.h"
#include "ui/fsel.h"
#include "ui/teline.h"
#include "zx.h"

static void menu_undraw(void);

/***** main menu *****/

#define MENU_NENT 8

static int menu_cx0;

static char *mentry_text[MENU_NENT]= {
  "~Load Snapshot",
  "~Save Snapshot",
  "Select ~Tapefile",
  "Reset ~48",
  "Reset ~128",
  "Toggle ~Windowed",
  "Lock ~UI",
  "~Quit",
};

static int mkeys[MENU_NENT]={
  WKEY_L,WKEY_S,WKEY_T,WKEY_4,WKEY_1,WKEY_W,WKEY_U,WKEY_Q
};


/*
  Draw a string at fixed width, with ~=highlight
*/
static void gputs_hl(int n, int fgc_, int hlc, int bgc_, char *s) {
  bgc=bgc_;
  while(n>0) {
    if(*s=='~') { fgc=hlc; s++; }
      else fgc=fgc_;
      
    gputc(*s);
    if(*s) s++;
    n--;
  }
}

//#include <alloc.h>
static void menu_draw(int mpos) {
  int i;
  int fgc_,bgc_,hlc_;
//  char buf[32];

  mgfx_fillrect(menu_cx0*8,0,menu_cx0*8+8*20,scr_ys-1,1);
  gmovec(scr_xs/16-(strlen("Main Menu")/2),0);
  fgc=7; bgc=1; gputs("Main Menu");

  for(i=0;i<MENU_NENT;i++) {
    if(i==mpos) { fgc_=hlc_=1; bgc_=5; }
      else { fgc_=7; hlc_=14; bgc_=1; }
    gmovec(menu_cx0+1,2+i);
    gputs_hl(18,fgc_,hlc_,bgc_,mentry_text[i]);
  }

//  sprintf(buf,"%lu",coreleft());
//  gputs(scr_xs/2-8*4,scr_ys-8,10,0,buf);
}

static void menu_undraw(void)
{
	mgfx_fillrect(0, 0, scr_xs, scr_ys, 0);
}

static void menu_run_line(int l) {
  switch(l) {
    case 0: load_snap_dialog(); break;
    case 1: save_snap_dialog(); break;
    case 2: select_tapefile_dialog(); break;
    case 3: zx_select_memmodel(ZXM_48K); zx_reset(); break;
    case 4: zx_select_memmodel(ZXM_128K); zx_reset(); break;
    case 5: mgfx_toggle_fs(); break;
    case 6: gzx_ui_lock(); break;
    case 7: quit=1; break;
  }
}

void main_menu(void) {
  wkey_t k;
  int end_menu = 0;
  int j;
  int mpos;
  
  mpos=0;
  menu_cx0=scr_xs/16 - 10;
  
  mgfx_selln(3); /* Enable rendering odd and even lines */

  while(!end_menu) {
    menu_draw(mpos);
    mgfx_updscr();
    do {
      mgfx_input_update();
      sys_usleep(1000);
    } while(!w_getkey(&k));
    if(k.press)
    switch(k.key) {
      case WKEY_ESC:
        end_menu=1;
	break;

      case WKEY_ENTER:
	menu_run_line(mpos);
	end_menu=1;
	break;
	
      case WKEY_UP:
        if(mpos>0) --mpos;
	  else mpos=MENU_NENT-1;
	break;
	
      case WKEY_DOWN:
        if(mpos<MENU_NENT-1) ++mpos;
	  else mpos=0;
	break;

      case WKEY_PGUP:
        mpos=0;
	break;
	
      case WKEY_PGDN:
        mpos=MENU_NENT-1;
	break;

      default:
	for(j=0;j<MENU_NENT;j++)
	  if(k.key==mkeys[j]) {
	    menu_run_line(j);
	    end_menu=1;
	  }
	break;
    }
  }

  menu_undraw();
}

/***** tape menu *****/

#define TMENU_NENT 7

static char *tmentry_text[TMENU_NENT]= {
  "~Play",
  "~Stop",
  "~Rewind",
  "~Quick Load Toggle",
  "~New",
  "Sa~ve",
  "Save ~As"
};

static int tmkeys[TMENU_NENT]={
  WKEY_P,WKEY_S,WKEY_R,WKEY_Q,WKEY_N,WKEY_V,WKEY_A
};

static void tmenu_draw(int mpos) {
  int i;
  int fgc_,bgc_,hlc_;
  
  mgfx_fillrect(menu_cx0*8,0,menu_cx0*8+8*20,scr_ys-1,1);
  gmovec(scr_xs/16-(strlen("Tape Menu")/2),0);
  fgc=7; bgc=1; gputs("Tape Menu");
 
  for(i=0;i<TMENU_NENT;i++) {
    if(i==mpos) { fgc_=hlc_=1; bgc_=5; }
      else { fgc_=7; hlc_=14; bgc_=1; }
    gmovec(menu_cx0+1,2+i);
    gputs_hl(18,fgc_,hlc_,bgc_,tmentry_text[i]);
  }
}

static void tmenu_run_line(int l) {
  switch(l) {
    case 0: tape_deck_play(tape_deck); break;
    case 1: tape_deck_stop(tape_deck); break;
    case 2: tape_deck_rewind(tape_deck); break;
    case 3: slow_load=!slow_load; break;
    case 4: tape_deck_new(tape_deck); break;
    case 5: tape_deck_save(tape_deck); break;
    case 6: save_tape_as_dialog(); break;
  }
}

void tape_menu(void) {
  wkey_t k;
  int end_menu = 0;
  int j;
  int mpos;

  mpos=0;
  menu_cx0=scr_xs/16 - 10;
  
  mgfx_selln(3); /* Enable rendering odd and even lines */
  
  while(!end_menu) {
    tmenu_draw(mpos);
    mgfx_updscr();
    do {
      mgfx_input_update();
      sys_usleep(1000);
    } while(!w_getkey(&k));
    if(k.press)
    switch(k.key) {
      case WKEY_ESC:
        end_menu=1;
	break;
	
      case WKEY_ENTER:
        tmenu_run_line(mpos);
	end_menu=1;
	break;
	
      case WKEY_UP:
        if(mpos>0) --mpos;
	  else mpos=TMENU_NENT-1;
	break;

      case WKEY_DOWN:
        if(mpos<TMENU_NENT-1) ++mpos;
	  else mpos=0;
	break;
	
      case WKEY_PGUP:
        mpos=0;
	break;
	
      case WKEY_PGDN:
	mpos=MENU_NENT-1;
	break;

      default:
	for(j=0;j<TMENU_NENT;j++)
	  if(k.key==tmkeys[j]) {
	    tmenu_run_line(j);
	    end_menu=1;
	  }
	break;
    }
  }

  menu_undraw();
}


/**** select tapefile dialog *****/

void select_tapefile_dialog(void) {
  char *fname;
  
  if(file_sel(&fname,"Select Tapefile")>0) {
    fprintf(logfi,"selecting tape file\n"); fflush(logfi);
    (void) tape_deck_open(tape_deck, fname);
    fprintf(logfi,"freeing filename\n"); fflush(logfi);
    free(fname);
  }
}

void load_snap_dialog(void) {
  char *fname;
  
  if(file_sel(&fname,"Load Snapshot")>0) {
    zx_load_snap(fname);
    free(fname);
  }
}

void save_snap_dialog(void) {
  wkey_t k;
  int fscols;
  int flist_cx0;
  teline_t fn_line;
  
  fscols=20;
  flist_cx0 = scr_xs/16 - fscols/2;
  teline_init(&fn_line,flist_cx0,12,20);
  fn_line.focus = 1;
    
  while(1) {
    mgfx_fillrect(flist_cx0*8-8,0,flist_cx0*8+8*(fscols+1),scr_ys-1,1);
    teline_draw(&fn_line);
    gmovec(scr_xs/16-(strlen("Save Snapshot")/2),0);
    fgc=7; bgc=1; gputs("Save Snapshot");

    mgfx_updscr();
    do {
      mgfx_input_update();
      sys_usleep(1000);
    } while(!w_getkey(&k));
    
    if(k.press)
    switch(k.key) {
      case WKEY_ESC:
	return;

      case WKEY_ENTER:
        fn_line.buf[fn_line.len]=0;
	zx_save_snap(fn_line.buf);
	return;
        
      default:
        teline_key(&fn_line,&k);
	break;
    }
  }
}

void save_tape_as_dialog(void) {
  wkey_t k;
  int fscols;
  int flist_cx0;
  teline_t fn_line;
  
  fscols=20;
  flist_cx0 = scr_xs/16 - fscols/2;
  teline_init(&fn_line,flist_cx0,12,20);
  fn_line.focus = 1;
    
  while(1) {
    mgfx_fillrect(flist_cx0*8-8,0,flist_cx0*8+8*(fscols+1),scr_ys-1,1);
    teline_draw(&fn_line);
    gmovec(scr_xs/16-(strlen("Save Tape As")/2),0);
    fgc=7; bgc=1; gputs("Save Tape As");

    mgfx_updscr();
    do {
      mgfx_input_update();
      sys_usleep(1000);
    } while(!w_getkey(&k));
    
    if(k.press)
    switch(k.key) {
      case WKEY_ESC:
	return;

      case WKEY_ENTER:
        fn_line.buf[fn_line.len]=0;
	tape_deck_save_as(tape_deck, fn_line.buf);
	return;
        
      default:
        teline_key(&fn_line,&k);
	break;
    }
  }
}
