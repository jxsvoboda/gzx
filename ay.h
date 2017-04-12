/*
 * GZX - George's ZX Spectrum Emulator
 * AY-3-8912 music chip emulation
 */
#ifndef AY_H
#define AY_H

#include <stdint.h>

extern void ay_reg_select(uint8_t regn);
extern void ay_reg_write(uint8_t val);
extern uint8_t ay_reg_read(void);

extern int ay_init(unsigned long);
extern void ay_reset(void);
extern int ay_get_sample(void);

/* snapshot writers need to read these */
extern uint8_t ay_reg[16];
extern int ay_cur_reg;

#endif
