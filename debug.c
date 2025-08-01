/*
 * GZX - George's ZX Spectrum Emulator
 * Integrated Debugger
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
#define HEX_LINES 6

#define STACK_CY 16

#define INSTR_CY 9
#define INSTR_LINES 6

/** Display 16-bit register.
 *
 * @param name Register name
 * @param value Value
 */
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

/** Display 8-bit register.
 *
 * @param name Register name
 * @param value Value
 */
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

/** Display single-bit register or flag.
 *
 * @param name Register/flag name
 * @param value Value
 */
static void dflag(char *name, int value)
{
	char buf[8];

	fgc = 7;
	gputs(name);

	fgc = 5;
	snprintf(buf, 8, "%d ", value);
	gputs(buf);
}

/** Display register summary. */
static void debbuger_disp_regs(void)
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
	dflag("FA:", cpus.flags_aff);
	gmovec(33, 4);
	dreg8("W", cpus.W);
	if (has_epg) {
		gmovec(33, 5);
		dreg("P", (page_reg << 8) | epg_reg);
	} else if (has_banksw) {
		gmovec(33, 5);
		dreg8("Pg", page_reg);
	}

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

/** Display a couple of entries from the top of the stack. */
static void debugger_disp_stack(void)
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

/** Display memory dump. */
static void debugger_disp_memdump(debugger_t *dbg)
{
	int i, j;
	char buf[16];
	uint8_t b;

	for (i = 0; i < HEX_LINES; i++) {
		fgc = 7;
		snprintf(buf, 16, "%04X:", (dbg->hex_base + 8 * i) & 0xffff);
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
			if (dbg->focus == dbgv_memory)
				bgc = 8 * i + j == dbg->mem_off ? 1 : 0;
			gmovec(16 + 3 * j, HEX_CY + i);
			gputs(buf);
		}

		bgc = 0;
	}
}

/** Display instruction code disassembly. */
static void debugger_disp_instr(debugger_t *dbg)
{
	int i;
	uint16_t xpos, c;
	char buf[16];

	disasm_org = xpos = dbg->instr_base;

	for (i = 0; i < INSTR_LINES; i++) {
		bgc = 0;
		if (disasm_org == cpus.PC)
			bgc |= 2;
		if (dbg->ic_ln == i && dbg->focus == dbgv_disasm)
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

/** Move disassembly window start address one instruction further.
 *
 * @param dbg Debugger
 */
static void instr_next(debugger_t *dbg)
{
	disasm_org = dbg->instr_base;
	disasm_instr();
	dbg->instr_base = disasm_org;
}

#define BKTRACE 8

/** Move disassembly window start address one instruction back.
 *
 * @param dbg Debugger
 */
static void instr_prev(debugger_t *dbg)
{
	uint16_t c, last;

	/*
	 * How we find the preceding instruction:
	 * Go BKTRACE bytes backward and hope that going forward from there
	 * we will sync before reaching the current position
	 * We then save the position of the preceding instruction
	 */

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

/** Trace into (i.e. single step instructions)
 *
 * @param dbg Debugger
 */
static void debugger_trace(debugger_t *dbg)
{
	dbg->itrap_enabled = true;
	dbg->exit = true;
}

/** Continue running until the specified address is reached
 *
 * @param dbg Debugger
 * @param addr Address where to stop
 */
static void debugger_run_upto(debugger_t *dbg, uint16_t addr)
{
	dbg->stop_enabled = true;
	dbg->stop_addr = addr;
	dbg->exit = true;
}

/** Step over function calls.
 *
 * @param dbg Debugger
 */
static void debugger_stepover(debugger_t *dbg)
{
	uint8_t b;

	b = zx_memget8(cpus.PC);
	if (b == 0xCD || (b & 0xC7) == 0xC4) {
		/* CALL or CALL cond */
		disasm_org = cpus.PC;
		disasm_instr();
		debugger_run_upto(dbg, disasm_org);
	} else {
		debugger_trace(dbg);
	}
}

/** Go to cursor.
 *
 * Run until the address under the cursor is reached.
 *
 * @param dbg Debugger
 */
static void debugger_to_cursor(debugger_t *dbg)
{
	int i;

	disasm_org = dbg->instr_base;
	for (i = 0; i < dbg->ic_ln; i++)
		disasm_instr();

	debugger_run_upto(dbg, disasm_org);
}

/** View spectrum screen without quitting the debugger. */
static void debugger_view_scr(void)
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

/** Move cursor one line up.
 *
 * @param dbg Debugger
 */
static void debugger_curs_up(debugger_t *dbg)
{
	switch (dbg->focus) {
	case dbgv_disasm:
		if (dbg->ic_ln > 0)
			dbg->ic_ln--;
		else
			instr_prev(dbg);
		break;
	case dbgv_memory:
		if (dbg->mem_off >= 8)
			dbg->mem_off -= 8;
		else
			dbg->hex_base -= 8;
		break;
	}
}

/** Move cursor one line down.
 *
 * @param dbg Debugger
 */
static void debugger_curs_down(debugger_t *dbg)
{
	switch (dbg->focus) {
	case dbgv_disasm:
		if (dbg->ic_ln < INSTR_LINES - 1)
			dbg->ic_ln++;
		else
			instr_next(dbg);
		break;
	case dbgv_memory:
		if (dbg->mem_off < (HEX_LINES - 1) * 8)
			dbg->mem_off += 8;
		else
			dbg->hex_base += 8;
		break;
	}
}

/** Move cursor one page up.
 *
 * @param dbg Debugger
 */
static void debugger_curs_pgup(debugger_t *dbg)
{
	int i;

	switch (dbg->focus) {
	case dbgv_disasm:
		for (i = 0; i < INSTR_LINES - 1; i++)
			debugger_curs_up(dbg);
		break;
	case dbgv_memory:
		for (i = 0; i < HEX_LINES - 1; i++)
			debugger_curs_up(dbg);
		break;
	}
}

static void debugger_curs_pgdown(debugger_t *dbg)
{
	int i;

	switch (dbg->focus) {
	case dbgv_disasm:
		for (i = 0; i < INSTR_LINES - 1; i++)
			debugger_curs_down(dbg);
		break;
	case dbgv_memory:
		for (i = 0; i < HEX_LINES - 1; i++)
			debugger_curs_down(dbg);
		break;
	}
}

/** Move cursor left.
 *
 * @param dbg Debugger
 */
static void debugger_curs_left(debugger_t *dbg)
{
	switch (dbg->focus) {
	case dbgv_disasm:
		dbg->instr_base--;
		break;
	case dbgv_memory:
		if (dbg->mem_off > 0) {
			dbg->mem_off--;
		} else {
			dbg->mem_off += 7;
			dbg->hex_base -= 8;
		}
		break;
	}
}

/** Move cursor right.
 *
 * @param dbg Debugger
 */
static void debugger_curs_right(debugger_t *dbg)
{
	switch (dbg->focus) {
	case dbgv_disasm:
		dbg->instr_base++;
		break;
	case dbgv_memory:
		if (dbg->mem_off < HEX_LINES * 8 - 1) {
			dbg->mem_off++;
		} else {
			dbg->hex_base += 8;
			dbg->mem_off -= 7;
		}
		break;
	}
}

/** Move cursor home.
 *
 * @param dbg Debugger
 */
static void debugger_curs_home(debugger_t *dbg)
{
	switch (dbg->focus) {
	case dbgv_disasm:
		dbg->instr_base -= 256;
		break;
	case dbgv_memory:
		dbg->hex_base -= 256;
		break;
	}
}

/** Move cursor to the end.
 *
 * @param dbg Debugger
 */
static void debugger_curs_end(debugger_t *dbg)
{
	switch (dbg->focus) {
	case dbgv_disasm:
		dbg->instr_base += 256;
		break;
	case dbgv_memory:
		dbg->hex_base += 256;
		break;
	}
}

/** Process debugger key without modifiers.
 *
 * @param dbg Debugger
 * @param k Key
 */
static void debugger_key_unmod(debugger_t *dbg, wkey_t k)
{
	switch (k.key) {
	case WKEY_ESC:
		dbg->exit = true;
		break;

	case WKEY_ENTER:
		dbg->exit = true;
		break;

	case WKEY_TAB:
		if (++dbg->focus >= (dbg_view_t)dbgv_limit)
			dbg->focus = dbgv_first;
		break;

	case WKEY_UP:
		debugger_curs_up(dbg);
		break;
	case WKEY_DOWN:
		debugger_curs_down(dbg);
		break;
	case WKEY_PGUP:
		debugger_curs_pgup(dbg);
		break;
	case WKEY_PGDN:
		debugger_curs_pgdown(dbg);
		break;
	case WKEY_LEFT:
		debugger_curs_left(dbg);
		break;
	case WKEY_RIGHT:
		debugger_curs_right(dbg);
		break;
	case WKEY_HOME:
		debugger_curs_home(dbg);
		break;
	case WKEY_END:
		debugger_curs_end(dbg);
		break;
	case WKEY_F7:
		debugger_trace(dbg);
		break;
	case WKEY_F8:
		debugger_stepover(dbg);
		break;
	case WKEY_F9:
		debugger_to_cursor(dbg);
		break;
	case WKEY_F11:
		debugger_view_scr();
		break;
	default:
		break;
	}
}

/** Display debuger.
 *
 * @param dbg Debugger
 */
static void debugger_display(debugger_t *dbg)
{
	mgfx_selln(3);

	debbuger_disp_regs();
	debugger_disp_memdump(dbg);
	debugger_disp_stack();
	debugger_disp_instr(dbg);
	mgfx_updscr();
}

/** Enter debugger.
 *
 * @param dbg Debugger
 */
void debugger_run(debugger_t *dbg)
{
	wkey_t k;

	dbg->stop_enabled = false;
	dbg->itrap_enabled = false;

	dbg->focus = dbgv_disasm;
	dbg->instr_base = cpus.PC;
	dbg->ic_ln = 0;
	dbg->exit = false;

	while (!dbg->exit) {
		debugger_display(dbg);

		do {
			mgfx_input_update();
			sys_usleep(1000);
		} while (!w_getkey(&k));

		if (k.press)
			debugger_key_unmod(dbg, k);
	}
}
