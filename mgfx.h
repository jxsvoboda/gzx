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

#ifndef CGFX_H
#define CGFX_H

#include <stdint.h>

#define WKEYBUF_SIZE 64
#define KST_SIZE 128

typedef struct {
  int press;            /* 1=press, 0=release */
  int key;              /* our own scancode */
  int c;                /* ASCII character */
} wkey_t;
      
void mgfx_selln(int mask);
void mgfx_setcolor(int color);
void mgfx_drawpixel(int x, int y);
void mgfx_fillrect(int x0, int y0, int x1, int y1, int color);

extern uint8_t fgc, bgc;
int gloadfont(const char *name);
void gmovec(int cx, int cy);
void gputc(char c);
void gputs(const char *s);

/* keyboard */
int w_getkey(wkey_t *k);
void w_putkey(int press, int key, int c);
int w_initkey(void);

extern uint8_t *vscr0;
extern uint8_t *vscr1;
extern int clip_x0,clip_y0,clip_x1,clip_y1;
extern int scr_xs,scr_ys;
extern int dbl_ln;
extern int write_l0, write_l1; /* read only, write using mgfx_selln */

extern int thpos,tvpos;

/* gfx-api-dependent .. contained in gfx_win.c/gfx_vga.c */

int mgfx_init(void);
void mgfx_updscr(void);
void mgfx_setpal(int base, int cnt, int *pal);
int mgfx_toggle_fs(void);
void mgfx_input_update(void);

/* our own key scancodes */

#define WKEY_		0

#define WKEY_ESC        1
#define WKEY_1          2
#define WKEY_2          3
#define WKEY_3          4
#define WKEY_4          5
#define WKEY_5          6
#define WKEY_6          7
#define WKEY_7          8
#define WKEY_8          9
#define WKEY_9          10
#define WKEY_0          11
#define WKEY_MINUS      12
#define WKEY_EQUAL      13
#define WKEY_BS         14
#define WKEY_TAB        15
#define WKEY_Q          16
#define WKEY_W          17
#define WKEY_E          18
#define WKEY_R          19
#define WKEY_T          20
#define WKEY_Y          21
#define WKEY_U          22
#define WKEY_I          23
#define WKEY_O          24
#define WKEY_P          25
#define WKEY_LBR        26
#define WKEY_RBR        27
#define WKEY_ENTER      28
#define WKEY_LCTRL      29
#define WKEY_A          30
#define WKEY_S          31
#define WKEY_D          32
#define WKEY_F          33
#define WKEY_G          34
#define WKEY_H          35
#define WKEY_J          36
#define WKEY_K          37
#define WKEY_L          38
#define WKEY_SCOLON     39
#define WKEY_FOOT       40
#define WKEY_GRAVE      41
#define WKEY_LSHIFT     42
#define WKEY_BSLASH     43
#define WKEY_Z          44
#define WKEY_X          45
#define WKEY_C          46
#define WKEY_V          47
#define WKEY_B          48
#define WKEY_N          49
#define WKEY_M          50
#define WKEY_COMMA      51
#define WKEY_PERIOD     52
#define WKEY_SLASH      53
#define WKEY_RSHIFT     54
#define WKEY_NSTAR      55
#define WKEY_LALT       56
#define WKEY_SPACE      57
#define WKEY_CLOCK      58
#define WKEY_F1         59
#define WKEY_F2         60
#define WKEY_F3         61
#define WKEY_F4         62
#define WKEY_F5         63
#define WKEY_F6         64
#define WKEY_F7         65
#define WKEY_F8         66
#define WKEY_F9         67
#define WKEY_F10        68
#define WKEY_NLOCK      69
#define WKEY_SLOCK      70
#define WKEY_N7         71
#define WKEY_N8         72
#define WKEY_N9         73
#define WKEY_NMINUS     74
#define WKEY_N4         75
#define WKEY_N5         76
#define WKEY_N6         77
#define WKEY_NPLUS      78
#define WKEY_N1         79
#define WKEY_N2         80
#define WKEY_N3         81
#define WKEY_N0         82
#define WKEY_NPERIOD    83
#define WKEY_LESS       84
#define WKEY_F11        85
#define WKEY_F12        86
#define WKEY_NENTER     87
#define WKEY_RCTRL      88
#define WKEY_NSLASH     89
#define WKEY_PRNSCR     90
#define WKEY_RALT       91
#define WKEY_BRK        92
#define WKEY_HOME       93
#define WKEY_UP         94
#define WKEY_PGUP       95
#define WKEY_LEFT       96
#define WKEY_RIGHT      97
#define WKEY_END        98
#define WKEY_DOWN       99
#define WKEY_PGDN       100
#define WKEY_INS        101
#define WKEY_DEL        102
#define WKEY_LOS        103
#define WKEY_ROS        104
#define WKEY_MENU	105

#endif
