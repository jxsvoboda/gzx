/*
 * Z80 core dependencies
 *
 * Functions required by a Z80 core.
 */

#ifndef Z80DEP_H
#define Z80DEP_H

#include "intdef.h"

#define PROGMEM
#define pgm_read_ptr(x) (*(x))

u8 z80_memget8(u16 addr);
void z80_memset8(u16 addr, u8 val);

void z80_out8(u16 addr, u8 val);
u8 z80_in8(u16 addr);

#endif
