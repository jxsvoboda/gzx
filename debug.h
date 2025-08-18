/*
 * GZX - George's ZX Spectrum Emulator
 * Integrated debugger
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
#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>
#include <stdint.h>
#include "ui/teline.h"

/** Debugger views */
typedef enum {
	/** Disassembly */
	dbgv_disasm,
	/** Memory */
	dbgv_memory
} dbg_view_t;

/** Debugger edit action */
typedef enum {
	/** Go to */
	dbge_goto,
	/** Enter */
	dbge_enter
} dbg_edit_t;

enum {
	dbgv_first = dbgv_disasm,
	dbgv_limit = dbgv_memory + 1
};

typedef struct {
	/** Which debugger view is focused */
	dbg_view_t focus;
	/** Which debugger edit function is active */
	dbg_edit_t edit;

	uint16_t hex_base;
	uint16_t instr_base;

	int ic_ln; /* instruction cursor line number */
	int mem_off; /* memory dump cursor offset */

	/** Drop to debugger after executing an instruction */
	bool itrap_enabled;
	/** Interrupts were already locked during previous instruction */
	bool prev_int_lock;
	/** Drop to debugger when dbg_stop-addr is reached */
	bool stop_enabled;
	/** When run upto cursor is selected, the address is stored here */
	uint16_t stop_addr;

	/** Text input */
	teline_t teline;

	bool exit;
} debugger_t;

void debugger_run(debugger_t *);

#endif
