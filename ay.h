#ifndef _AY_H
#define _AY_H

#include "global.h"

#define AY_REG_SEL_PORT   0xfffd
#define AY_REG_READ_PORT  0xfffd
#define AY_REG_WRITE_PORT 0xbffd

void ay_reg_select(u8 regn);
void ay_reg_write(u8 val);
u8   ay_reg_read(void);

int ay_init(unsigned long d_t_states);
void ay_reset(void);
int ay_get_sample(void);

/* snapshot writers need to read these */
extern u8 ay_reg[16];
extern int ay_cur_reg;

#endif
