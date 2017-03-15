/*
  GZX - George's ZX Spectrum Emulator
  I/O recording (output only)
*/

#include <stdio.h>
#include <stdlib.h>
#include "fileutil.h"
#include "global.h"
#include "iorec.h"

static char *iorec_file = "out.ior";
static FILE *iorf;
static unsigned long last_tick = 0;
static int have_tick;

void iorec_enable(void) {
  if (iorf != NULL)
    return;
  
  printf("I/O recording start to %s\n", iorec_file);
  iorf = fopen(iorec_file, "wt");
  if (iorf == NULL) {
    printf("Failed opening file.\n");
    return;
  }
  
  have_tick = 0;
}

void iorec_disable(void)
{
  int rc;
  
  if (iorf == NULL)
    return;
  
  printf("I/O recording stop\n");
  
  rc = fclose(iorf);
  if (rc < 0)
    printf("Failed closing file.\n");
  iorf = NULL;
}

static void fputvlc(FILE *f, unsigned long val)
{
    u8 cur;
    u8 cont;

    do {
      cur = val & 0x7f;
      cont = (val & ~0x7f) != 0 ? 0x80 : 0x00;
      fputu8(f, cur | cont);
      val = val >> 7;
    } while (val != 0);
}

void iorec_out(unsigned long tick, u16 addr, u8 data)
{
  if (iorf == NULL)
    return;
  
  if (!have_tick) {
    last_tick = tick;
    have_tick = 1;
  }
  
  printf("%lu,0x%04x,0x%02x\n", tick - last_tick, addr, data);
  fputvlc(iorf, tick - last_tick);
  fputu16le(iorf, addr);
  fputu8(iorf, data);
  
  last_tick = tick;
}
