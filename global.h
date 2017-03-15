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

/* useful file i/o routines */
unsigned fgetu8(FILE *f);
void fungetu8(FILE *f, u8 c);
unsigned fgetu16le(FILE *f);
unsigned fgetu32le(FILE *f);
unsigned fgetu16be(FILE *f);
unsigned fgetu32be(FILE *f);

signed   fgets16le(FILE *f);
unsigned fgetu24le(FILE *f);

unsigned long fsize(FILE *f);

void fputu8(FILE *f, u8 val);
void fputu16le(FILE *f, u16 val);
void fputu16be(FILE *f, u16 val);

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
