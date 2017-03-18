/*
 * Z80 core dependencies
 */

#include "memio.h"
#include "z80dep.h"

u8 z80_memget8(u16 addr)
{
	return zx_memget8(addr);
}

void z80_memset8(u16 addr, u8 val)
{
	zx_memset8(addr, val);
}

u8 z80_in8(u16 a)
{
	return zx_in8(a);
}

void z80_out8(u16 addr, u8 val)
{
	zx_out8(addr, val);
}
