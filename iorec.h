#ifndef IOREC_H
#define IOREC_H

#include "intdef.h"

void iorec_enable(void);
void iorec_disable(void);
void iorec_out(unsigned long, u16, u8);

#endif
