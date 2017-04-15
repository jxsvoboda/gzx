/*
 * GZX - George's ZX Spectrum Emulator
 * ZX Spectrum 128K RS-232 port emulation
 */
#ifndef RS232_H
#define RS232_H

#include <stdint.h>

/* Bits within the RS232 register */
enum {
	rs232_txd = 2
};

typedef enum {
	rs232_idle,
	rs232_sending
} rs232_state_t;

typedef struct {
	/** Z80 T states between successive bits (rel. to baud rate) */
	uint32_t d_clocks;
	/** State */
	rs232_state_t state;
	/** Bit number */
	uint8_t bit_no;
	/** Transfer buffer */
	uint8_t buf;
	/** A symbol was sent */
	void (*sendchar)(void *, uint8_t);
	/** Argument to sendchar callback */
	void *sendchar_arg;
} rs232_t;

extern void rs232_init(rs232_t *, uint32_t);
extern void rs232_write(rs232_t *, uint8_t);

#endif
