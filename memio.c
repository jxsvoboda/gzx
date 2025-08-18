/*
 * GZX - George's ZX Spectrum Emulator
 * Memory and I/O port access
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ay.h"
#include "gzx.h"
#include "iorec.h"
#include "iospace.h"
#include "memio.h"
#include "sys_all.h"
#include "video/ulaplus.h"
#include "z80.h"
#include "z80g.h"
#include "zx.h"
#include "zx_kbd.h"
#include "zx_scr.h"

uint8_t *zxram, *zxrom;	/* whole memory */
uint8_t *zxbnk[4];	/* currently switched in banks */
uint8_t *zxscr;		/* selected screen bank */
uint8_t border;
uint8_t spk, mic, ear;

uint32_t ram_size, rom_size;
int has_banksw, bnk_lock48, has_epg;
int mem_model;

uint8_t page_reg; /* last data written to the page select port */
uint8_t epg_reg; /* last data written to the enhanced paging port */

static int rom_load(char *fname, int bank, uint16_t banksize);
static int spec_rom_load(char *fname, int bank);

/*
 * memory access routines
 * any wraps as MMIOs should be placed here
 */

uint8_t zx_memget8(uint16_t addr)
{
	if (mem_model == ZXM_ZX81) {
		if (addr <= 0x7fff)
			return zxbnk[addr >> 14][addr & 0x1fff];
		else
			return 0xff;
	} else {
		return zxbnk[addr >> 14][addr & 0x3fff];
	}
}

uint8_t zx_imemget8(uint16_t addr)
{
	if (mem_model == ZXM_48K) {
		if (addr < 16384)
			return zxrom[addr];
		else
			return zxram[addr - 16384];
	} else {
		return zx_memget8(addr);
	}
}

void zx_memset8(uint16_t addr, uint8_t val)
{
	if (mem_model == ZXM_ZX81) {
		if (addr >= 8192 && addr <= 0x7fff)
			zxbnk[addr >> 14][addr & 0x1fff] = val;
	} else {
		if (addr >= 16384 || (epg_reg & 1) != 0) {
			zxbnk[addr >> 14][addr & 0x3fff] = val;
		} else {
			// printf("%4x: memory protecion error, write to "
			//   "0x%04x\n", cpus.PC, addr);
		}
	}
}

/** Write byte without ROM protection */
void zx_memset8f(uint16_t addr, uint8_t val)
{
	zxbnk[addr >> 14][addr & 0x3fff] = val;
}

uint16_t zx_memget16(uint16_t addr)
{
	return (uint16_t)zx_memget8(addr) + (((uint16_t)zx_memget8(addr + 1)) << 8);
}

void zx_memset16(uint16_t addr, uint16_t val)
{
	zx_memset8(addr, val & 0xff);
	zx_memset8(addr + 1, val >> 8);
}

static void zx_epg_write(uint8_t val)
{
	uint8_t memmode;
	epg_reg = val;

	if ((epg_reg & 1) == 0) {
		/* back to normal paging */
		zxbnk[1] = zxram + 5 * 0x4000;
		zxbnk[2] = zxram + 2 * 0x4000;
		return;
	}

	/* 'enhanced' paging */
	memmode = (epg_reg >> 1) & 0x03;
	switch (memmode) {
	case 0:
		zxbnk[0] = zxram + 0 * 0x4000;
		zxbnk[1] = zxram + 1 * 0x4000;
		zxbnk[2] = zxram + 2 * 0x4000;
		zxbnk[3] = zxram + 3 * 0x4000;
		break;
	case 1:
		zxbnk[0] = zxram + 4 * 0x4000;
		zxbnk[1] = zxram + 5 * 0x4000;
		zxbnk[2] = zxram + 6 * 0x4000;
		zxbnk[3] = zxram + 7 * 0x4000;
		break;
	case 2:
		zxbnk[0] = zxram + 4 * 0x4000;
		zxbnk[1] = zxram + 5 * 0x4000;
		zxbnk[2] = zxram + 6 * 0x4000;
		zxbnk[3] = zxram + 3 * 0x4000;
		break;
	case 3:
		zxbnk[0] = zxram + 4 * 0x4000;
		zxbnk[1] = zxram + 7 * 0x4000;
		zxbnk[2] = zxram + 6 * 0x4000;
		zxbnk[3] = zxram + 3 * 0x4000;
		break;
	}
}

void zx_mem_page_select(uint16_t addr, uint8_t val)
{
	uint8_t rom;

	if (!has_banksw || bnk_lock48)
		return;

	if (has_epg) {
		/* with EPG exact port numbers are needed for EPG and PAGESEL */
		if (addr == ZXPLUS_EPG_PORT) {
			zx_epg_write(val);
			if ((epg_reg & 1) != 0)
				return;
		} else if (addr == ZXPLUS_PAGESEL_PORT) {
			page_reg = val;
		}

		rom = ((page_reg & 0x10) ? 1 : 0) +
		    ((epg_reg & 0x04) ? 2 : 0);
	} else {
		/* for 128K exact PAGESEL port number is not required */
		page_reg = val;
		rom = (val & 0x10) ? 1 : 0;
	}

	zxbnk[3] = zxram + ((uint32_t)(page_reg & 0x07) << 14); /* RAM select */
	zxbnk[0] = zxrom + rom * 0x4000;                        /* ROM select */
	zxscr   = zxram + ((page_reg & 0x08) ? 0x1c000 : 0x14000); /* screen select */
	//  printf("bnk select 0x%02x: ram=%d,rom=%d,scr=%d\n",val,val&7,val&0x10,val&0x08);
	if (page_reg & 0x20) { /* 48k lock */
		bnk_lock48 = 1;
		gzx_notify_mode_48k(true);
	}
}

/* select default banks */
void zx_mem_page_reset(void)
{
	bnk_lock48 = 0;
	epg_reg = 0x00;
	zx_mem_page_select(ZXPLUS_PAGESEL_PORT, 0x07);
}

/* Return nonzero if 0-0x3fff contains 48K BASIC ROM. */
int zx_mem_is_48k_basic_rom(void)
{
	if (!has_banksw)
		return 1;
	if (has_epg) {
		if ((epg_reg & 1) == 0) {
			/* only if ROM 3 is paged in */
			return ((page_reg & 0x10) != 0 && (epg_reg & 0x04) != 0) ? 1 : 0;
		} else {
			/* all-RAM mode */
			return 0;
		}
	} else {
		/* only if ROM 1 is paged in */
		return (page_reg & 0x10) ? 1 : 0;
	}
}

/** ****************************** */
/*
 * IO routines
 * all wraps should be placed here
 */

uint8_t zx_in8(uint16_t a)
{
	//  printf("in 0x%04x\n",a);
	//  z80_printstatus();
	//  getchar();
	if (a == AY_REG_READ_PORT && ay0_enable)
		return ay_reg_read(&ay0);
	if ((a & ZX128K_PAGESEL_PORT_MASK) == ZX128K_PAGESEL_PORT_VAL)
		printf("bnk sw port read!!!!!!\n");
	switch (a & 0xff) {
		/* ULA */
	case ULA_PORT:
		return zx_key_in(&keys, a >> 8) | 0xa0 | (ear ? 0x40 : 0x00);

	case KEMPSTON_JOY_A_PORT:
		if (kjoy0_enable)
			return kempston_joy_read(&kjoy0);
	default:
		break;
	}

	printf("in 0x%04x\n (no device there)", a);
	return video_ula.idle_bus_byte;
}

void zx_out8(uint16_t addr, uint8_t val)
{
	//  printf("out (0x%04x),0x%02x\n",addr,val);
	if (iorec != NULL)
		iorec_out(iorec, z80_clock, addr, val);

	if ((addr & ULA_PORT_MASK) == ULA_PORT) {
		/* the ULA (border/speaker/mic) */
		border = val & 7;
		spk = (val & 0x10) == 0;
		mic = (val & 0x18) == 0;
		//    printf("border %d, spk:%d, mic:%d\n",border,(val>>4)&1,(val>>3)&1);
		//    z80_printstatus();
		//    getchar();
	} else if ((addr & ZX128K_PAGESEL_PORT_MASK) == ZX128K_PAGESEL_PORT_VAL) {
		zx_mem_page_select(addr, val);
	} else if (addr == AY_REG_WRITE_PORT && ay0_enable) {
		ay_reg_write(&ay0, val);
	}

	if (addr == AY_REG_SEL_PORT && ay0_enable) {
		ay_reg_select(&ay0, val);
	} else if (addr == ULAPLUS_REGSEL_PORT && video_ula.plus_enable) {
		ulaplus_write_regsel(&video_ula.plus, val);
		zx_scr_update_pal();
	} else if (addr == ULAPLUS_DATA_PORT && video_ula.plus_enable) {
		ulaplus_write_data(&video_ula.plus, val);
		zx_scr_update_pal();
	} else {
		// printf("out (0x%04x),0x%02x (no device there)\n",addr,val);
	}

	/* no device attached */
}

int zx_select_memmodel(int model)
{
	int i;
	char *cur_dir;

	mem_model = model;
	switch (model) {
	case ZXM_48K:
		/* 48K spectrum */
		ram_size = 48 * 1024;
		rom_size = 16 * 1024;
		has_banksw = 0;
		has_epg = 0;
		break;

	case ZXM_128K:
		/* 128K spectrum */
		ram_size = 128 * 1024;
		rom_size = 32 * 1024;
		has_banksw = 1;
		has_epg = 0;
		break;

	case ZXM_PLUS2:
		/* spectrum +2 */
		ram_size = 128 * 1024;
		rom_size = 32 * 1024;
		has_banksw = 1;
		has_epg = 0;
		break;

	case ZXM_PLUS2A:
	case ZXM_PLUS3:
		/* spectrum +3 */
		ram_size = 128 * 1024;
		rom_size = 64 * 1024;
		has_banksw = 1;
		has_epg = 1;
		break;

	case ZXM_ZX81:
		/* ZX81 */
		ram_size = 24 * 1024;
		rom_size = 16 * 1024;
		has_banksw = 0;
		has_epg = 0;
		break;
	}

	/* reallocate memory */
	zxram = realloc(zxram, ram_size);
	zxrom = realloc(zxrom, rom_size);
	if (zxram == NULL || zxrom == NULL) {
		printf("malloc failed\n");
		return -1;
	}

	/* fill RAM with random stuff */
	srand(time(NULL));
	for (i = 0; i < ram_size; i++)
		zxram[i] = rand();

	/* load ROM */
	cur_dir = sys_getcwd(NULL, 0);
	if (start_dir)
		sys_chdir(start_dir);

	switch (mem_model) {
	case ZXM_48K:
		if (spec_rom_load("roms/zx48.rom", 0) < 0)
			return -1;
		break;

	case ZXM_128K:
		if (spec_rom_load("roms/zx128_0.rom", 0) < 0)
			return -1;
		if (spec_rom_load("roms/zx128_1.rom", 1) < 0)
			return -1;
		break;

	case ZXM_PLUS2:
		if (spec_rom_load("roms/zxp2_0.rom", 0) < 0)
			return -1;
		if (spec_rom_load("roms/zxp2_1.rom", 1) < 0)
			return -1;
		break;

	case ZXM_PLUS2A:
	case ZXM_PLUS3:
		if (spec_rom_load("roms/zxp3_0.rom", 0) < 0)
			return -1;
		if (spec_rom_load("roms/zxp3_1.rom", 1) < 0)
			return -1;
		if (spec_rom_load("roms/zxp3_2.rom", 2) < 0)
			return -1;
		if (spec_rom_load("roms/zxp3_3.rom", 3) < 0)
			return -1;
		break;

	case ZXM_ZX81:
		if (rom_load("roms/zx81.rom", 0, 0x2000) < 0)
			return -1;
		break;
	}
	if (cur_dir) {
		sys_chdir(cur_dir);
		free(cur_dir);
	}

	/* setup memory banks */
	switch (mem_model) {
	case ZXM_48K:
		zxbnk[0] = zxrom;
		zxbnk[1] = zxram;
		zxbnk[2] = zxram + 16 * 1024;
		zxbnk[3] = zxram + 32 * 1024;
		zxscr = zxram;
		break;

	case ZXM_128K:
	case ZXM_PLUS2:
		zxbnk[0] = zxrom;
		zxbnk[1] = zxram + 5 * 0x4000;
		zxbnk[2] = zxram + 2 * 0x4000;
		zxbnk[3] = zxram + 7 * 0x4000;
		zxscr = zxram + 5 * 0x4000;
		break;

	case ZXM_PLUS2A:
	case ZXM_PLUS3:
		zxbnk[0] = zxrom;
		zxbnk[1] = zxram + 5 * 0x4000;
		zxbnk[2] = zxram + 2 * 0x4000;
		zxbnk[3] = zxram + 7 * 0x4000;
		zxscr = zxram + 5 * 0x4000;
		break;

	case ZXM_ZX81: /* 8k pages */
		zxbnk[0] = zxrom;
		zxbnk[1] = zxram;
		zxbnk[2] = zxram + 16 * 1024;
		zxbnk[3] = zxram + 32 * 1024;
		zxscr = zxram;
		break;
	}

	gzx_notify_mode_48k(has_banksw == false);
	return 0;
}

static int rom_load(char *fname, int bank, uint16_t banksize)
{
	FILE *f;

	f = fopen(fname, "rb");

	if (f == NULL) {
		printf("rom_load: cannot open file '%s'\n", fname);
		return -1;
	}

	if (fread(zxrom + (bank * banksize), 1, banksize, f) != banksize) {
		printf("rom_load: unexpected end of file\n");
		fclose(f);
		return -1;
	}

	fclose(f);
	return 0;
}

static int spec_rom_load(char *fname, int bank)
{
	return rom_load(fname, bank, 0x4000);
}

int gfxrom_load(char *fname, unsigned bank)
{
	FILE *f;
	unsigned u, v, w;
	uint8_t buf[8];
	uint8_t b;
	char *cur_dir;
	size_t nr;

	cur_dir = sys_getcwd(NULL, 0);
	if (start_dir)
		sys_chdir(start_dir);

	f = fopen(fname, "rb");
	if (f == NULL) {
		printf("gfxrom_load: cannot open file '%s'\n", fname);
		sys_chdir(cur_dir);
		return -1;
	}
	for (u = 0; u < 16384; u++) {
		nr = fread(buf, 1, 8, f);
		if (nr != 8) {
			printf("gfxrom_load: error loading file '%s'\n", fname);
			sys_chdir(cur_dir);
			return -1;
		}

		for (v = 0; v < 8; v++) {
			b = 0;
			for (w = 0; w < 8; w++) {
				if (buf[w] & (1 << v))
					b |= (1 << w);
			}
			gfxrom[v][bank * 0x4000 + u] = b;
		}
	}

	fclose(f);
	sys_chdir(cur_dir);

	return 0;
}
