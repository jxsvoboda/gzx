#ifndef AY_H
#define AY_H

#include "intdef.h"

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
