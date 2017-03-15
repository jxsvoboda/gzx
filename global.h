#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <stdio.h>

/* pod win32 dodeklaruj nektere linuxove funkce emulovane v sys_win.c */

#ifdef __MINGW32__
#include "sys_win.h"
#else
#include <unistd.h>
#endif

//#define USE_GPU

/* machine-dependent typedefs */
#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned long

/* memory models */
#define ZXM_48K   0
#define ZXM_128K  1
#define ZXM_PLUS2 2
#define ZXM_PLUS3 3
#define ZXM_ZX81  4

#define Z80_CLOCK 3500000L

/* case insensitive strcmp */
int strcmpci(char *a, char *b);

u8 zx_key_in(u8 pwr);

void zx_reset(void);

void zx_debug_mstep(void);
void zx_debug_key(int press, int key);

extern char *start_dir;
extern FILE *logfi;
extern int quit;

extern int slow_load;

#endif
