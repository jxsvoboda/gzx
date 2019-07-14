/*
 * GZX - George's ZX Spectrum Emulator
 * Portable graphics/keyboard module
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "mgfx.h"

uint8_t *vscr0,*vscr1;

int scr_xs,scr_ys;
int clip_x0,clip_x1,clip_y0,clip_y1;

static int drw_clr;

/* 
  povoleni zapisu do sudych/lichych radek 
  pri neprokladanem rezimu se zapisuje jen do sudych
*/
int write_l0=1;
int write_l1=0;

int dbl_ln; /* Double lines enabled? */

#define SWAP(a,b,tmp) { (tmp)=(a); (a)=(b); (b)=(tmp); }
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

void mgfx_selln(int mask) {
  if(dbl_ln) {
    write_l0=mask&1;
    write_l1=mask&2;
  }
}

void mgfx_fillrect(int x0, int y0, int x1, int y1, int color) {
  int tmp,x,y;
  
  if(x1<x0) SWAP(x0,x1,tmp);
  if(y1<y0) SWAP(y0,y1,tmp);
  
  if(x0<clip_x0) x0=clip_x0;
  if(y0<clip_y0) y0=clip_y0;
  if(x1>clip_x1) x1=clip_x1;
  if(y1>clip_y1) y1=clip_y1;
  
  if(write_l0)
    for(y=y0;y<=y1;y++)
      for(x=x0;x<=x1;x++)
        vscr0[y*scr_xs+x]=color;
	
  if(write_l1)
    for(y=y0;y<=y1;y++)
      for(x=x0;x<=x1;x++)
        vscr1[y*scr_xs+x]=color;
}

void mgfx_setcolor(int color) {
  drw_clr=color;
}

void mgfx_drawpixel(int x, int y) {
  if(x<clip_x0 || y<clip_y0 || x>clip_x1 || y>clip_y1) return;
  if(write_l0) vscr0[y*scr_xs+x]=drw_clr;
  if(write_l1) vscr1[y*scr_xs+x]=drw_clr;
}

/**** gui - text/windows ****/

static uint8_t *gfont;
static int gdx, gdy;
uint8_t fgc,bgc;

int gloadfont(const char *name) {
  FILE *f;
  
  gfont=malloc(768);
  if(!gfont) return -1;
  
  f=fopen(name,"rb");
  if(!f) return -1;
  if(fread(gfont,1,768,f)!=768) return -1;
  fclose(f);
  return 0;
}

void gmovec(int cx, int cy) {
  gdx=cx<<3;
  gdy=cy<<3;
}

void gputc(char c) {
  int x,y;
  uint8_t b;
  
  if(c<32) c=0;
    else c=c-32;
  
  for(y=0;y<8;y++) {
    b=gfont[c*8+y];
    for(x=0;x<8;x++) {
      mgfx_setcolor((b&0x80)? fgc : bgc);
      mgfx_drawpixel(gdx+x,gdy+y);
      b<<=1;
    }
  }
  gdx+=8;
}

void gputs(const char *s) {
  while(*s) gputc(*s++);
}


/**** keyboard ****/

/* keymap */

static int ktxsrc_n[] = {
  WKEY_A,		'a',
  WKEY_B,		'b',
  WKEY_C,		'c',
  WKEY_D,		'd',
  WKEY_E,		'e',
  WKEY_F,		'f',
  WKEY_G,		'g',
  WKEY_H,		'h',
  WKEY_I,		'i',
  WKEY_J,		'j',
  WKEY_K,		'k',
  WKEY_L,		'l',
  WKEY_M,		'm',
  WKEY_N,		'n',
  WKEY_O,		'o',
  WKEY_P,		'p',
  WKEY_Q,		'q',
  WKEY_R,		'r',
  WKEY_S,		's',
  WKEY_T,		't',
  WKEY_U,		'u',
  WKEY_V,		'v',
  WKEY_W,		'w',
  WKEY_X,		'x',
  WKEY_Y,		'y',
  WKEY_Z,		'z',
  
  WKEY_0,		'0',
  WKEY_1,		'1',
  WKEY_2,		'2',
  WKEY_3,		'3',
  WKEY_4,		'4',
  WKEY_5,		'5',
  WKEY_6,		'6',
  WKEY_7,		'7',
  WKEY_8,		'8',
  WKEY_9,		'9',
  
  WKEY_N0,		'0',
  WKEY_N1,		'1',
  WKEY_N2,		'2',
  WKEY_N3,		'3',
  WKEY_N4,		'4',
  WKEY_N5,		'5',
  WKEY_N6,		'6',
  WKEY_N7,		'7',
  WKEY_N8,		'8',
  WKEY_N9,		'9',
  
  WKEY_SPACE,		' ',
  WKEY_MINUS,		'-',
  WKEY_EQUAL,		'=',
  WKEY_BS,		8,
  WKEY_TAB,		9,
  WKEY_ESC,		27,
  WKEY_LBR,		'[',
  WKEY_RBR,		']',
  WKEY_ENTER,		13,
  WKEY_SCOLON,		';',
  WKEY_FOOT,		39,
  WKEY_GRAVE,		'`',
  WKEY_BSLASH,		'\\',
  WKEY_COMMA,		',',
  WKEY_PERIOD,		'.',
  WKEY_SLASH,		'/',
  
  WKEY_NENTER,		13,
  WKEY_NSLASH,		'/',
  WKEY_NSTAR,		'*',
  WKEY_NMINUS,		'-',
  WKEY_NPLUS,		'+',
  WKEY_NPERIOD,		'.',

  -1,			-1
};

static int ktxsrc_s[] = {
  WKEY_A,		'A',
  WKEY_B,		'B',
  WKEY_C,		'C',
  WKEY_D,		'D',
  WKEY_E,		'E',
  WKEY_F,		'F',
  WKEY_G,		'G',
  WKEY_H,		'H',
  WKEY_I,		'I',
  WKEY_J,		'J',
  WKEY_K,		'K',
  WKEY_L,		'L',
  WKEY_M,		'M',
  WKEY_N,		'N',
  WKEY_O,		'O',
  WKEY_P,		'P',
  WKEY_Q,		'Q',
  WKEY_R,		'R',
  WKEY_S,		'S',
  WKEY_T,		'T',
  WKEY_U,		'U',
  WKEY_V,		'V',
  WKEY_W,		'W',
  WKEY_X,		'X',
  WKEY_Y,		'Y',
  WKEY_Z,		'Z',
  
  WKEY_0,		')',
  WKEY_1,		'!',
  WKEY_2,		'@',
  WKEY_3,		'#',
  WKEY_4,		'$',
  WKEY_5,		'%',
  WKEY_6,		'^',
  WKEY_7,		'&',
  WKEY_8,		'*',
  WKEY_9,		'(',
  
  WKEY_N0,		'0',
  WKEY_N1,		'1',
  WKEY_N2,		'2',
  WKEY_N3,		'3',
  WKEY_N4,		'4',
  WKEY_N5,		'5',
  WKEY_N6,		'6',
  WKEY_N7,		'7',
  WKEY_N8,		'8',
  WKEY_N9,		'9',
  
  WKEY_SPACE,		' ',
  WKEY_MINUS,		'_',
  WKEY_EQUAL,		'+',
  WKEY_BS,		8,
  WKEY_TAB,		9,
  WKEY_ESC,		27,
  WKEY_LBR,		'{',
  WKEY_RBR,		'}',
  WKEY_ENTER,		13,
  WKEY_SCOLON,		':',
  WKEY_FOOT,		'"',
  WKEY_GRAVE,		'~',
  WKEY_BSLASH,		'|',
  WKEY_COMMA,		'<',
  WKEY_PERIOD,		'>',
  WKEY_SLASH,		'?',
  
  WKEY_NENTER,		13,
  WKEY_NSLASH,		'/',
  WKEY_NSTAR,		'*',
  WKEY_NMINUS,		'-',
  WKEY_NPLUS,		'+',
  WKEY_NPERIOD,		'.',
  
  -1,			-1
};

static int *ktx_n,ktx_nn;
static int *ktx_s,ktx_sn;

static wkey_t wbuf[WKEYBUF_SIZE];
static int wh,wt;
static int lshift,rshift;

int w_getkey(wkey_t *k) {
  mgfx_input_update();
  
  if(wh==wt) return 0; /* no keys in queue */
  *k=wbuf[wt++];
  if(wt>=WKEYBUF_SIZE) wt=0;
    
  return 1;
}

void w_putkey(int press, int key, int c) {
  int *ktx,ktxn;
  
  wbuf[wh].press=press;
  wbuf[wh].key  =key;
  
  if(key==WKEY_LSHIFT) lshift=press;
  if(key==WKEY_RSHIFT) rshift=press;
  
  if(lshift || rshift) {
    ktx=ktx_s;
    ktxn=ktx_sn;
  } else {
    ktx=ktx_n;
    ktxn=ktx_nn;
  }
  
//  printf("key: %d\n",key);
  if(key>=0 && key<ktxn)
    wbuf[wh].c=ktx[key];
  else
    wbuf[wh].c=0;
//  printf("char: %d ('%c')\n",wbuf[wh].c,wbuf[wh].c);
  
  wh++;
  if(wh>=WKEYBUF_SIZE) wh=0;
  if(wh==wt) {
    printf("keybuffer overflow!\n");
    wh--;
  }
}

int w_initkey(void) {
  int i;
  
  wh=wt=0;
  lshift=rshift=0;
  
  /* setup scancode to ASCII translation */
  
  ktx_nn=1;
  for(i=0;ktxsrc_n[i*2+1]!=-1;i++)
    if(ktxsrc_n[i*2]>=ktx_nn) ktx_nn=ktxsrc_n[i*2]+1;
    
  ktx_n=calloc(ktx_nn,sizeof(int));
  if(!ktx_n) {
    printf("malloc failed\n");
    exit(1);
  }
  
  for(i=0;ktxsrc_n[i*2+1]!=-1;i++)
    ktx_n[ktxsrc_n[i*2]]=ktxsrc_n[i*2+1];
    
  ktx_sn=1;
  for(i=0;ktxsrc_s[i*2+1]!=-1;i++)
    if(ktxsrc_s[i*2]>=ktx_sn) ktx_sn=ktxsrc_s[i*2]+1;
    
  ktx_s=calloc(ktx_sn,sizeof(int));
  if(!ktx_s) {
    printf("malloc failed\n");
    exit(1);
  }
  
  for(i=0;ktxsrc_s[i*2+1]!=-1;i++)
    ktx_s[ktxsrc_s[i*2]]=ktxsrc_s[i*2+1];
  
  return 0;
}
