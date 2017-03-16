#ifndef _FILEUTIL_H
#define _FILEUTIL_H

#include <stdio.h>
#include "intdef.h"

/* useful file i/o routines */
unsigned fgetu8(FILE *f);
void fungetu8(FILE *f, u8 c);
unsigned fgetu16le(FILE *f);
unsigned fgetu32le(FILE *f);
unsigned fgetu16be(FILE *f);
unsigned fgetu32be(FILE *f);

signed   fgets16le(FILE *f);
unsigned fgetu24le(FILE *f);

unsigned long fsize(FILE *f);

void fputu8(FILE *f, u8 val);
void fputu16le(FILE *f, u16 val);
void fputu16be(FILE *f, u16 val);

#endif
