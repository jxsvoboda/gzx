/*
 * GZX - George's ZX Spectrum Emulator
 * SDL graphics
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
#include <SDL.h>
#include "../../mgfx.h"

#define WINDOW_CAPTION "GZX"

static SDL_Surface *sdl_screen;
static int fs = 0;
static  SDL_Color color[256];
static int xscale;
static int yscale;
static int video_w, video_h;

static int *txkey;
static int txsize;
static int ktabsrc[]= {
  SDLK_ESCAPE,			WKEY_ESC,
  SDLK_1,			WKEY_1,
  SDLK_2,			WKEY_2,
  SDLK_3,			WKEY_3,
  SDLK_4,			WKEY_4,
  SDLK_5,			WKEY_5,
  SDLK_6,			WKEY_6,
  SDLK_7,			WKEY_7,
  SDLK_8,			WKEY_8,
  SDLK_9,			WKEY_9,
  SDLK_0,			WKEY_0,
  SDLK_MINUS,			WKEY_MINUS,
  SDLK_EQUALS,			WKEY_EQUAL,
  SDLK_BACKSPACE,		WKEY_BS,
  SDLK_TAB,			WKEY_TAB,
  SDLK_q,			WKEY_Q,
  SDLK_w,			WKEY_W,
  SDLK_e,			WKEY_E,
  SDLK_r,			WKEY_R,
  SDLK_t,			WKEY_T,
  SDLK_y,			WKEY_Y,
  SDLK_u,			WKEY_U,
  SDLK_i,			WKEY_I,
  SDLK_o,			WKEY_O,
  SDLK_p,			WKEY_P,
  SDLK_LEFTBRACKET,		WKEY_LBR,
  SDLK_RIGHTBRACKET,		WKEY_RBR,
  SDLK_RETURN,			WKEY_ENTER,
  SDLK_LCTRL,			WKEY_LCTRL,
  SDLK_a,			WKEY_A,
  SDLK_s,			WKEY_S,
  SDLK_d,			WKEY_D,
  SDLK_f,			WKEY_F,
  SDLK_g,			WKEY_G,
  SDLK_h,			WKEY_H,
  SDLK_j,			WKEY_J,
  SDLK_k,			WKEY_K,
  SDLK_l,			WKEY_L,
  SDLK_SEMICOLON,		WKEY_SCOLON,
  SDLK_QUOTE,			WKEY_FOOT,
  SDLK_BACKQUOTE,		WKEY_GRAVE,
  SDLK_LSHIFT,			WKEY_LSHIFT,
  SDLK_BACKSLASH,		WKEY_BSLASH,
  SDLK_z,			WKEY_Z,
  SDLK_x,			WKEY_X,
  SDLK_c,			WKEY_C,
  SDLK_v,			WKEY_V,
  SDLK_b,			WKEY_B,
  SDLK_n,			WKEY_N,
  SDLK_m,			WKEY_M,
  SDLK_COMMA,			WKEY_COMMA,
  SDLK_PERIOD,			WKEY_PERIOD,
  SDLK_SLASH,			WKEY_SLASH,
  SDLK_RSHIFT,			WKEY_RSHIFT,
  SDLK_KP_MULTIPLY,		WKEY_NSTAR,
  SDLK_LALT,			WKEY_LALT,
  SDLK_SPACE,			WKEY_SPACE,
  SDLK_CAPSLOCK,		WKEY_CLOCK,
  SDLK_F1,			WKEY_F1,
  SDLK_F2,			WKEY_F2,
  SDLK_F3,			WKEY_F3,
  SDLK_F4,			WKEY_F4,
  SDLK_F5,			WKEY_F5,
  SDLK_F6,			WKEY_F6,
  SDLK_F7,			WKEY_F7,
  SDLK_F8,			WKEY_F8,
  SDLK_F9,			WKEY_F9,
  SDLK_F10,			WKEY_F10,
  SDLK_NUMLOCK,			WKEY_NLOCK,
  SDLK_SCROLLOCK,		WKEY_SLOCK,
  SDLK_KP7,			WKEY_N7,
  SDLK_KP8,			WKEY_N8,
  SDLK_KP9,			WKEY_N9,
  SDLK_KP_MINUS,		WKEY_NMINUS,
  SDLK_KP4,			WKEY_N4,
  SDLK_KP5,			WKEY_N5,
  SDLK_KP6,			WKEY_N6,
  SDLK_KP_PLUS,			WKEY_NPLUS,
  SDLK_KP1,			WKEY_N1,
  SDLK_KP2,			WKEY_N2,
  SDLK_KP3,			WKEY_N3,
  SDLK_KP0,			WKEY_N0,
  SDLK_KP_PERIOD,		WKEY_NPERIOD,
  SDLK_LESS,			WKEY_LESS,
  SDLK_F11,			WKEY_F11,
  SDLK_F12,			WKEY_F12,
  SDLK_KP_ENTER,		WKEY_NENTER,
  SDLK_RCTRL,			WKEY_RCTRL,
  SDLK_KP_DIVIDE,		WKEY_NSLASH,
  SDLK_PRINT,			WKEY_PRNSCR,
  SDLK_RALT,			WKEY_RALT,
  SDLK_BREAK,			WKEY_BRK,
  SDLK_HOME,			WKEY_HOME,
  SDLK_UP,			WKEY_UP,
  SDLK_PAGEUP,			WKEY_PGUP,
  SDLK_LEFT,			WKEY_LEFT,
  SDLK_RIGHT,			WKEY_RIGHT,
  SDLK_END,			WKEY_END,
  SDLK_DOWN,			WKEY_DOWN,
  SDLK_PAGEDOWN,		WKEY_PGDN,
  SDLK_INSERT,			WKEY_INS,
  SDLK_DELETE,			WKEY_DEL,
  SDLK_LSUPER,			WKEY_LOS,
  SDLK_RSUPER,			WKEY_ROS,
  -1,				-1
};

/* graphics */

static void w_vga_problem(void) {
  fprintf(stderr, "vga Problem!");
  exit(1);
}

static void init_video(void) {
  int flags;
  int vw, vh;
  
  /* Initialize SDL */
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    w_vga_problem();
  }
  
  flags = SDL_SWSURFACE | (fs ? SDL_FULLSCREEN : 0);
  scr_xs = video_w;
  scr_ys = video_h;
  
  if (dbl_ln) {
    xscale = 2;
    yscale = 1;
  } else {
    xscale = 2;
    yscale = 2;
  }
  
  vw = scr_xs * xscale;
  vh = scr_ys * yscale;
  
  if (dbl_ln) {
    vh *= 2;
  }
  
  SDL_WM_SetCaption(WINDOW_CAPTION, WINDOW_CAPTION);
  
  sdl_screen = SDL_SetVideoMode(vw, vh, 8, flags);
  
  if (sdl_screen == NULL)
    w_vga_problem();
}

static void init_vscr(void) {
  vscr0=calloc(scr_xs*scr_ys, sizeof(uint8_t));
  if(!vscr0) {
    printf("malloc failed\n");
    exit(1);
  }
  
  if(dbl_ln) {
    vscr1=calloc(scr_xs*scr_ys, sizeof(uint8_t));
    if(!vscr1) {
      printf("malloc failed\n");
      exit(1);
    }
  }
}

static void fini_vscr(void) {
  free(vscr0);
  vscr0 = NULL;
  if(dbl_ln) {
    free(vscr1);
    vscr1 = NULL;
  }
}

static void quit_video(void) {
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

int mgfx_init(int w, int h) {
  int i;
  
  atexit(SDL_Quit);
  
  video_w = w;
  video_h = h;
  init_video();
  
  w_initkey();
  
  /* set up key translation table */
  txsize=1;
  for(i=0;ktabsrc[i*2+1]!=-1;i++)
    if(ktabsrc[i*2]>=txsize) txsize=ktabsrc[i*2]+1;
    
  txkey=calloc(txsize,sizeof(int));
  if(!txkey) {
    printf("malloc failed\n");
    exit(1);
  }
  
  for(i=0;ktabsrc[i*2+1]!=-1;i++)
    txkey[ktabsrc[i*2]]=ktabsrc[i*2+1];
    
  /* set up virtual frame buffer */
  init_vscr();
  
  mgfx_selln(3);

  clip_x0=clip_y0=0;
  clip_x1=scr_xs-1;
  clip_y1=scr_ys-1;
  
  return 0;
}

static void render_display_line(int dy, uint8_t *spix)
{
  uint8_t *dp;
  int i, j, k;
  
  dp = sdl_screen->pixels + sdl_screen->pitch * dy * yscale;
  if (xscale != 1 || yscale != 1) {
    for (j = 0; j < yscale; j++) {
      for (i = 0; i < scr_xs; i++) {
        for (k = 0; k < xscale; k++) {
          dp[xscale * i + k] = spix[i];
        }
      }
      dp += sdl_screen->pitch;
    }
  } else {
    memcpy(dp, spix, scr_xs);
  }
}

void mgfx_updscr(void) {
  unsigned y;
  
  if(dbl_ln) {
    for (y = 0; y < scr_ys; y++) {
      render_display_line(2 * y, vscr0 + scr_xs * y);
      render_display_line(2 * y + 1, vscr1 + scr_xs * y);
    }
  } else {
    for (y = 0; y < scr_ys; y++) {
      render_display_line(y, vscr0 + scr_xs * y);
    }
  }
  SDL_UpdateRect(sdl_screen, 0, 0, 0, 0);
}

static unsigned b6to8(unsigned cval)
{
  return 255 * cval / 63;
}

void mgfx_setpal(int base, int cnt, int *p) {
  int i;
  
  for (i = 0; i < cnt; i++) {
    color[i].r = b6to8(p[3*i]);
    color[i].g = b6to8(p[3*i + 1]);
    color[i].b = b6to8(p[3*i + 2]);
  }
  
  SDL_SetColors(sdl_screen, color, 0, 256);
}

/* input */

void mgfx_input_update(void) {
  SDL_Event event;
  
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      exit(0);
      break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      // XXX use SDL key to character translation
      w_putkey(event.type == SDL_KEYDOWN, txkey[event.key.keysym.sym], -1);
      break;
    default:
      break;
    }
  }
}

int mgfx_toggle_fs(void) {
  /* Toggle fullscreen mode */
  quit_video();
  fs = !fs;
  init_video();
  SDL_SetColors(sdl_screen, color, 0, 256);
  return 0;
}

int mgfx_toggle_dbl_ln(void) {
  quit_video();
  fini_vscr();
  dbl_ln = !dbl_ln;
  /* Make sure to update write bits */
  mgfx_selln(3);
  init_video();
  init_vscr();
  SDL_SetColors(sdl_screen, color, 0, 256);
  return 0;
}

int mgfx_is_fs(void) {
  return fs;
}

int mgfx_set_disp_size(int w, int h)
{
  video_w = w;
  video_h = h;
  quit_video();
  fini_vscr();
  init_video();
  init_vscr();
  clip_x0=clip_y0=0;
  clip_x1=scr_xs-1;
  clip_y1=scr_ys-1;
  SDL_SetColors(sdl_screen, color, 0, 256);
  return 0;
}