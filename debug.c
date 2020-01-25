/*
 * GZX - George's ZX Spectrum Emulator
 * Integrated Debugger
 *
 * Copyright (c) 1999-2021 Jiri Svoboda
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
#include <stdint.h>
#include <string.h>
#include "debug.h"
#include "gzx.h"
#include "memio.h"
#include "mgfx.h"
#include "zx_scr.h"
#include "z80.h"
#include "disasm.h"
#include "sys_all.h"

#define MK_PAIR(hi,lo) ( (((uint16_t)(hi)) << 8) | (lo) )

#define HEX_CY 18
#define STACK_CY 16
#define INSTR_CY 9

#define INSTR_LINES 6

static void dreg(char *name, uint16_t value)
{
	char buf[16];

	fgc = 7;
	gputs(name);
	gputc(':');

	fgc = 5;
	snprintf(buf, 16, "%04X", value);
	gputs(buf);
}

static void dreg8(char *name, uint16_t value)
{
	char buf[16];

	fgc = 7;
	gputs(name);
	gputc(':');

	fgc = 5;
	snprintf(buf, 16, "%02X", value);
	gputs(buf);
}

static void dflag(char *name, int value)
{
	char buf[8];

	fgc = 7;
	gputs(name);

	fgc = 5;
	snprintf(buf, 8, "%d ", value);
	gputs(buf);
}

static void d_regs(void)
{

	mgfx_fillrect(0, 0, scr_xs - 1, scr_ys - 1, 0);

	bgc = 0;

	fgc = 7;
	gmovec(scr_xs / 16 - (strlen("Debugger") / 2), 0);
	gputs("Debugger");

	fgc = 5;

	gmovec(1, 2);
	dreg("AF", MK_PAIR(cpus.r[rA], cpus.F));
	gmovec(1, 3);
	dreg("BC", MK_PAIR(cpus.r[rB], cpus.r[rC]));
	gmovec(1, 4);
	dreg("DE", MK_PAIR(cpus.r[rD], cpus.r[rE]));
	gmovec(1, 5);
	dreg("HL", MK_PAIR(cpus.r[rH], cpus.r[rL]));

	gmovec(9, 2);
	dreg("AF'", MK_PAIR(cpus.r_[rA], cpus.F_));
	gmovec(9, 3);
	dreg("BC'", MK_PAIR(cpus.r_[rB], cpus.r_[rC]));
	gmovec(9, 4);
	dreg("DE'", MK_PAIR(cpus.r_[rD], cpus.r_[rE]));
	gmovec(9, 5);
	dreg("HL'", MK_PAIR(cpus.r_[rH], cpus.r_[rL]));

	gmovec(18, 2);
	dreg("IX", cpus.IX);
	gmovec(18, 3);
	dreg("IY", cpus.IY);
	gmovec(18, 4);
	dreg("IR", MK_PAIR(cpus.I, cpus.R));
	gmovec(18, 5);
	dreg("SP", cpus.SP);

	gmovec(26, 2);
	dflag("IFF1:", cpus.IFF1);
	gmovec(26, 3);
	dflag("IFF2:", cpus.IFF2);
	gmovec(26, 4);
	dflag("IM:  ", cpus.int_mode);
	gmovec(26, 5);
	dflag("HLT: ", cpus.halted);

	gmovec(33, 2);
	dflag("ILCK:", cpus.int_lock);
	gmovec(33, 3);
	dreg8("Pg", page_reg);

	gmovec(1, 7);
	dflag("S:", (cpus.F & fS) != 0);
	dflag("Z:", (cpus.F & fZ) != 0);
	dflag("H:", (cpus.F & fHC) != 0);
	dflag("PV:", (cpus.F & fPV) != 0);
	dflag("N:", (cpus.F & fN) != 0);
	dflag("C:", (cpus.F & fC) != 0);

	gmovec(30, 7);
	dreg("PC", cpus.PC);
}

static void d_stack(void)
{
	char buf[5];
	int i;

	gmovec(1, STACK_CY);
	fgc = 7;
	gputs("Stack:");

	fgc = 5;
	for (i = 0; i < 6; i++) {
		snprintf(buf, 6, " %04X", zx_memget16(cpus.SP + 2 * i));
		gputs(buf);
	}
}

static void d_hex(debugger_t *dbg)
{
	int i, j;
	char buf[16];
	uint8_t b;

	for (i = 0; i < 6; i++) {
		fgc = 7;
		snprintf(buf, 16, "%04X:", dbg->hex_base + 8 * i);
		gmovec(1, HEX_CY + i);
		gputs(buf);

		fgc = 5;
		for (j = 0; j < 8; j++) {
			b = zx_memget8(dbg->hex_base + 8 * i + j);
			gputc(b);
		}
		for (j = 0; j < 8; j++) {
			b = zx_memget8(dbg->hex_base + 8 * i + j);
			snprintf(buf, 16, "%02X", b);
			gmovec(16 + 3 * j, HEX_CY + i);
			gputs(buf);
		}
	}
}

static void d_instr(debugger_t *dbg)
{
	int i;
	uint16_t xpos, c;
	char buf[16];

	disasm_org = xpos = dbg->instr_base;

	for (i = 0; i < INSTR_LINES; i++) {
		bgc = 0;
		if (disasm_org == cpus.PC)
			bgc |= 2;
		if (dbg->ic_ln == i)
			bgc |= 1;
		fgc = 7;
		snprintf(buf, 16, "%04X:", disasm_org & 0xffff);
		gmovec(1, INSTR_CY + i);
		gputs(buf);

		disasm_instr();

		fgc = 5;
		for (c = xpos; c != disasm_org; c++) {
			snprintf(buf, 16, "%02X", zx_memget8(c));
			gputs(buf);
		}

		fgc = 4;
		gmovec(15, INSTR_CY + i);
		gputs(disasm_buf);

		xpos = disasm_org;
	}
	bgc = 0;
}

static void instr_next(debugger_t *dbg)
{
	disasm_org = dbg->instr_base;
	disasm_instr();
	dbg->instr_base = disasm_org;
}

/*
 * How to find the preceding instruction:
 * Go BKTRACE bytes backward and hope that going forward from there
 * we will sync. before reaching the current position
 * We then save the position of the preceding instruction
 */

#define BKTRACE 8

static void instr_prev(debugger_t *dbg)
{
	uint16_t c, last;

	disasm_org = dbg->instr_base - BKTRACE;
	c = 0;
	do {
		last = disasm_org;
		disasm_instr();
		c++;
	} while (c < BKTRACE && disasm_org != dbg->instr_base);

	if (c == BKTRACE) {
		dbg->instr_base--;
		return;
	}
	dbg->instr_base = last;
}

static void d_trace(debugger_t *dbg)
{
	dbg->itrap_enabled = true;
	dbg->exit = true;
}

static void d_run_upto(debugger_t *dbg, uint16_t addr)
{
	dbg->stop_enabled = true;
	dbg->stop_addr = addr;
	dbg->exit = true;
}

static void d_stepover(debugger_t *dbg)
{
	uint8_t b;

	b = zx_memget8(cpus.PC);
	if (b == 0xCD || (b & 0xC7) == 0xC4) {
		/* CALL or CALL cond */
		disasm_org = cpus.PC;
		disasm_instr();
		d_run_upto(dbg, disasm_org);
	} else {
		d_trace(dbg);
	}
}

static void d_to_cursor(debugger_t *dbg)
{
	int i;

	disasm_org = dbg->instr_base;
	for (i = 0; i < dbg->ic_ln; i++)
		disasm_instr();

	d_run_upto(dbg, disasm_org);
}

static void d_view_scr(void)
{
	wkey_t k;

	zx_scr_disp_fast();
	mgfx_updscr();
	while (1) {
		mgfx_input_update();
		if (w_getkey(&k)) {
			if (k.press && k.key == WKEY_ESC)
				break;
		}
	}
}

static void curs_up(debugger_t *dbg)
{
	if (dbg->ic_ln > 0)
		dbg->ic_ln--;
	else
		instr_prev(dbg);
}

static void curs_down(debugger_t *dbg)
{
	if (dbg->ic_ln < INSTR_LINES - 1)
		dbg->ic_ln++;
	else
		instr_next(dbg);
}

static void curs_pgup(debugger_t *dbg)
{
	int i;
	for (i = 0; i < INSTR_LINES - 1; i++)
		curs_up(dbg);
}

static void curs_pgdown(debugger_t *dbg)
{
	int i;
	for (i = 0; i < INSTR_LINES - 1; i++)
		curs_down(dbg);
}

void debugger_run(debugger_t *dbg)
{
	wkey_t k;

	dbg->stop_enabled = false;
	dbg->itrap_enabled = false;

	dbg->instr_base = cpus.PC;
	dbg->ic_ln = 0;
	dbg->exit = false;

	while (!dbg->exit) {
		mgfx_selln(3);
		d_regs();
		d_hex(dbg);
		d_stack();
		d_instr(dbg);
		mgfx_updscr();
		do {
			mgfx_input_update();
			sys_usleep(1000);
		} while (!w_getkey(&k));

		if (k.press)
			switch (k.key) {
			case WKEY_ESC:
				return;

			case WKEY_ENTER:
				//fn_line.buf[fn_line.len]=0;
				//zx_save_snap(fn_line.buf);
				return;

			case WKEY_UP:
				curs_up(dbg);
				break;
			case WKEY_DOWN:
				curs_down(dbg);
				break;
			case WKEY_PGUP:
				curs_pgup(dbg);
				break;
			case WKEY_PGDN:
				curs_pgdown(dbg);
				break;
			case WKEY_LEFT:
				dbg->instr_base--;
				break;
			case WKEY_RIGHT:
				dbg->instr_base++;
				break;
			case WKEY_HOME:
				dbg->instr_base -= 256;
				break;
			case WKEY_END:
				dbg->instr_base += 256;
				break;

			case WKEY_F7:
				d_trace(dbg);
				break;
			case WKEY_F8:
				d_stepover(dbg);
				break;
			case WKEY_F9:
				d_to_cursor(dbg);
				break;
			case WKEY_F11:
				d_view_scr();
				break;

			default:
				//teline_key(&fn_line,&k);
				break;
			}
	}
}
