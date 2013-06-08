#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <stdio.h>

/* pod win32 dodeklaruj nektere linuxove funkce emulovane v sys_win.c */

#ifdef __MINGW32__
#include "sys_win.h"
#endif

//#define USE_GPU

/* machine-dependent typedefs */
#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned long

/* mechine-independent typedefs */
#define uchar unsigned char
#define ushort unsigned short
#define uint unsigned int
#define ulong unsigned long

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

/* spectrum memory access */
u8 zx_memget8(u16 addr);
void zx_memset8(u16 addr, u8 val);
u16 zx_memget16(u16 addr);
void zx_memset16(u16 addr, u16 val);

/* spectrum i/o port access */
void zx_out8(u16 addr, u8 val);
u8 zx_in8(u16 addr);

void zx_reset(void);
int zx_select_memmodel(int model);
void zx_mem_page_select(u8 val);

void zx_debug_mstep(void);
void zx_debug_key(int press, int key);

extern u8 page_reg;
extern u8 border;
extern u8 spk,mic,ear;
extern u8 *zxram,*zxrom,*zxscr,*zxbnk[4];
extern int mem_model;

extern int bnk_lock48;
extern unsigned ram_size,rom_size;

extern FILE *logfi;
extern int quit;

extern int slow_load;

#endif
