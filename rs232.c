/*
 * GZX - George's ZX Spectrum Emulator
 * ZX Spectrum 128K RS-232 port emulation
 */
#include <stdio.h>
#include "rs232.h"

void rs232_init(rs232_t *rs, uint32_t d_clocks)
{
	rs->d_clocks = d_clocks;
	rs->state = rs232_idle;
}

void rs232_write(rs232_t *rs, uint8_t val)
{
	uint8_t b;

	b = (val >> rs232_txd) & 0x1;

	if (rs->state == rs232_idle && (b == 0x0)) {
		rs->state = rs232_sending;
		rs->bit_no = 0;
		rs->buf = 0x0;
	} else if (rs->state == rs232_sending) {
		rs->buf = (rs->buf >> 1) | (b << 7);
		if (++rs->bit_no >= 8) {
			rs->state = rs232_idle;
			if (rs->sendchar != NULL)
				rs->sendchar(rs->sendchar_arg, rs->buf);
		}
	}
}
