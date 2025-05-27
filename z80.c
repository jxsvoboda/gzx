/*
 * GZX - George's ZX Spectrum Emulator
 * Z80 CPU emulation
 *
 * Copyright (c) 1999-2025 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
  Flag computations for most instructions should work
  except block I/O instructions and undocumented 5,3 flags
  for BIT (HL)
  
  some games known not to work:
    belegost (dies)
    
  !spemu		not implemented right in spectemu
  
  to do:
    sooner
    - flags for I/O block instructions
    later
    - BIT (HL) - vlajky 3,5
    - timing of undocumented instructions
*/

#include <stdint.h>
#include <stdlib.h>
#include "z80.h"
#include "z80dep.h"

static int z80_readinstr(void);
static void z80_check_nmi(void);
static void z80_check_int(void);

static uint8_t opcode;
z80s cpus;
/** CPU state used to read memory addresses from. Used by Spec256. */
z80s *rcpus = &cpus;
static uint8_t cbop;
unsigned long z80_clock;

/* debugging & statistics */
unsigned long uoc;	/* unsupported opcode counter */
unsigned long smc;	/* stray modifier counter */
static uint8_t prefix1,prefix2;

static void (*const *ei_tab)(void);

#ifndef NO_Z80STAT
static unsigned stat_tab[7][256];
static int stat_i;
#endif

/* fast flag computation lookup table */
static uint8_t ox_tab[256]; /* OR,XOR and more */

static uint16_t z80_memget16(uint16_t addr) {
  return (uint16_t)z80_memget8(addr)+(((uint16_t)z80_memget8(addr+1))<<8);
}

static uint16_t z80_imemget16(uint16_t addr) {
  return (uint16_t)z80_imemget8(addr)+(((uint16_t)z80_imemget8(addr+1))<<8);
}

static void z80_memset16(uint16_t addr, uint16_t val) {
  z80_memset8(addr, val & 0xff);
  z80_memset8(addr+1, val >> 8);
}

static inline void z80_clock_inc(uint8_t inc)
{
#ifndef NO_Z80CLOCK
	z80_clock += inc;
#endif
}

/* returns the signed value of the byte: 0..127 ->0..127
                                         128..255 ->-128..-1 */
static int u8sval(uint8_t u) {
  if(u<0x80) return u;
    else return (int)(u&0x7f)-128;
}

static int u16sval(uint16_t u) {
  if(u<0x8000) return u;
    else return (int)(u&0x7fff)-32768;
}

/********************** flags calculation ****************************/

/*  these functions return 1 when sign overflow occurs */
/*  (the result would be >127 or <-128) */

static int add_v8(uint8_t a, uint8_t b) {
  int sign_r;
  sign_r=u8sval(a)+u8sval(b);
  return sign_r<-128 || sign_r>127;
}

static int sub_v8(uint8_t a, uint8_t b) {
  int sign_r;
  sign_r=u8sval(a)-u8sval(b);
  return sign_r<-128 || sign_r>127;
}

static int adc_v8(uint8_t a, uint8_t b, uint8_t c) {
  int sign_r;
  sign_r=u8sval(a)+u8sval(b)+u8sval(c);
  return sign_r<-128 || sign_r>127;
}

static int sbc_v8(uint8_t a, uint8_t b, uint8_t c) {
  int sign_r;
  sign_r=u8sval(a)-u8sval(b)-c;
  return sign_r<-128 || sign_r>127;
}

static int adc_v16(uint16_t a, uint16_t b, uint16_t c) {
  int sign_r;
  sign_r=u16sval(a)+u16sval(b)+u16sval(c);
  return sign_r<-32768 || sign_r>32767;
}

static int sbc_v16(uint16_t a, uint16_t b, uint16_t c) {
  int sign_r;
  sign_r=u16sval(a)-u16sval(b)-c;
  return sign_r<-32768 || sign_r>32767;
}

/* calculates an even parity for the given 8-bit number */
static int oddp8(uint8_t x) {
  x^=x>>4;
  x^=x>>2;
  x^=x>>1;
  x^=1;
  
  return x&1;
}

static void setflags(int s, int z, int hc, int pv, int n, int c) {
  if(s>=0) cpus.F = (cpus.F & (fS^0xff)) | (s?fS:0);
  if(z>=0) cpus.F = (cpus.F & (fZ^0xff)) | (z?fZ:0);
  if(hc>=0) cpus.F = (cpus.F & (fHC^0xff)) | (hc?fHC:0);
  if(pv>=0) cpus.F = (cpus.F & (fPV^0xff)) | (pv?fPV:0);
  if(n>=0) cpus.F = (cpus.F & (fN^0xff)) | (n?fN:0);
  if(c>=0) cpus.F = (cpus.F & (fC^0xff)) | (c?fC:0);
}

#ifndef NO_Z80UNDOC

static void setundocflags8(uint8_t res) {
  cpus.F &= fD;		/* leave only documented flags */
  cpus.F |= (res & fU);     /* set undocumented flags */
}

#else

#define setundocflags8(res) ((void)(res))

static void ei_undoc(void)
{
	z80_clock_inc(4);
}

#endif

static void incr_R(uint8_t amount) {
  cpus.R = (cpus.R & 0x80) | ((cpus.R+amount)&0x7f);
}

/**************************** address register access *******************/

/*
 * In GPU case we need to take addresses from the CPU registers as a special
 * case. If GPU is not enabled, rcpus just points to the CPU state.
 */

static uint16_t get_addrBC(void)
{
  return ((uint16_t)rcpus->r[rB] << 8) | rcpus->r[rC];
}

static uint16_t get_addrDE(void)
{
  return ((uint16_t)rcpus->r[rD] << 8) | rcpus->r[rE];
}

static uint16_t get_addrHL(void)
{
  return ((uint16_t)rcpus->r[rH] << 8) | rcpus->r[rL];
}

static uint16_t get_addrIX(void)
{
  return rcpus->IX;
}

static uint16_t get_addrIY(void)
{
  return rcpus->IY;
}

/**************************** operand access ***************************/

/* returns (HL)(8) */
static uint8_t _iHL8(void) {
  return z80_memget8(get_addrHL());
}

/* returns (BC) */
static uint8_t _iBC8(void) {
  return z80_memget8(get_addrBC());
}

/* returns (DE) */
static uint8_t _iDE8(void) {
  return z80_memget8(get_addrDE());
}

/* returns (IX+N) */
static uint8_t _iIXN8(uint16_t N) {
  return z80_memget8(get_addrIX()+u8sval(N));
}

/* returns (IY+N) */
static uint8_t _iIYN8(uint16_t N) {
  return z80_memget8(get_addrIY()+u8sval(N));
}

/* (IX+N) <- val*/
static void s_iIXN8(uint16_t N, uint8_t val) {
  z80_memset8(get_addrIX()+u8sval(N),val);
}

/* (IY+N) <- val*/
static void s_iIYN8(uint16_t N, uint8_t val) {
  z80_memset8(get_addrIY()+u8sval(N),val);
}


/* (HL) <- val */
static void s_iHL8(uint8_t val) {
  z80_memset8(get_addrHL(),val);
}

/* (BC) <- val */
static void s_iBC8(uint8_t val) {
  z80_memset8(get_addrBC(),val);
}

/* (DE) <- val */
static void s_iDE8(uint8_t val) {
  z80_memset8(get_addrDE(),val);
}

/* returns (SP)(16-bits) */
static uint16_t _iSP16(void) {
  return z80_memget16(cpus.SP);
}

/* (SP)(16-bits) <- val */
static void s_iSP16(uint16_t val) {
  z80_memset16(cpus.SP,val);
}

static uint16_t getAF(void) {
  return ((uint16_t)cpus.r[rA] << 8)|(uint16_t)cpus.F;
}

static uint16_t getBC(void) {
  return ((uint16_t)cpus.r[rB] << 8)|(uint16_t)cpus.r[rC];
}

static uint16_t getDE(void) {
  return ((uint16_t)cpus.r[rD] << 8)|(uint16_t)cpus.r[rE];
}

static uint16_t getHL(void) {
  return ((uint16_t)cpus.r[rH] << 8)|(uint16_t)cpus.r[rL];
}

static uint16_t getAF_(void) {
  return ((uint16_t)cpus.r_[rA] << 8)|(uint16_t)cpus.F_;
}

static uint16_t getBC_(void) {
  return ((uint16_t)cpus.r_[rB] << 8)|(uint16_t)cpus.r_[rC];
}

static uint16_t getDE_(void) {
  return ((uint16_t)cpus.r_[rD] << 8)|(uint16_t)cpus.r_[rE];
}

static uint16_t getHL_(void) {
  return ((uint16_t)cpus.r_[rH] << 8)|(uint16_t)cpus.r_[rL];
}

static void setAF(uint16_t val) {
  cpus.r[rA]=val>>8;
  cpus.F=val & 0xff;
}

static void setBC(uint16_t val) {
  cpus.r[rB]=val>>8;
  cpus.r[rC]=val & 0xff;
}

static void setDE(uint16_t val) {
  cpus.r[rD]=val>>8;
  cpus.r[rE]=val & 0xff;
}

static void setHL(uint16_t val) {
  cpus.r[rH]=val>>8;
  cpus.r[rL]=val & 0xff;
}

uint16_t z80_getAF(void)
{
	return getAF();
}

uint16_t z80_getBC(void)
{
	return getBC();
}

uint16_t z80_getDE(void)
{
	return getDE();
}

uint16_t z80_getHL(void)
{
	return getHL();
}

uint16_t z80_getAF_(void)
{
	return getAF_();
}

uint16_t z80_getBC_(void)
{
	return getBC_();
}

uint16_t z80_getDE_(void)
{
	return getDE_();
}

uint16_t z80_getHL_(void)
{
	return getHL_();
}

static uint8_t z80_iget8(void) {
  uint8_t tmp;

  tmp=z80_imemget8(cpus.PC);
  cpus.PC++;
  return tmp;
}

static uint16_t z80_iget16(void) {
  uint16_t tmp;

  tmp=z80_imemget16(cpus.PC);
  cpus.PC+=2;
  return tmp;
}

/******************* undocumented operand access ************************/

#ifndef NO_Z80UNDOC

static void setIXh(uint8_t val) {
  cpus.IX = (cpus.IX & 0x00ff) | ((uint16_t)val<<8);
}

static void setIYh(uint8_t val) {
  cpus.IY = (cpus.IY & 0x00ff) | ((uint16_t)val<<8);
}

static void setIXl(uint8_t val) {
  cpus.IX = (cpus.IX & 0xff00) | (uint16_t)val;
}

static void setIYl(uint8_t val) {
  cpus.IY = (cpus.IY & 0xff00) | (uint16_t)val;
}

static uint8_t getIXh(void) {
  return cpus.IX>>8;
}

static uint8_t getIYh(void) {
  return cpus.IY>>8;
}

static uint8_t getIXl(void) {
  return cpus.IX&0xff;
}

static uint8_t getIYl(void) {
  return cpus.IY&0xff;
}

#endif

/************************************************************************/
static void _push16(uint16_t val);
/************************************************************************/


void z80_init_tables(void) {
  unsigned u;
  
  for(u=0;u<256;u++) {
    ox_tab[u] = u & (fS | fU1 | fU2);
    if(oddp8(u)) ox_tab[u] |= fPV;
  }
  ox_tab[0] |= fZ;
}

/************************ operations ************************************/

static uint8_t _adc8(uint16_t a, uint16_t b) {
  uint16_t res;
  uint16_t c;
  
  c=(cpus.F&fC)?1:0;

  res=a+b+c;
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (a&0x0f)+(b&0x0f)+c > 0x0f,
	   adc_v8(a,b,c),
	   0,
	   res>0xff);
  setundocflags8(res);
  return res & 0xff;
}

static uint16_t _adc16(uint16_t a, uint16_t b) {
  uint16_t res0,res1,a1,b1,c,c1;
  
  c=((cpus.F&fC)?1:0);

  res0=(a&0xff)+(b&0xff)+ c;
  a1=a>>8; b1=b>>8; c1=((res0>0xff)?1:0);
  res1=a1+b1+c1;
  setflags(res1&0x80,
	   !((res0&0xff)|(res1&0xff)),
	   (a1&0x0f) + (b1&0x0f)>0x0f,
	   adc_v16(a,b,c),
	   0,
	   res1>0xff);
  setundocflags8(res1);
  return (res0&0xff)|((res1&0xff)<<8);
}

/************************************************************************/

static uint8_t _add8(uint16_t a, uint16_t b) {
  uint16_t res;

  res=a+b;
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (a&0x0f)+(b&0x0f) > 0x0f,
	   add_v8(a,b),
	   0,
	   res>0xff);
  setundocflags8(res);
  return res & 0xff;
}

static uint16_t _add16(uint16_t a, uint16_t b) {
  uint16_t res0,res1,a1,b1;

  res0=(a&0xff)+(b&0xff);
  a1=a>>8; b1=b>>8;
  res1=a1+b1+((res0>0xff)?1:0);
  setflags(-1,
	   -1,
	   (a1&0x0f) + (b1&0x0f)>0x0f,
	   -1,
	   0,
	   res1>0xff);
  setundocflags8(res1);
  return (res0&0xff)|((res1&0xff)<<8);
}

/************************************************************************/

static uint8_t _and8(uint8_t a, uint8_t b) {
  uint8_t res;

  res=a&b;
  cpus.F=ox_tab[res]|fHC;
  return res;
}

/************************************************************************/

static uint8_t _bit8(uint8_t a, uint8_t b) {
  uint8_t res;

  res=b & (1<<a);
  cpus.F=(cpus.F&fC)|ox_tab[res]|fHC; /* CF does not change */
/*  setflags(res&0x80,
	   res==0,
	   1,
	   res==0, 
	   0,
	   -1);*/
	
  /* undoc flags are different for different opcodes. see BIT opcodes. */
  return res;
}

/************************************************************************/

static void _call16(uint16_t addr) {
  _push16(cpus.PC);
  cpus.PC=addr;
}

/************************************************************************/

static uint8_t _cp8(uint16_t a, uint16_t b) {
  uint16_t res;

  res=a-b;
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (a&0x0f)-(b&0x0f) < 0, /* I hope */
	   sub_v8(a,b),
	   1,
	   res>0xff);
  setundocflags8(b); /* not from the result! */
  return res & 0xff;
}

/************************************************************************/

static uint8_t _dec8(uint16_t a) {
  uint16_t res;

  res=(a-1)&0xff;
  setflags(res>>7,
	   res==0,
	   (a&0x0f)-1<0, /* I hope */
	   sub_v8(a,1),
	   1,    /* !spemu */
	   -1);
  setundocflags8(res);
  return res;
}

/************************************************************************/

static uint8_t _in8pf(uint16_t a) {
  return z80_in8(a);		/* query ZX */
}

static uint8_t _in8(uint16_t a) {
  uint16_t res;

  res=_in8pf(a)&0xff;
  setflags(res>>7,
	   res==0,
	   0,
	   oddp8(res),
	   0,
	   -1);
  setundocflags8(res);
  return res;
}

/************************************************************************/

static uint8_t _inc8(uint16_t a) {
  uint16_t res;

  res=(a+1)&0xff;
  setflags(res>>7,
	   res==0,
	   (a&0x0f)+1>0x0f,
	   add_v8(a,1),
	   0,		/* !spemu */
	   -1);
  setundocflags8(res);
  return res;
}

/************************************************************************/


static void _jp16(uint16_t addr) {
  cpus.PC=addr;
}

/************************************************************************/

static void _jr8(uint8_t ofs) {
  cpus.PC+=u8sval(ofs);
}

/************************************************************************/

static uint8_t _or8(uint8_t a, uint8_t b) {
  uint8_t res;

  res=a|b;
  cpus.F=ox_tab[res];
  return res;
}

/************************************************************************/

static void _out8(uint16_t addr, uint8_t val) {
  z80_out8(addr,val);			/* pass it to ZX */
}

/************************************************************************/

static void _push16(uint16_t val) {
  cpus.SP-=2;
  z80_memset16(cpus.SP,val);
}

static uint16_t _pop16(void) {
  uint16_t res;

  res=z80_memget16(cpus.SP);
  cpus.SP+=2;
  return res;
}

/************************************************************************/

static uint8_t _res8(uint16_t a, uint16_t b) {
  uint16_t res;

  res=b & ((1<<a)^0xff);
  return res;
}

/************************************************************************/

static uint8_t _rla8(uint8_t a) {
  uint8_t nC,oC;

  nC=a>>7;
  oC=(cpus.F & fC)?1:0;
  a=(a<<1)|oC;
  cpus.F = (cpus.F & ~(fU1|fHC|fU2|fN|fC)) | (a&(fU1|fU2)) | nC;
  return a;
}

static uint8_t _rl8(uint8_t a) {
  uint8_t nC,oC;

  nC=a>>7;
  oC=(cpus.F & fC)?1:0;
  a=(a<<1)|oC;
  cpus.F=ox_tab[a]|nC;
  return a;
}

static uint8_t _rlca8(uint8_t a) {
  uint8_t tmp;

  tmp=a>>7;
  a=(a<<1)|tmp;
  cpus.F = (cpus.F & ~(fU1|fHC|fU2|fN|fC)) | (a&(fU1|fU2)) | tmp;
  return a;
}

static uint8_t _rlc8(uint8_t a) {
  uint8_t tmp;

  tmp=a>>7;
  a=(a<<1)|tmp;
  cpus.F=ox_tab[a]|tmp;
  return a;
}

static uint8_t _rra8(uint8_t a) {
  uint8_t nC,oC;

  nC=a&1;
  oC=(cpus.F & fC)?1:0;
  a=(a>>1)|(oC<<7);
  cpus.F = (cpus.F & ~(fU1|fHC|fU2|fN|fC)) | (a&(fU1|fU2)) | nC;
  return a;
}

static uint8_t _rr8(uint8_t a) {
  uint8_t nC,oC;

  nC=a&1;
  oC=(cpus.F & fC)?1:0;
  a=(a>>1)|(oC<<7);
  cpus.F=ox_tab[a]|nC;
  return a;
}

static uint8_t _rrca8(uint8_t a) {
  uint8_t tmp;

  tmp=a&1;
  a=(a>>1)|(tmp<<7);
  cpus.F = (cpus.F & ~(fU1|fHC|fU2|fN|fC)) | (a&(fU1|fU2)) | tmp;
  return a;
}

static uint8_t _rrc8(uint8_t a) {
  uint8_t tmp;

  tmp=a&1;
  a=(a>>1)|(tmp<<7);
  cpus.F=ox_tab[a]|tmp;
  return a;
}



/************************************************************************/

static uint8_t _sla8(uint8_t a) {
  uint8_t nC;

  nC=a>>7;
  a<<=1;
  cpus.F=ox_tab[a]|nC;
  return a;
}

static uint8_t _sra8(uint8_t a) {
  uint8_t nC;

  nC=a&1;
  a=(a&0x80) | (a>>1);
  cpus.F=ox_tab[a]|nC;
  return a;
}

#ifndef NO_Z80UNDOC

/* Shift left with 1 insertion (sl1), also called sll */
static uint8_t _sll8(uint8_t a) {
  uint8_t nC;

  nC=a>>7;
  a=(a<<1)|0x1;
  cpus.F=ox_tab[a]|nC;
  return a;
}

#endif

static uint8_t _srl8(uint16_t a) {
  uint16_t nC;

  nC=a&1;
  a>>=1;
  cpus.F=ox_tab[a]|nC;
  return a;
}

/************************************************************************/

static uint8_t _sbc8(uint16_t a, uint16_t b) {
  uint16_t res;
  uint16_t c;
  
  c=((cpus.F&fC)!=0);

  res=a-b-c;
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (a&0x0f)-(b&0x0f)-c < 0, /* I hope */
	   sbc_v8(a,b,c),
	   1,
	   res>0xff);
  setundocflags8(res);
  return res & 0xff;
}

static uint16_t _sbc16(uint16_t a, uint16_t b) {
  uint16_t res0,res1,a1,b1,c,c1;
  
  c=((cpus.F&fC)?1:0);

  res0=(a&0xff)-(b&0xff)- c;
  a1=a>>8; b1=b>>8; c1=((res0>0xff)?1:0);
  res1=a1-b1-c1;
  setflags(res1&0x80,
	   !((res0&0xff)|(res1&0xff)),
	   (a1&0x0f) < (b1&0x0f)+c1,
	   sbc_v16(a,b,c),
	   1,
	   res1>0xff);
  setundocflags8(res1);
  return (res0&0xff)|((res1&0xff)<<8);
}

/************************************************************************/

static uint8_t _set8(uint16_t a, uint16_t b) {
  uint16_t res;

  res=b | (1<<a);
  return res;
}

/************************************************************************/


static uint8_t _sub8(uint16_t a, uint16_t b) {
  uint16_t res;

  res=a-b;
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (a&0x0f)-(b&0x0f) < 0, /* I hope */
	   sub_v8(a,b),
	   1,
	   res>0xff);
  setundocflags8(res);
  return res & 0xff;
}

/************************************************************************/

static uint8_t _xor8(uint8_t a, uint8_t b) {
  uint8_t res;

  res=a^b;
  cpus.F=ox_tab[res];
  return res;
}



/************************************************************************/
/************************************************************************/
/********************* documented opcodes *******************************/

static void ei_adc_A_r(void) {
  uint8_t res;

  res=_adc8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_adc_A_N(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_adc8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_adc_A_iHL(void) {
  uint8_t res;

  res=_adc8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_adc_A_iIXN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_adc8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

static void ei_adc_A_iIYN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_adc8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

static void ei_adc_HL_BC(void) {
  uint16_t res;

  res=_adc16(getHL(),getBC());
  setHL(res);

  z80_clock_inc(15);
}

static void ei_adc_HL_DE(void) {
  uint16_t res;

  res=_adc16(getHL(),getDE());
  setHL(res);

  z80_clock_inc(15);
}

static void ei_adc_HL_HL(void) {
  uint16_t res;

  res=_adc16(getHL(),getHL());
  setHL(res);

  z80_clock_inc(15);
}

static void ei_adc_HL_SP(void) {
  uint16_t res;

  res=_adc16(getHL(),cpus.SP);
  setHL(res);

  z80_clock_inc(15);
}

/************************************************************************/

static void ei_add_A_r(void) {
  uint8_t res;

  res=_add8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_add_A_N(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_add8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_add_A_iHL(void) {
  uint8_t res;

  res=_add8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_add_A_iIXN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_add8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

static void ei_add_A_iIYN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_add8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

static void ei_add_HL_BC(void) {
  uint16_t res;

  res=_add16(getHL(),getBC());
  setHL(res);

  z80_clock_inc(11);
}

static void ei_add_HL_DE(void) {
  uint16_t res;

  res=_add16(getHL(),getDE());
  setHL(res);

  z80_clock_inc(11);
}

static void ei_add_HL_HL(void) {
  uint16_t res;

  res=_add16(getHL(),getHL());
  setHL(res);

  z80_clock_inc(11);
}

static void ei_add_HL_SP(void) {
  uint16_t res;

  res=_add16(getHL(),cpus.SP);
  setHL(res);

  z80_clock_inc(11);
}

static void ei_add_IX_BC(void) {
  uint16_t res;

  res=_add16(cpus.IX,getBC());
  cpus.IX=res;

  z80_clock_inc(11);
}

static void ei_add_IX_DE(void) {
  uint16_t res;

  res=_add16(cpus.IX,getDE());
  cpus.IX=res;

  z80_clock_inc(11);
}

static void ei_add_IX_IX(void) {
  uint16_t res;

  res=_add16(cpus.IX,cpus.IX);
  cpus.IX=res;

  z80_clock_inc(11);
}

static void ei_add_IX_SP(void) {
  uint16_t res;

  res=_add16(cpus.IX,cpus.SP);
  cpus.IX=res;

  z80_clock_inc(11);
}

static void ei_add_IY_BC(void) {
  uint16_t res;

  res=_add16(cpus.IY,getBC());
  cpus.IY=res;

  z80_clock_inc(11);
}

static void ei_add_IY_DE(void) {
  uint16_t res;

  res=_add16(cpus.IY,getDE());
  cpus.IY=res;

  z80_clock_inc(11);
}

static void ei_add_IY_IY(void) {
  uint16_t res;

  res=_add16(cpus.IY,cpus.IY);
  cpus.IY=res;

  z80_clock_inc(11);
}

static void ei_add_IY_SP(void) {
  uint16_t res;

  res=_add16(cpus.IY,cpus.SP);
  cpus.IY=res;

  z80_clock_inc(11);
}

/************************************************************************/

static void ei_and_r(void) {
  uint8_t res;

  res=_and8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_and_N(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_and8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_and_iHL(void) {
  uint8_t res;

  res=_and8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_and_iIXN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_and8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

static void ei_and_iIYN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_and8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

/************************************************************************/

static void ei_bit_b_r(void) {
  _bit8((opcode>>3)&0x07,cpus.r[opcode & 0x07]);
  /* Note that undoc flags are set from source operand, not from result! */
  setundocflags8(cpus.r[opcode & 0x07]);
  z80_clock_inc(8);
}

static void ei_bit_b_iHL(void) {
  _bit8((opcode>>3)&0x07,_iHL8());
  /*
   * setundocflags8(cpus.W);
   * W register not implemented yet.
   */

  z80_clock_inc(12);
}

/* DDCB ! */
static void ei_bit_b_iIXN(void) {
  _bit8((opcode>>3)&0x07,_iIXN8(cbop));
  setundocflags8((cpus.IX+u8sval(cbop))>>8); /* weird, huh? */

  z80_clock_inc(16);
}

/* FDCB ! */
static void ei_bit_b_iIYN(void) {

  _bit8((opcode>>3)&0x07,_iIYN8(cbop));
  setundocflags8((cpus.IY+u8sval(cbop))>>8); /* weird, huh? */

  z80_clock_inc(16);
}

/************************************************************************/

static void ei_call_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  _call16(addr);
  z80_clock_inc(17);
}

static void ei_call_C_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(cpus.F & fC) {
    _call16(addr);
    z80_clock_inc(17);
  } else z80_clock_inc(10);
}

static void ei_call_NC_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(!(cpus.F & fC)) {
    _call16(addr);
    z80_clock_inc(17);
  } else z80_clock_inc(10);
}

static void ei_call_M_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(cpus.F & fS) {
    _call16(addr);
    z80_clock_inc(17);
  } else z80_clock_inc(10);
}

static void ei_call_P_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(!(cpus.F & fS)) {
    _call16(addr);
    z80_clock_inc(17);
  } else z80_clock_inc(10);
}

static void ei_call_Z_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(cpus.F & fZ) {
    _call16(addr);
    z80_clock_inc(17);
  } else z80_clock_inc(10);
}

static void ei_call_NZ_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(!(cpus.F & fZ)) {
    _call16(addr);
    z80_clock_inc(17);
  } else z80_clock_inc(10);
}

static void ei_call_PE_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(cpus.F & fPV) {
    _call16(addr);
    z80_clock_inc(17);
  } else z80_clock_inc(10);
}

static void ei_call_PO_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(!(cpus.F & fPV)) {
    _call16(addr);
    z80_clock_inc(17);
  } else z80_clock_inc(10);
}

/************************************************************************/

static void ei_ccf(void) { /* complement carry flag */
  uint8_t nHC;
  
  nHC=(cpus.F&fC)?fHC:0;
  cpus.F= ((cpus.F ^ fC) & ~(fU1|fHC|fU2|fN)) | nHC | (cpus.r[rA]&(fU1|fU2));
  z80_clock_inc(4);
}

/************************************************************************/

static void ei_cp_r(void) {
  _cp8(cpus.r[rA],cpus.r[opcode & 0x07]);
  z80_clock_inc(4);
}

static void ei_cp_N(void) {
  uint8_t op;

  op=z80_iget8();

  _cp8(cpus.r[rA],op);
  z80_clock_inc(7);
}

static void ei_cp_iHL(void) {
  _cp8(cpus.r[rA],_iHL8());
  z80_clock_inc(7);
}

static void ei_cp_iIXN(void) {
  uint8_t op;

  op=z80_iget8();

  _cp8(cpus.r[rA],_iIXN8(op));
  z80_clock_inc(15);
}

static void ei_cp_iIYN(void) {
  uint8_t op;

  op=z80_iget8();

  _cp8(cpus.r[rA],_iIYN8(op));
  z80_clock_inc(15);
}

static void ei_cpd(void) {
  uint8_t a,b,ufr,res;
  uint16_t newBC;

  a=cpus.r[rA];
  b=_iHL8();
  res=a-b;
  setHL(getHL()-1);
  newBC=getBC()-1; setBC(newBC);
  
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (a&0x0f)-(b&0x0f) < 0,
	   newBC!=0,
	   1,
	   -1);
	   
  ufr=cpus.r[rA]-b;
  if(newBC!=0) ufr--;  /* if we turned H flag on, decrease by 1 */
  setundocflags8(((ufr&0x02)<<4)|(ufr&0x08));

  z80_clock_inc(16);
}

static void ei_cpdr(void) {
  uint8_t a,b,ufr,res;
  uint16_t newBC;

  a=cpus.r[rA];
  b=_iHL8();
  res=a-b;
  setHL(getHL()-1);
  newBC=getBC()-1; setBC(newBC);
  
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (a&0x0f)-(b&0x0f) < 0,
	   newBC!=0,
	   1,
	   -1);
	   
  ufr=cpus.r[rA]-b;
  if(newBC!=0) ufr--;  /* if we turned H flag on, decrease by 1 */
  setundocflags8(((ufr&0x02)<<4)|(ufr&0x08));

  if(newBC==0 || (cpus.F & fZ)) {
    z80_clock_inc(16);
  } else {
    z80_clock_inc(21);
    cpus.PC-=2;
  }
}

static void ei_cpi(void) {
  uint8_t a,b,ufr,res;
  uint16_t newBC;

  a=cpus.r[rA];
  b=_iHL8();
  res=a-b;
  setHL(getHL()+1);
  newBC=getBC()-1; setBC(newBC);
  
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (a&0x0f)-(b&0x0f) < 0,
	   newBC!=0,
	   1,
	   -1);
	   
  ufr=cpus.r[rA]-b;
  if(newBC!=0) ufr--;  /* if we turned H flag on, decrease by 1 */
  setundocflags8(((ufr&0x02)<<4)|(ufr&0x08));

  z80_clock_inc(16);
}

static void ei_cpir(void) {
  uint8_t a,b,ufr,res;
  uint16_t newBC;

  a=cpus.r[rA];
  b=_iHL8();
  res=a-b;
  setHL(getHL()+1);
  newBC=getBC()-1; setBC(newBC);
  
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (a&0x0f)-(b&0x0f) < 0,
	   newBC!=0,
	   1,
	   -1);
	   
  ufr=cpus.r[rA]-b;
  if(newBC!=0) ufr--;  /* if we turned H flag on, decrease by 1 */
  setundocflags8(((ufr&0x02)<<4)|(ufr&0x08));

  if(newBC==0 || (cpus.F & fZ)) {
    z80_clock_inc(16);
  } else {
    z80_clock_inc(21);
    cpus.PC-=2;
  }
}

/************************************************************************/

static void ei_cpl(void) { /* A <- cpl(A) ... one's complement */
  cpus.r[rA] ^= 0xff;
  setflags(-1,
           -1,
	   1,
	   -1,
	   1,
	   -1);
  setundocflags8(cpus.r[rA]);
  z80_clock_inc(4);
}


/************************************************************************/

static void ei_daa(void) {
  uint16_t res;
  
  res=cpus.r[rA];
  
  if((cpus.F & fN)==0) {
    if(cpus.F & fC) res += 0x60;
      else if(res>0x99) { res += 0x60; cpus.F|=fC; }
      
    if(cpus.F & fHC) { if ((res & 0x0f) <= 0x09) cpus.F &= ~fHC; res += 0x06; }
      else if((res&0x0f)>0x09) { res += 0x06; cpus.F|=fHC; }
  } else {
    if(cpus.F & fC) res -= 0x60;
      else if(res>0x99) { res -= 0x60; cpus.F|=fC; }
      
    if(cpus.F & fHC) { if ((res & 0x0f) >= 0x06) cpus.F &= ~fHC; res -= 0x06; }
      else if((res&0x0f)>0x09) { res -= 0x06; }
  }
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   -1,
	   oddp8(res&0xff),
	   -1,
	   -1);
  setundocflags8(res);
  
  cpus.r[rA] = res & 0xff;
  
  z80_clock_inc(4);
}

/************************************************************************/

static void ei_dec_A(void) {
  uint8_t res;

  res=_dec8(cpus.r[rA]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_dec_B(void) {
  uint8_t res;

  res=_dec8(cpus.r[rB]);
  cpus.r[rB]=res;

  z80_clock_inc(4);
}

static void ei_dec_C(void) {
  uint8_t res;

  res=_dec8(cpus.r[rC]);
  cpus.r[rC]=res;

  z80_clock_inc(4);
}

static void ei_dec_D(void) {
  uint8_t res;

  res=_dec8(cpus.r[rD]);
  cpus.r[rD]=res;

  z80_clock_inc(4);
}

static void ei_dec_E(void) {
  uint8_t res;

  res=_dec8(cpus.r[rE]);
  cpus.r[rE]=res;

  z80_clock_inc(4);
}

static void ei_dec_H(void) {
  uint8_t res;

  res=_dec8(cpus.r[rH]);
  cpus.r[rH]=res;

  z80_clock_inc(4);
}

static void ei_dec_L(void) {
  uint8_t res;

  res=_dec8(cpus.r[rL]);
  cpus.r[rL]=res;

  z80_clock_inc(4);
}

static void ei_dec_iHL(void) {
  uint8_t res;

  res=_dec8(_iHL8());
  s_iHL8(res);

  z80_clock_inc(11);
}

static void ei_dec_iIXN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_dec8(_iIXN8(op));
  s_iIXN8(op,res);

  z80_clock_inc(19);
}

static void ei_dec_iIYN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_dec8(_iIYN8(op));
  s_iIYN8(op,res);

  z80_clock_inc(19);
}

static void ei_dec_BC(void) {

  setBC(getBC()-1);
  z80_clock_inc(6);
}

static void ei_dec_DE(void) {

  setDE(getDE()-1);
  z80_clock_inc(6);
}

static void ei_dec_HL(void) {

  setHL(getHL()-1);
  z80_clock_inc(6);
}

static void ei_dec_SP(void) {

  cpus.SP--;
  z80_clock_inc(6);
}

static void ei_dec_IX(void) {

  cpus.IX--;
  z80_clock_inc(6);
}

static void ei_dec_IY(void) {

  cpus.IY--;
  z80_clock_inc(6);
}

/************************************************************************/

static void ei_di(void) {
  cpus.IFF1=cpus.IFF2=0;
  cpus.int_lock=1;
  z80_clock_inc(4);
}

/************************************************************************/

static void ei_djnz(void) {
  uint8_t ofs;
  
  ofs=z80_iget8();
  cpus.r[rB]--;
  if(cpus.r[rB]!=0) {
    _jr8(ofs);
    z80_clock_inc(13);
  } else z80_clock_inc(8);
}

/************************************************************************/

static void ei_ei(void) {
  cpus.IFF1=cpus.IFF2=1;
  cpus.int_lock=1;
  z80_clock_inc(4);
}

/************************************************************************/

static void ei_ex_iSP_HL(void) {
  uint16_t tmp;

  tmp=_iSP16();
  s_iSP16(getHL());
  setHL(tmp);

  z80_clock_inc(19);
}

static void ei_ex_iSP_IX(void) {
  uint16_t tmp;

  tmp=_iSP16();
  s_iSP16(cpus.IX);
  cpus.IX=tmp;

  z80_clock_inc(19);
}

static void ei_ex_iSP_IY(void) {
  uint16_t tmp;

  tmp=_iSP16();
  s_iSP16(cpus.IY);
  cpus.IY=tmp;

  z80_clock_inc(19);
}

static void ei_ex_AF_xAF(void) {
  uint8_t tmp;

  tmp=cpus.r[rA]; cpus.r[rA]=cpus.r_[rA]; cpus.r_[rA]=tmp;
  tmp=cpus.F; cpus.F=cpus.F_; cpus.F_=tmp;

  z80_clock_inc(4);
}

static void ei_ex_DE_HL(void) {
  uint16_t tmp;

  tmp=getDE(); setDE(getHL()); setHL(tmp);
  z80_clock_inc(4);
}

static void ei_exx(void) {
  uint8_t tmp;

  tmp=cpus.r[rB]; cpus.r[rB]=cpus.r_[rB]; cpus.r_[rB]=tmp;
  tmp=cpus.r[rC]; cpus.r[rC]=cpus.r_[rC]; cpus.r_[rC]=tmp;
  tmp=cpus.r[rD]; cpus.r[rD]=cpus.r_[rD]; cpus.r_[rD]=tmp;
  tmp=cpus.r[rE]; cpus.r[rE]=cpus.r_[rE]; cpus.r_[rE]=tmp;
  tmp=cpus.r[rH]; cpus.r[rH]=cpus.r_[rH]; cpus.r_[rH]=tmp;
  tmp=cpus.r[rL]; cpus.r[rL]=cpus.r_[rL]; cpus.r_[rL]=tmp;

  z80_clock_inc(4);
}


/************************************************************************/


static void ei_halt(void) {
  cpus.halted=1;

  z80_clock_inc(4);
}


/************************************************************************/

static void ei_im_0(void) {
  cpus.int_mode=0;
  z80_clock_inc(8);
}

static void ei_im_1(void) {
  cpus.int_mode=1;
  z80_clock_inc(8);
}

static void ei_im_2(void) {
  cpus.int_mode=2;
  z80_clock_inc(8);
}

/************************************************************************/

static void ei_in_A_iN(void) {
  uint8_t res;
  uint16_t op;

  op=z80_iget8();

  res=_in8pf(((uint16_t)cpus.r[rA]<<8)|(uint16_t)op);
  cpus.r[rA]=res;

  z80_clock_inc(11);
}

static void Ui_in_iC(void) {

//  printf("ei_in_iC (unsupported)\n");
  _in8(getBC());

  uoc++;
  z80_clock_inc(12);
}

static void ei_in_A_iC(void) {
  uint8_t res;

  res=_in8(getBC());
  cpus.r[rA]=res;

  z80_clock_inc(12);
}

static void ei_in_B_iC(void) {
  uint8_t res;

  res=_in8(getBC());
  cpus.r[rB]=res;

  z80_clock_inc(12);
}

static void ei_in_C_iC(void) {
  uint8_t res;

  res=_in8(getBC());
  cpus.r[rC]=res;

  z80_clock_inc(12);
}

static void ei_in_D_iC(void) {
  uint8_t res;

  res=_in8(getBC());
  cpus.r[rD]=res;

  z80_clock_inc(12);
}

static void ei_in_E_iC(void) {
  uint8_t res;

  res=_in8(getBC());
  cpus.r[rE]=res;

  z80_clock_inc(12);
}

static void ei_in_H_iC(void) {
  uint8_t res;

  res=_in8(getBC());
  cpus.r[rH]=res;

  z80_clock_inc(12);
}

static void ei_in_L_iC(void) {
  uint8_t res;

  res=_in8(getBC());
  cpus.r[rL]=res;

  z80_clock_inc(12);
}

/************************************************************************/

static void ei_inc_A(void) {
  uint8_t res;

  res=_inc8(cpus.r[rA]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_inc_B(void) {
  uint8_t res;

  res=_inc8(cpus.r[rB]);
  cpus.r[rB]=res;

  z80_clock_inc(4);
}

static void ei_inc_C(void) {
  uint8_t res;

  res=_inc8(cpus.r[rC]);
  cpus.r[rC]=res;

  z80_clock_inc(4);
}

static void ei_inc_D(void) {
  uint8_t res;

  res=_inc8(cpus.r[rD]);
  cpus.r[rD]=res;

  z80_clock_inc(4);
}

static void ei_inc_E(void) {
  uint8_t res;

  res=_inc8(cpus.r[rE]);
  cpus.r[rE]=res;

  z80_clock_inc(4);
}

static void ei_inc_H(void) {
  uint8_t res;

  res=_inc8(cpus.r[rH]);
  cpus.r[rH]=res;

  z80_clock_inc(4);
}

static void ei_inc_L(void) {
  uint8_t res;

  res=_inc8(cpus.r[rL]);
  cpus.r[rL]=res;

  z80_clock_inc(4);
}

static void ei_inc_iHL(void) {
  uint8_t res;

  res=_inc8(_iHL8());
  s_iHL8(res);

  z80_clock_inc(11);
}

static void ei_inc_iIXN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_inc8(_iIXN8(op));
  s_iIXN8(op,res);

  z80_clock_inc(19);
}

static void ei_inc_iIYN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_inc8(_iIYN8(op));
  s_iIYN8(op,res);

  z80_clock_inc(19);
}

static void ei_inc_BC(void) {

  setBC(getBC()+1);
  z80_clock_inc(6);
}

static void ei_inc_DE(void) {

  setDE(getDE()+1);
  z80_clock_inc(6);
}

static void ei_inc_HL(void) {

  setHL(getHL()+1);
  z80_clock_inc(6);
}

static void ei_inc_SP(void) {

  cpus.SP++;
  z80_clock_inc(6);
}

static void ei_inc_IX(void) {

  cpus.IX++;
  z80_clock_inc(6);
}

static void ei_inc_IY(void) {

  cpus.IY++;
  z80_clock_inc(6);
}


/************************************************************************/

static void ei_ind(void) {
  uint8_t res,val;

  val=_in8pf(getBC());
  s_iHL8(val);
  setHL(getHL()-1);
  res=(cpus.r[rB]-1)&0xff;
  
  cpus.F = res & (fU1 | fU2);
  setflags(res>>7,
           res==0,
	   ((uint16_t)val+(uint8_t)(cpus.r[rC]-1))>0xff,
	   oddp8(((val+(uint8_t)(cpus.r[rC]-1))&7)^res),
	   val>>7,
	   ((uint16_t)val+(uint8_t)(cpus.r[rC]-1))>0xff);
  
  cpus.r[rB]=res;
  z80_clock_inc(16);
}

static void ei_indr(void) {
  uint8_t res,val;

  val=_in8pf(getBC());
  s_iHL8(val);
  setHL(getHL()-1);
  res=(cpus.r[rB]-1)&0xff;
  
  cpus.F = res & (fU1 | fU2);
  setflags(res>>7,
           res==0,
	   ((uint16_t)val+(uint8_t)(cpus.r[rC]-1))>0xff,
	   oddp8(((val+(uint8_t)(cpus.r[rC]-1))&7)^res),
	   val>>7,
	   ((uint16_t)val+(uint8_t)(cpus.r[rC]-1))>0xff);
  
  cpus.r[rB]=res;
  
  if(res==0) {
    z80_clock_inc(16);
  } else {
    z80_clock_inc(21);
    cpus.PC-=2;
  }
}

static void ei_ini(void) {
  uint8_t res,val;

  val=_in8pf(getBC());
  s_iHL8(val);
  setHL(getHL()+1);
  res=(cpus.r[rB]-1)&0xff;
  
  cpus.F = res & (fU1 | fU2);
  setflags(res>>7,
           res==0,
	   ((uint16_t)val+(uint8_t)(cpus.r[rC]+1))>0xff,
	   oddp8(((val+(uint8_t)(cpus.r[rC]+1))&7)^res),
	   val>>7,
	   ((uint16_t)val+(uint8_t)(cpus.r[rC]+1))>0xff);
  
  cpus.r[rB]=res;
  z80_clock_inc(16);
}

static void ei_inir(void) {
  uint8_t res,val;

  val=_in8pf(getBC());
  s_iHL8(val);
  setHL(getHL()+1);
  res=(cpus.r[rB]-1)&0xff;
  
  cpus.F = res & (fU1 | fU2);
  setflags(res>>7,
           res==0,
	   ((uint16_t)val+(uint8_t)(cpus.r[rC]+1))>0xff,
	   oddp8(((val+(uint8_t)(cpus.r[rC]+1))&7)^res),
	   val>>7,
	   ((uint16_t)val+(uint8_t)(cpus.r[rC]+1))>0xff);
  
  cpus.r[rB]=res;
  
  if(res==0) {
    z80_clock_inc(16);
  } else {
    z80_clock_inc(21);
    cpus.PC-=2;
  }
}

/************************************************************************/

static void ei_jp_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
 // printf("jp 0x%04x\n",addr);
  _jp16(addr);
  z80_clock_inc(10);
}

static void ei_jp_HL(void) {
  uint16_t addr;

  addr=getHL();
//  printf("%04x:jp HL [0x%04x]\n",cpus.PC,addr);
  _jp16(addr);
  z80_clock_inc(4);
}

static void ei_jp_IX(void) {
  uint16_t addr;

  addr=cpus.IX;
  _jp16(addr);
  z80_clock_inc(4);
}

static void ei_jp_IY(void) {
  uint16_t addr;

  addr=cpus.IY;
  _jp16(addr);
  z80_clock_inc(4);
}

static void ei_jp_C_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(cpus.F & fC) {
    _jp16(addr);
  }
  z80_clock_inc(10);
}

static void ei_jp_NC_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(!(cpus.F & fC)) {
    _jp16(addr);
  }
  z80_clock_inc(10);
}

static void ei_jp_M_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(cpus.F & fS) {
    _jp16(addr);
  }
  z80_clock_inc(10);
}

static void ei_jp_P_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(!(cpus.F & fS)) {
    _jp16(addr);
  }
  z80_clock_inc(10);
}

static void ei_jp_Z_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(cpus.F & fZ) {
    _jp16(addr);
  }
  z80_clock_inc(10);
}

static void ei_jp_NZ_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(!(cpus.F & fZ)) {
    _jp16(addr);
  }
  z80_clock_inc(10);
}

static void ei_jp_PE_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(cpus.F & fPV) {
    _jp16(addr);
  }
  z80_clock_inc(10);
}

static void ei_jp_PO_NN(void) {
  uint16_t addr;

  addr=z80_iget16();
  if(!(cpus.F & fPV)) {
    _jp16(addr);
  }
  z80_clock_inc(10);
}

/************************************************************************/

static void ei_jr_N(void) {
  uint8_t ofs;

  ofs=z80_iget8();
  _jr8(ofs);
  z80_clock_inc(12);
}

static void ei_jr_C_N(void) {
  uint8_t ofs;

  ofs=z80_iget8();
  if(cpus.F & fC) {
    _jr8(ofs);
    z80_clock_inc(12);
  } else z80_clock_inc(7);
}

static void ei_jr_NC_N(void) {
  uint8_t ofs;

  ofs=z80_iget8();
  if(!(cpus.F & fC)) {
    _jr8(ofs);
    z80_clock_inc(12);
  } else z80_clock_inc(7);
}

static void ei_jr_Z_N(void) {
  uint8_t ofs;

  ofs=z80_iget8();
  if(cpus.F & fZ) {
    _jr8(ofs);
    z80_clock_inc(12);
  } else z80_clock_inc(7);
}

static void ei_jr_NZ_N(void) {
  uint8_t ofs;

  ofs=z80_iget8();
  if(!(cpus.F & fZ)) {
    _jr8(ofs);
    z80_clock_inc(12);
  } else z80_clock_inc(7);
}

/************************************************************************/

static void ei_ld_I_A(void) {

  cpus.I=cpus.r[rA];
  z80_clock_inc(9);
}

static void ei_ld_R_A(void) {

  cpus.R=cpus.r[rA];
  z80_clock_inc(9);
}

static void ei_ld_A_I(void) {

  cpus.r[rA]=cpus.I;
  setflags(cpus.r[rA]>>7,
	   cpus.r[rA]==0,
	   0,
	   cpus.IFF2,
	   0,
	   -1);
  setundocflags8(cpus.r[rA]);
  z80_clock_inc(9);
}

static void ei_ld_A_R(void) {

  cpus.r[rA]=cpus.R;
  setflags(cpus.r[rA]>>7,
	   cpus.r[rA]==0,
	   0,
	   cpus.IFF2,
	   0,
	   -1);
  setundocflags8(cpus.r[rA]);
  z80_clock_inc(9);
}

static void ei_ld_A_r(void) {

  cpus.r[rA]=cpus.r[opcode & 0x07];
  z80_clock_inc(4);
}

static void ei_ld_A_N(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rA]=op;
  z80_clock_inc(7);
}

static void ei_ld_A_iBC(void) {

  cpus.r[rA]=_iBC8();
  z80_clock_inc(7);
}

static void ei_ld_A_iDE(void) {

  cpus.r[rA]=_iDE8();
  z80_clock_inc(7);
}

static void ei_ld_A_iHL(void) {

  cpus.r[rA]=_iHL8();
  z80_clock_inc(7);
}

static void ei_ld_A_iIXN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rA]=_iIXN8(op);
  z80_clock_inc(15);
}

static void ei_ld_A_iIYN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rA]=_iIYN8(op);
  z80_clock_inc(15);
}

static void ei_ld_A_iNN(void) {
  uint16_t addr;

  addr=z80_iget16();
  cpus.r[rA]=z80_memget8(addr);
  z80_clock_inc(13);
}

static void ei_ld_B_r(void) {

  cpus.r[rB]=cpus.r[opcode & 0x07];
  z80_clock_inc(4);
}

static void ei_ld_B_N(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rB]=op;
  z80_clock_inc(7);
}

static void ei_ld_B_iHL(void) {

  cpus.r[rB]=_iHL8();
  z80_clock_inc(7);
}

static void ei_ld_B_iIXN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rB]=_iIXN8(op);
  z80_clock_inc(15);
}

static void ei_ld_B_iIYN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rB]=_iIYN8(op);
  z80_clock_inc(15);
}

static void ei_ld_C_r(void) {

  cpus.r[rC]=cpus.r[opcode & 0x07];
  z80_clock_inc(4);
}

static void ei_ld_C_N(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rC]=op;
  z80_clock_inc(7);
}

static void ei_ld_C_iHL(void) {

  cpus.r[rC]=_iHL8();
  z80_clock_inc(7);
}

static void ei_ld_C_iIXN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rC]=_iIXN8(op);
  z80_clock_inc(15);
}

static void ei_ld_C_iIYN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rC]=_iIYN8(op);
  z80_clock_inc(15);
}

static void ei_ld_D_r(void) {

  cpus.r[rD]=cpus.r[opcode & 0x07];
  z80_clock_inc(4);
}

static void ei_ld_D_N(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rD]=op;
  z80_clock_inc(7);
}

static void ei_ld_D_iHL(void) {

  cpus.r[rD]=_iHL8();
  z80_clock_inc(7);
}

static void ei_ld_D_iIXN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rD]=_iIXN8(op);
  z80_clock_inc(15);
}

static void ei_ld_D_iIYN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rD]=_iIYN8(op);
  z80_clock_inc(15);
}

static void ei_ld_E_r(void) {

  cpus.r[rE]=cpus.r[opcode & 0x07];
  z80_clock_inc(4);
}

static void ei_ld_E_N(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rE]=op;
  z80_clock_inc(7);
}

static void ei_ld_E_iHL(void) {

  cpus.r[rE]=_iHL8();
  z80_clock_inc(7);
}

static void ei_ld_E_iIXN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rE]=_iIXN8(op);
  z80_clock_inc(15);
}

static void ei_ld_E_iIYN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rE]=_iIYN8(op);
  z80_clock_inc(15);
}

static void ei_ld_H_r(void) {

  cpus.r[rH]=cpus.r[opcode & 0x07];
  z80_clock_inc(4);
}

static void ei_ld_H_N(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rH]=op;
  z80_clock_inc(7);
}

static void ei_ld_H_iHL(void) {

  cpus.r[rH]=_iHL8();
  z80_clock_inc(7);
}

static void ei_ld_H_iIXN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rH]=_iIXN8(op);
  z80_clock_inc(15);
}

static void ei_ld_H_iIYN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rH]=_iIYN8(op);
  z80_clock_inc(15);
}

static void ei_ld_L_r(void) {

  cpus.r[rL]=cpus.r[opcode & 0x07];
  z80_clock_inc(4);
}

static void ei_ld_L_N(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rL]=op;
  z80_clock_inc(7);
}

static void ei_ld_L_iHL(void) {

  cpus.r[rL]=_iHL8();
  z80_clock_inc(7);
}

static void ei_ld_L_iIXN(void) {
  uint8_t op;

  op=z80_iget8();
  cpus.r[rL]=_iIXN8(op);
  z80_clock_inc(15);
}

static void ei_ld_L_iIYN(void) {
  uint8_t op;

  op=z80_iget8();

  cpus.r[rL]=_iIYN8(op);

  z80_clock_inc(15);
}

static void ei_ld_BC_iNN(void) {
  uint16_t addr;

  addr=z80_iget16();
  setBC(z80_memget16(addr));
  z80_clock_inc(20);
}

static void ei_ld_BC_NN(void) {
  uint16_t data;

  data=z80_iget16();
  setBC(data);
  z80_clock_inc(10);
}

static void ei_ld_DE_iNN(void) {
  uint16_t addr;

  addr=z80_iget16();
  setDE(z80_memget16(addr));
  z80_clock_inc(20);
}

static void ei_ld_DE_NN(void) {
  uint16_t data;

  data=z80_iget16();
  setDE(data);
  z80_clock_inc(10);
}

static void ei_ld_HL_iNN(void) {
  uint16_t addr;

  addr=z80_iget16();
  setHL(z80_memget16(addr));
  z80_clock_inc(16);
}

/* ED prefixed variant, takes 4 more T states */
static void ei_ld_HL_iNN_x(void) {
  uint16_t addr;

  addr=z80_iget16();
  setHL(z80_memget16(addr));
  z80_clock_inc(20);
}

static void ei_ld_HL_NN(void) {
  uint16_t data;

  data=z80_iget16();
  setHL(data);
  z80_clock_inc(10);
}

static void ei_ld_SP_iNN(void) {
  uint16_t addr;

  addr=z80_iget16();
  cpus.SP=z80_memget16(addr);
  z80_clock_inc(20);
}

static void ei_ld_SP_NN(void) {
  uint16_t data;

  data=z80_iget16();
  cpus.SP=data;
  z80_clock_inc(10);
}

static void ei_ld_SP_HL(void) {

  cpus.SP=getHL();
  z80_clock_inc(6);
}

static void ei_ld_SP_IX(void) {

  cpus.SP=cpus.IX;
  z80_clock_inc(6);
}

static void ei_ld_SP_IY(void) {

  cpus.SP=cpus.IY;
  z80_clock_inc(6);
}

static void ei_ld_IX_iNN(void) {
  uint16_t addr;

  addr=z80_iget16();
  cpus.IX=z80_memget16(addr);
  z80_clock_inc(16);
}

static void ei_ld_IX_NN(void) {
  uint16_t data;

  data=z80_iget16();
  cpus.IX=data;
  z80_clock_inc(10);
}

static void ei_ld_IY_iNN(void) {
  uint16_t addr;

  addr=z80_iget16();
  cpus.IY=z80_memget16(addr);
  z80_clock_inc(16);
}

static void ei_ld_IY_NN(void) {
  uint16_t data;

  data=z80_iget16();
  cpus.IY=data;
  z80_clock_inc(10);
}

static void ei_ld_iHL_r(void) {

  s_iHL8(cpus.r[opcode & 0x07]);
  z80_clock_inc(7);
}

static void ei_ld_iHL_N(void) {
  uint8_t op;

  op=z80_iget8();
  s_iHL8(op);
  z80_clock_inc(10);
}

static void ei_ld_iBC_A(void) {

  s_iBC8(cpus.r[rA]);
  z80_clock_inc(7);
}

static void ei_ld_iDE_A(void) {

  s_iDE8(cpus.r[rA]);
  z80_clock_inc(7);
}

static void ei_ld_iNN_A(void) {
  uint16_t addr;

  addr=z80_iget16();
  z80_memset8(addr,cpus.r[rA]);
  z80_clock_inc(13);
}

static void ei_ld_iNN_BC(void) {
  uint16_t addr;

  addr=z80_iget16();
  z80_memset16(addr,getBC());
  z80_clock_inc(20);
}

static void ei_ld_iNN_DE(void) {
  uint16_t addr;

  addr=z80_iget16();
  z80_memset16(addr,getDE());
  z80_clock_inc(20);
}

static void ei_ld_iNN_HL(void) {
  uint16_t addr;

  addr=z80_iget16();
  z80_memset16(addr,getHL());
  z80_clock_inc(16);
}

static void ei_ld_iNN_SP(void) {
  uint16_t addr;

  addr=z80_iget16();
  z80_memset16(addr,cpus.SP);
  z80_clock_inc(20);
}

static void ei_ld_iNN_IX(void) {
  uint16_t addr;

  addr=z80_iget16();
  z80_memset16(addr,cpus.IX);
  z80_clock_inc(16);
}

static void ei_ld_iNN_IY(void) {
  uint16_t addr;

  addr=z80_iget16();
  z80_memset16(addr,cpus.IY);
  z80_clock_inc(16);
}

static void ei_ld_iIXN_r(void) {
  uint8_t op;

  op=z80_iget8();
  s_iIXN8(op,cpus.r[opcode & 0x07]);
  z80_clock_inc(15);
}

static void ei_ld_iIXN_N(void) {
  uint8_t op,data;

  op=z80_iget8();
  data=z80_iget8();
  s_iIXN8(op,data);
  z80_clock_inc(15);
}

static void ei_ld_iIYN_r(void) {
  uint8_t op;

  op=z80_iget8();
  s_iIYN8(op,cpus.r[opcode & 0x07]);
  z80_clock_inc(15);
}

static void ei_ld_iIYN_N(void) {
  uint8_t op,data;

  op=z80_iget8();
  data=z80_iget8();

  s_iIYN8(op,data);

  z80_clock_inc(15);
}


/************************************************************************/

static void ei_ldd(void) {
  uint8_t res,ufr;
  uint16_t newBC;

  res=_iHL8();
  s_iDE8(res);
  setHL(getHL()-1);
  newBC=getBC()-1;  setBC(newBC);
  setDE(getDE()-1);
  
  setflags(-1,
	   -1,
	   0,
	   newBC!=0,
	   0,
	   -1);
  ufr=res+cpus.r[rA];
  setundocflags8(((ufr&0x02)<<4) | (ufr&0x08));

  z80_clock_inc(16);
}

static void ei_lddr(void) {
  uint8_t res,ufr;
  uint16_t newBC;

  res=_iHL8();
  s_iDE8(res);
  setHL(getHL()-1);
  newBC=getBC()-1;  setBC(newBC);
  setDE(getDE()-1);
  
  setflags(-1,
	   -1,
	   0,
	   newBC!=0,
	   0,
	   -1);
  ufr=res+cpus.r[rA];
  setundocflags8(((ufr&0x02)<<4) | (ufr&0x08));

  if(newBC==0) {
    z80_clock_inc(16);
  } else {
    z80_clock_inc(21);
    cpus.PC-=2;
  }
}


static void ei_ldi(void) {
  uint8_t res,ufr;
  uint16_t newBC;

  res=_iHL8();
  s_iDE8(res);
  setHL(getHL()+1);
  newBC=getBC()-1; setBC(newBC);
  setDE(getDE()+1);
  
  setflags(-1,
	   -1,
	   0,
	   newBC!=0,
	   0,
	   -1);
  ufr=res+cpus.r[rA];
  setundocflags8(((ufr&0x02)<<4) | (ufr&0x08));

  z80_clock_inc(16);
}

static void ei_ldir(void) {
  uint8_t res,ufr;
  uint16_t newBC;

  res=_iHL8();
  s_iDE8(res);
  setHL(getHL()+1);
  newBC=getBC()-1; setBC(newBC);
  setDE(getDE()+1);
  setflags(-1,
	   -1,
	   0,
	   newBC!=0,
	   0,
	   -1);
  ufr=res+cpus.r[rA];
  setundocflags8(((ufr&0x02)<<4) | (ufr&0x08));

  if(newBC==0) {
    z80_clock_inc(16);
  } else {
    z80_clock_inc(21);
    cpus.PC-=2;
  }
}


/************************************************************************/

static void ei_neg(void) {               /* A <- neg(A) .. two's complement */
  uint8_t oldA;
  uint8_t res;

  oldA = cpus.r[rA];
  res = (oldA ^ 0xff)+1;

  setflags(res>>7,
	   res==0,
	   (oldA&0x0f)!=0,
	   oldA==0x80,     /* 127 -> -128 */
	   1,
	   oldA != 0x00);

  cpus.r[rA] = res;
  setundocflags8(res);
  z80_clock_inc(8);
}

/************************************************************************/

static void ei_nop(void) {
  z80_clock_inc(4);
}

/************************************************************************/

static void ei_or_r(void) {
  uint8_t res;

  res=_or8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_or_N(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_or8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_or_iHL(void) {
  uint8_t res;

  res=_or8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_or_iIXN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_or8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

static void ei_or_iIYN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_or8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

/************************************************************************/

static void ei_out_iN_A(void) {
  uint16_t op;

  op=z80_iget8();

  _out8(((uint16_t)cpus.r[rA]<<8)|(uint16_t)op,cpus.r[rA]);

  z80_clock_inc(11);
}

static void Ui_out_iC_0(void) {

//  printf("ei_out_iC_0 (unsupported)\n");
  _out8(getBC(),0);

  uoc++;
  z80_clock_inc(12);
}

static void ei_out_iC_A(void) {

  _out8(getBC(),cpus.r[rA]);
  z80_clock_inc(12);
}

static void ei_out_iC_B(void) {

  _out8(getBC(),cpus.r[rB]);
  z80_clock_inc(12);
}

static void ei_out_iC_C(void) {

  _out8(getBC(),cpus.r[rC]);
  z80_clock_inc(12);
}

static void ei_out_iC_D(void) {

  _out8(getBC(),cpus.r[rD]);
  z80_clock_inc(12);
}

static void ei_out_iC_E(void) {

  _out8(getBC(),cpus.r[rE]);
  z80_clock_inc(12);
}

static void ei_out_iC_H(void) {

  _out8(getBC(),cpus.r[rH]);
  z80_clock_inc(12);
}

static void ei_out_iC_L(void) {

  _out8(getBC(),cpus.r[rL]);
  z80_clock_inc(12);
}

/************************************************************************/

static void ei_outd(void) {
  uint8_t res;
  uint8_t val;
  uint16_t bc;
  
  res=cpus.r[rB]-1;
  bc = ((uint16_t)res << 8) | cpus.r[rC];
  val = _iHL8();
  _out8(bc,val);
  setHL(getHL()-1);
  
  cpus.F = res & (fU1 | fU2);
  setflags(res>>7,
           res==0,
	   ((uint16_t)val+cpus.r[rL])>0xff,
	   oddp8(((val+cpus.r[rL])&7)^res),
	   val>>7,
	   ((uint16_t)val+cpus.r[rL])>0xff);
   
  cpus.r[rB]=res;
  z80_clock_inc(16);
}

static void ei_otdr(void) {
  uint8_t res;
  uint8_t val;
  uint16_t bc;
  
  res=cpus.r[rB]-1;
  bc = ((uint16_t)res << 8) | cpus.r[rC];
  val = _iHL8();
  _out8(bc,val);
  setHL(getHL()-1);
  
  cpus.F = res & (fU1 | fU2);
  setflags(res>>7,
           res==0,
	   ((uint16_t)val+cpus.r[rL])>0xff,
	   oddp8(((val+cpus.r[rL])&7)^res),
	   val>>7,
	   ((uint16_t)val+cpus.r[rL])>0xff);
  
  cpus.r[rB]=res;
  if(res==0) {
    z80_clock_inc(16);
  } else {
    z80_clock_inc(21);
    cpus.PC-=2;
  }
}

static void ei_outi(void) {
  uint8_t res;
  uint8_t val;
  uint16_t bc;
  
  res=cpus.r[rB]-1;
  bc = ((uint16_t)res << 8) | cpus.r[rC];
  val = _iHL8();
  _out8(bc,val);
  setHL(getHL()+1);
  
  cpus.F = res & (fU1 | fU2);
  setflags(res>>7,
           res==0,
	   ((uint16_t)val+cpus.r[rL])>0xff,
	   oddp8(((val+cpus.r[rL])&7)^res),
	   val>>7,
	   ((uint16_t)val+cpus.r[rL])>0xff);
   
  cpus.r[rB]=res;
  z80_clock_inc(16);
}

static void ei_otir(void) {
  uint8_t res;
  uint8_t val;
  uint16_t bc;
  
  res=cpus.r[rB]-1;
  bc = ((uint16_t)res << 8) | cpus.r[rC];
  val = _iHL8();
  _out8(bc,val);
  setHL(getHL()+1);
  
  cpus.F = res & (fU1 | fU2);
  setflags(res>>7,
           res==0,
	   ((uint16_t)val+cpus.r[rL])>0xff,
	   oddp8(((val+cpus.r[rL])&7)^res),
	   val>>7,
	   ((uint16_t)val+cpus.r[rL])>0xff);
  
  cpus.r[rB]=res;
  if(res==0) {
    z80_clock_inc(16);
  } else {
    z80_clock_inc(21);
    cpus.PC-=2;
  }
}


/************************************************************************/

static void ei_pop_AF(void) {
  setAF(_pop16());
  z80_clock_inc(10);
}

static void ei_pop_BC(void) {
  setBC(_pop16());
  z80_clock_inc(10);
}

static void ei_pop_DE(void) {
  setDE(_pop16());
  z80_clock_inc(10);
}

static void ei_pop_HL(void) {
  setHL(_pop16());
  z80_clock_inc(10);
}

static void ei_pop_IX(void) {
  cpus.IX=_pop16();
  z80_clock_inc(10);
}

static void ei_pop_IY(void) {
  cpus.IY=_pop16();
  z80_clock_inc(10);
}

static void ei_push_AF(void) {
  _push16(getAF());
  z80_clock_inc(11);
}

static void ei_push_BC(void) {
  _push16(getBC());
  z80_clock_inc(11);
}

static void ei_push_DE(void) {
  _push16(getDE());
  z80_clock_inc(11);
}

static void ei_push_HL(void) {
  _push16(getHL());
  z80_clock_inc(11);
}

static void ei_push_IX(void) {
  _push16(cpus.IX);
  z80_clock_inc(11);
}

static void ei_push_IY(void) {
  _push16(cpus.IY);
  z80_clock_inc(11);
}

/************************************************************************/

static void ei_res_b_r(void) {
  uint8_t res;

  res=_res8((opcode>>3)&0x07,cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(8);
}

static void ei_res_b_iHL(void) {
  uint8_t res;

  res=_res8((opcode>>3)&0x07,_iHL8());
  s_iHL8(res);

  z80_clock_inc(15);
}

/* DDCB ! */
static void ei_res_b_iIXN(void) {
  uint8_t res;

  res=_res8((opcode>>3)&0x07,_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock_inc(19);
}

/* FDCB ! */
static void ei_res_b_iIYN(void) {
   uint8_t res;

  res=_res8((opcode>>3)&0x07,_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock_inc(19);
}

/************************************************************************/

static void ei_ret(void) {
  cpus.PC=_pop16();
  z80_clock_inc(10);
}

static void ei_ret_C(void) {
  if(cpus.F & fC) {
    cpus.PC=_pop16();
    z80_clock_inc(11);
  } else z80_clock_inc(5);
}

static void ei_ret_NC(void) {
  if(!(cpus.F & fC)) {
    cpus.PC=_pop16();
    z80_clock_inc(11);
  } else z80_clock_inc(5);
}

static void ei_ret_M(void) {
  if(cpus.F & fS) {
    cpus.PC=_pop16();
    z80_clock_inc(11);
  } else z80_clock_inc(5);
}

static void ei_ret_P(void) {
  if(!(cpus.F & fS)) {
    cpus.PC=_pop16();
    z80_clock_inc(11);
  } else z80_clock_inc(5);
}


static void ei_ret_Z(void) {
  if(cpus.F & fZ) {
    cpus.PC=_pop16();
    z80_clock_inc(11);
  } else z80_clock_inc(5);
}

static void ei_ret_NZ(void) {
  if(!(cpus.F & fZ)) {
    cpus.PC=_pop16();
    z80_clock_inc(11);
  } else z80_clock_inc(5);
}

static void ei_ret_PE(void) {
  if(cpus.F & fPV) {
    cpus.PC=_pop16();
    z80_clock_inc(11);
  } else z80_clock_inc(5);
}

static void ei_ret_PO(void) {
  if(!(cpus.F & fPV)) {
    cpus.PC=_pop16();
    z80_clock_inc(11);
  } else z80_clock_inc(5);
}

/************************************************************************/

static void ei_reti(void) {
  cpus.IFF1=cpus.IFF2;
  cpus.PC=_pop16();
  z80_clock_inc(14);
}


static void ei_retn(void) {
  cpus.IFF1=cpus.IFF2;
  cpus.PC=_pop16();
  z80_clock_inc(14);
}

/************************************************************************/

static void ei_rla(void) {
  uint8_t res;

  res=_rla8(cpus.r[rA]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_rl_r(void) {
  uint8_t res;

  res=_rl8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(8);
}

static void ei_rl_iHL(void) {
  uint8_t res;

  res=_rl8(_iHL8());
  s_iHL8(res);

  z80_clock_inc(15);
}

/* DDCB */
static void ei_rl_iIXN(void) {
  uint8_t res;

  res=_rl8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock_inc(19);
}

/* FDCB */
static void ei_rl_iIYN(void) {
  uint8_t res;

  res=_rl8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock_inc(19);
}

static void ei_rlca(void) {
  uint8_t res;

  res=_rlca8(cpus.r[rA]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_rlc_r(void) {
  uint8_t res;

  res=_rlc8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(8);
}

static void ei_rlc_iHL(void) {
  uint8_t res;

  res=_rlc8(_iHL8());
  s_iHL8(res);

  z80_clock_inc(15);
}

/* DDCB */
static void ei_rlc_iIXN(void) {
  uint8_t res;

  res=_rlc8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock_inc(19);
}

/* FDCB */
static void ei_rlc_iIYN(void) {
  uint8_t res;

  res=_rlc8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock_inc(19);
}

static void ei_rld(void) {
  uint8_t tmp,tmp2,tmp3;

  tmp=cpus.r[rA] & 0x0f;
  tmp2=_iHL8();
  tmp3=tmp2>>4;
  tmp2=(tmp2<<4)|tmp;
  cpus.r[rA]=(cpus.r[rA] & 0xf0)| tmp3;
  s_iHL8(tmp2);
  
  cpus.F=(cpus.F&fC)|ox_tab[cpus.r[rA]];

  z80_clock_inc(18);
}

static void ei_rra(void) {
  uint8_t res;

  res=_rra8(cpus.r[rA]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_rr_r(void) {
  uint8_t res;

  res=_rr8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(8);
}

static void ei_rr_iHL(void) {
  uint8_t res;

  res=_rr8(_iHL8());
  s_iHL8(res);

  z80_clock_inc(15);
}

/* DDCB */
static void ei_rr_iIXN(void) {
  uint8_t res;

  res=_rr8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock_inc(19);
}

/* FDCB */
static void ei_rr_iIYN(void) {
  uint8_t res;

  res=_rr8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock_inc(19);
}

static void ei_rrca(void) {
  uint8_t res;

  res=_rrca8(cpus.r[rA]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_rrc_r(void) {
  uint8_t res;

  res=_rrc8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(8);
}

static void ei_rrc_iHL(void) {
  uint8_t res;

  res=_rrc8(_iHL8());
  s_iHL8(res);

  z80_clock_inc(15);
}

/* DDCB */
static void ei_rrc_iIXN(void) {
  uint8_t res;

  res=_rrc8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock_inc(19);
}

/* FDCB */
static void ei_rrc_iIYN(void) {
  uint8_t res;

  res=_rrc8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock_inc(19);
}

static void ei_rrd(void) {
  uint8_t tmp,tmp2,tmp3;

  tmp=cpus.r[rA] & 0x0f;
  tmp2=_iHL8();
  tmp3=tmp2 & 0x0f;
  tmp2=(tmp2>>4)|(tmp<<4);
  cpus.r[rA]=(cpus.r[rA] & 0xf0)| tmp3;
  s_iHL8(tmp2);
  
  cpus.F=(cpus.F&fC)|ox_tab[cpus.r[rA]];

  z80_clock_inc(18);
}

/************************************************************************/

static void ei_rst_0(void) {
  _call16(0x0000);
  z80_clock_inc(11);
}

static void ei_rst_8(void) {
  _call16(0x0008);
  z80_clock_inc(11);
}

static void ei_rst_10(void) {
  _call16(0x0010);
  z80_clock_inc(11);
}

static void ei_rst_18(void) {
  _call16(0x0018);
  z80_clock_inc(11);
}

static void ei_rst_20(void) {
  _call16(0x0020);
  z80_clock_inc(11);
}

static void ei_rst_28(void) {
  _call16(0x0028);
  z80_clock_inc(11);
}

static void ei_rst_30(void) {
  _call16(0x0030);
  z80_clock_inc(11);
}

static void ei_rst_38(void) {
  _call16(0x0038);
  z80_clock_inc(11);
}

/************************************************************************/

static void ei_sbc_A_r(void) {
  uint8_t res;

  res=_sbc8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_sbc_A_N(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_sbc8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_sbc_A_iHL(void) {
  uint8_t res;

  res=_sbc8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_sbc_A_iIXN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_sbc8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

static void ei_sbc_A_iIYN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_sbc8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

static void ei_sbc_HL_BC(void) {
  uint16_t res;

  res=_sbc16(getHL(),getBC());
  setHL(res);

  z80_clock_inc(15);
}

static void ei_sbc_HL_DE(void) {
  uint16_t res;

  res=_sbc16(getHL(),getDE());
  setHL(res);

  z80_clock_inc(15);
}

static void ei_sbc_HL_HL(void) {
  uint16_t res;

  res=_sbc16(getHL(),getHL());
  setHL(res);

  z80_clock_inc(15);
}

static void ei_sbc_HL_SP(void) {
  uint16_t res;

  res=_sbc16(getHL(),cpus.SP);
  setHL(res);

  z80_clock_inc(15);
}


/************************************************************************/

static void ei_scf(void) {
  setflags(-1,-1,0,-1,0,1);
  setundocflags8(cpus.r[rA]);
  z80_clock_inc(4);
}

/************************************************************************/

static void ei_set_b_r(void) {
  uint8_t res;

  res=_set8((opcode>>3)&0x07,cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(8);
}

static void ei_set_b_iHL(void) {
  uint8_t res;

  res=_set8((opcode>>3)&0x07,_iHL8());
  s_iHL8(res);

  z80_clock_inc(15);
}

/* DDCB ! */
static void ei_set_b_iIXN(void) {
  uint8_t res;

  res=_set8((opcode>>3)&0x07,_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock_inc(19);
}

/* FDCB ! */
static void ei_set_b_iIYN(void) {
  uint8_t res;

  res=_set8((opcode>>3)&0x07,_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock_inc(19);
}

/************************************************************************/

static void ei_sla_r(void) {
  uint8_t res;

  res=_sla8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(8);
}

static void ei_sla_iHL(void) {
  uint8_t res;

  res=_sla8(_iHL8());
  s_iHL8(res);

  z80_clock_inc(15);
}

/* DDCB */
static void ei_sla_iIXN(void) {
  uint8_t res;

  res=_sla8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock_inc(19);
}

/* FDCB */
static void ei_sla_iIYN(void) {
  uint8_t res;

  res=_sla8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock_inc(19);
}

static void ei_sra_r(void) {
  uint8_t res;

  res=_sra8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(8);
}

static void ei_sra_iHL(void) {
  uint8_t res;

  res=_sra8(_iHL8());
  s_iHL8(res);

  z80_clock_inc(15);
}

/* DDCB */
static void ei_sra_iIXN(void) {
  uint8_t res;

  res=_sra8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock_inc(19);
}

/* FDCB */
static void ei_sra_iIYN(void) {
  uint8_t res;

  res=_sra8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock_inc(19);
}

/************************************************************************/

#ifndef NO_Z80UNDOC

static void Ui_sll_r(void) {
  uint8_t res;

  res=_sll8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  uoc++;
  z80_clock_inc(8);
}

static void Ui_sll_iHL(void) {
  uint8_t res;

  res=_sll8(_iHL8());
  s_iHL8(res);

  uoc++;
  z80_clock_inc(15);
}

/* DDCB */
static void Ui_sll_iIXN(void) {
  uint8_t res;

  res=_sll8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  uoc++;
  z80_clock_inc(19);
}

/* FDCB */
static void Ui_sll_iIYN(void) {
  uint8_t res;

  res=_sll8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  uoc++;
  z80_clock_inc(19);
}

#else

#define Ui_sll_r ei_undoc
#define Ui_sll_iHL ei_undoc
#define Ui_sll_iIXN ei_undoc
#define Ui_sll_iIYN ei_undoc

#endif

static void ei_srl_r(void) {
  uint8_t res;

  res=_srl8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(8);
}

static void ei_srl_iHL(void) {
  uint8_t res;

  res=_srl8(_iHL8());
  s_iHL8(res);

  z80_clock_inc(15);
}

/* DDCB */
static void ei_srl_iIXN(void) {
  uint8_t res;

  res=_srl8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock_inc(19);
}

/* FDCB */
static void ei_srl_iIYN(void) {
  uint8_t res;

  res=_srl8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock_inc(19);
}

/************************************************************************/

static void ei_sub_r(void) {
  uint8_t res;

  res=_sub8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_sub_N(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_sub8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_sub_iHL(void) {
  uint8_t res;

  res=_sub8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_sub_iIXN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_sub8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

static void ei_sub_iIYN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_sub8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}


/************************************************************************/

static void ei_xor_r(void) {
  uint8_t res;

  res=_xor8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock_inc(4);
}

static void ei_xor_N(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_xor8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_xor_iHL(void) {
  uint8_t res;

  res=_xor8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock_inc(7);
}

static void ei_xor_iIXN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_xor8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

static void ei_xor_iIYN(void) {
  uint8_t res,op;

  op=z80_iget8();

  res=_xor8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock_inc(15);
}

/************************ undocumented opcodes ****************************/
/**** DD .. ***************************************************************/

#ifndef NO_Z80UNDOC

static void Ui_inc_IXh(void) {
  uint8_t res;

  res=_inc8(getIXh());
  setIXh(res);

  z80_clock_inc(4); /* timing&flags taken from inc_A */
  uoc++;
}

static void Ui_dec_IXh(void) {
  uint8_t res;

  res=_dec8(getIXh());
  setIXh(res);

  z80_clock_inc(4); /* timing&flags taken from dec_A */
  uoc++;
}

static void Ui_ld_IXh_N(void) {
  uint8_t res;

  res=z80_iget8();
  setIXh(res);

  z80_clock_inc(7); /* timing&flags taken from ld_A_N */
  uoc++;
}

static void Ui_inc_IXl(void) {
  uint8_t res;

  res=_inc8(getIXl());
  setIXl(res);

  z80_clock_inc(4); /* timing&flags taken from inc_A */
  uoc++;
}

static void Ui_dec_IXl(void) {
  uint8_t res;

  res=_dec8(getIXl());
  setIXl(res);

  z80_clock_inc(4); /* timing&flags taken from dec_A */
  uoc++;
}

static void Ui_ld_IXl_N(void) {
  uint8_t res;

  res=z80_iget8();
  setIXl(res);

  z80_clock_inc(7); /* timing&flags taken from ld_A_N */
  uoc++;
}

static void Ui_ld_B_IXh(void) {

  cpus.r[rB]=getIXh();

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_B_IXl(void) {

  cpus.r[rB]=getIXl();

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_C_IXh(void) {

  cpus.r[rC]=getIXh();

  z80_clock_inc(4); /* timing&flags taken from ld_C_r */
  uoc++;
}

static void Ui_ld_C_IXl(void) {

  cpus.r[rC]=getIXl();

  z80_clock_inc(4); /* timing&flags taken from ld_C_r */
  uoc++;
}

static void Ui_ld_D_IXh(void) {

  cpus.r[rD]=getIXh();

  z80_clock_inc(4); /* timing&flags taken from ld_D_r */
  uoc++;
}

static void Ui_ld_D_IXl(void) {

  cpus.r[rD]=getIXl();

  z80_clock_inc(4); /* timing&flags taken from ld_D_r */
  uoc++;
}

static void Ui_ld_E_IXh(void) {

  cpus.r[rE]=getIXh();

  z80_clock_inc(4); /* timing&flags taken from ld_E_r */
  uoc++;
}

static void Ui_ld_E_IXl(void) {

  cpus.r[rE]=getIXl();

  z80_clock_inc(4); /* timing&flags taken from ld_E_r */
  uoc++;
}

static void Ui_ld_IXh_B(void) {

  setIXh(cpus.r[rB]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXh_C(void) {

  setIXh(cpus.r[rC]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXh_D(void) {

  setIXh(cpus.r[rD]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXh_E(void) {

  setIXh(cpus.r[rE]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXh_IXh(void) {

  /* does nothing */

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXh_IXl(void) {

  setIXh(getIXl());

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXh_A(void) {

  setIXh(cpus.r[rA]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_B(void) {

  setIXl(cpus.r[rB]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_C(void) {

  setIXl(cpus.r[rC]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_D(void) {

  setIXl(cpus.r[rD]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_E(void) {

  setIXl(cpus.r[rE]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_IXh(void) {

  setIXl(getIXh());

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_IXl(void) {

  /* does nothing */

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_A(void) {

  setIXl(cpus.r[rA]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_A_IXh(void) {

  cpus.r[rA]=getIXh();

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_A_IXl(void) {

  cpus.r[rA]=getIXl();

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

/***********************************************************************/

static void Ui_add_A_IXh(void) {
  uint8_t res;

  res=_add8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from add_A_r */
  uoc++;
}

static void Ui_add_A_IXl(void) {
  uint8_t res;

  res=_add8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from add_A_r */
  uoc++;
}

static void Ui_adc_A_IXh(void) {
  uint8_t res;

  res=_adc8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from adc_A_r */
  uoc++;
}

static void Ui_adc_A_IXl(void) {
  uint8_t res;

  res=_adc8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from adc_A_r */
  uoc++;
}

static void Ui_sub_IXh(void) {
  uint8_t res;

  res=_sub8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from sub_r */
  uoc++;
}

static void Ui_sub_IXl(void) {
  uint8_t res;

  res=_sub8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from sub_r */
  uoc++;
}

static void Ui_sbc_IXh(void) {
  uint8_t res;

  res=_sbc8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from sbc_r */
  uoc++;
}

static void Ui_sbc_IXl(void) {
  uint8_t res;

  res=_sbc8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from sbc_r */
  uoc++;
}

static void Ui_and_IXh(void) {
  uint8_t res;

  res=_and8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from and_r */
  uoc++;
}

static void Ui_and_IXl(void) {
  uint8_t res;

  res=_and8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from and_r */
  uoc++;
}

static void Ui_xor_IXh(void) {
  uint8_t res;

  res=_xor8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from xor_r */
  uoc++;
}

static void Ui_xor_IXl(void) {
  uint8_t res;

  res=_xor8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from xor_r */
  uoc++;
}

static void Ui_or_IXh(void) {
  uint8_t res;

  res=_or8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from or_r */
  uoc++;
}

static void Ui_or_IXl(void) {
  uint8_t res;

  res=_or8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from or_r */
  uoc++;
}

static void Ui_cp_IXh(void) {
  uint8_t res;

  res=_cp8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from cp_r */
  uoc++;
}

static void Ui_cp_IXl(void) {
  uint8_t res;

  res=_cp8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from cp_r */
  uoc++;
}

/**** FD .. ***************************************************************/

static void Ui_inc_IYh(void) {
  uint8_t res;

  res=_inc8(getIYh());
  setIYh(res);

  z80_clock_inc(4); /* timing&flags taken from inc_A */
  uoc++;
}

static void Ui_dec_IYh(void) {
  uint8_t res;

  res=_dec8(getIYh());
  setIYh(res);

  z80_clock_inc(4); /* timing&flags taken from dec_A */
  uoc++;
}

static void Ui_ld_IYh_N(void) {
  uint8_t res;

  res=z80_iget8();
  setIYh(res);

  z80_clock_inc(7); /* timing&flags taken from ld_A_N */
  uoc++;
}

static void Ui_inc_IYl(void) {
  uint8_t res;

  res=_inc8(getIYl());
  setIYl(res);

  z80_clock_inc(4); /* timing&flags taken from inc_A */
  uoc++;
}

static void Ui_dec_IYl(void) {
  uint8_t res;

  res=_dec8(getIYl());
  setIYl(res);

  z80_clock_inc(4); /* timing&flags taken from dec_A */
  uoc++;
}

static void Ui_ld_IYl_N(void) {
  uint8_t res;

  res=z80_iget8();
  setIYl(res);

  z80_clock_inc(7); /* timing&flags taken from ld_A_N */
  uoc++;
}

static void Ui_ld_B_IYh(void) {

  cpus.r[rB]=getIYh();

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_B_IYl(void) {

  cpus.r[rB]=getIYl();

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_C_IYh(void) {

  cpus.r[rC]=getIYh();

  z80_clock_inc(4); /* timing&flags taken from ld_C_r */
  uoc++;
}

static void Ui_ld_C_IYl(void) {

  cpus.r[rC]=getIYl();

  z80_clock_inc(4); /* timing&flags taken from ld_C_r */
  uoc++;
}

static void Ui_ld_D_IYh(void) {

  cpus.r[rD]=getIYh();

  z80_clock_inc(4); /* timing&flags taken from ld_D_r */
  uoc++;
}

static void Ui_ld_D_IYl(void) {

  cpus.r[rD]=getIYl();

  z80_clock_inc(4); /* timing&flags taken from ld_D_r */
  uoc++;
}

static void Ui_ld_E_IYh(void) {

  cpus.r[rE]=getIYh();

  z80_clock_inc(4); /* timing&flags taken from ld_E_r */
  uoc++;
}

static void Ui_ld_E_IYl(void) {

  cpus.r[rE]=getIYl();

  z80_clock_inc(4); /* timing&flags taken from ld_E_r */
  uoc++;
}

static void Ui_ld_IYh_B(void) {

  setIYh(cpus.r[rB]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYh_C(void) {

  setIYh(cpus.r[rC]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYh_D(void) {

  setIYh(cpus.r[rD]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYh_E(void) {

  setIYh(cpus.r[rE]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYh_IYh(void) {

  /* does nothing */

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYh_IYl(void) {

  setIYh(getIYl());

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYh_A(void) {

  setIYh(cpus.r[rA]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_B(void) {

  setIYl(cpus.r[rB]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_C(void) {

  setIYl(cpus.r[rC]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_D(void) {

  setIYl(cpus.r[rD]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_E(void) {

  setIYl(cpus.r[rE]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_IYh(void) {

  setIYl(getIYh());

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_IYl(void) {

  /* does nothing */

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_A(void) {

  setIYl(cpus.r[rA]);

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_A_IYh(void) {

  cpus.r[rA]=getIYh();

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_A_IYl(void) {

  cpus.r[rA]=getIYl();

  z80_clock_inc(4); /* timing&flags taken from ld_B_r */
  uoc++;
}

/***********************************************************************/

static void Ui_add_A_IYh(void) {
  uint8_t res;

  res=_add8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from add_A_r */
  uoc++;
}

static void Ui_add_A_IYl(void) {
  uint8_t res;

  res=_add8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from add_A_r */
  uoc++;
}

static void Ui_adc_A_IYh(void) {
  uint8_t res;

  res=_adc8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from adc_A_r */
  uoc++;
}

static void Ui_adc_A_IYl(void) {
  uint8_t res;

  res=_adc8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from adc_A_r */
  uoc++;
}

static void Ui_sub_IYh(void) {
  uint8_t res;

  res=_sub8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from sub_r */
  uoc++;
}

static void Ui_sub_IYl(void) {
  uint8_t res;

  res=_sub8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from sub_r */
  uoc++;
}

static void Ui_sbc_IYh(void) {
  uint8_t res;

  res=_sbc8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from sbc_r */
  uoc++;
}

static void Ui_sbc_IYl(void) {
  uint8_t res;

  res=_sbc8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from sbc_r */
  uoc++;
}

static void Ui_and_IYh(void) {
  uint8_t res;

  res=_and8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from and_r */
  uoc++;
}

static void Ui_and_IYl(void) {
  uint8_t res;

  res=_and8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from and_r */
  uoc++;
}

static void Ui_xor_IYh(void) {
  uint8_t res;

  res=_xor8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from xor_r */
  uoc++;
}

static void Ui_xor_IYl(void) {
  uint8_t res;

  res=_xor8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from xor_r */
  uoc++;
}

static void Ui_or_IYh(void) {
  uint8_t res;

  res=_or8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from or_r */
  uoc++;
}

static void Ui_or_IYl(void) {
  uint8_t res;

  res=_or8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from or_r */
  uoc++;
}

static void Ui_cp_IYh(void) {
  uint8_t res;

  res=_cp8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from cp_r */
  uoc++;
}

static void Ui_cp_IYl(void) {
  uint8_t res;

  res=_cp8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock_inc(4); /* timing&flags taken from cp_r */
  uoc++;
}

/**** ED .. ***************************************************************/

static void Ui_ednop(void) { /* different from ei_nop! */
  z80_clock_inc(8); /* according to Sean it's like 2 NOPs */
  uoc++;
}

static void Ui_neg(void) {               /* A <- neg(A) .. two's complement */
  uint8_t oldA;
  uint8_t res;

  oldA = cpus.r[rA];
  res = (oldA ^ 0xff)+1;

  setflags(res>>7,
	   res==0,
	   (oldA&0x0f)!=0,
	   oldA==0x80,     /* 127 -> -128 */
	   1,
	   oldA != 0x00);

  cpus.r[rA] = res;
  setundocflags8(res);
  z80_clock_inc(8);
  uoc++;
}

static void Ui_im_0(void) { /* probably ..*/
//  printf("IM 0\n");
  cpus.int_mode=0;
  z80_clock_inc(8); /* timing taken from ei_im_0 */
  uoc++;
}

static void Ui_im_1(void) { /* probably ..*/
//  printf("IM 1\n");
  cpus.int_mode=1;
  z80_clock_inc(8); /* timing taken from ei_im_1 */
  uoc++;
}

static void Ui_im_2(void) { /* probably ..*/
//  printf("IM 2\n");
  cpus.int_mode=2;
  z80_clock_inc(8); /* timing taken from ei_im_2 */
  uoc++;
}

static void Ui_reti(void) { /* undoc RETI behaves like RETN actually.. */
  cpus.IFF1=cpus.IFF2;
  cpus.PC=_pop16();
  z80_clock_inc(14); /* timing taken from ei_reti */
  uoc++;
}

static void Ui_retn(void) {
  cpus.IFF1=cpus.IFF2;
  cpus.PC=_pop16();
  z80_clock_inc(14);  /* timing taken from ei_retn */
  uoc++;
}

/**** DD CB dd .. ***************************************************************/

static void Ui_ld_r_rlc_iIXN(void) {
  uint8_t res;

  res=_rlc8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_rlc_iIXN */
  uoc++;
}

static void Ui_ld_r_rrc_iIXN(void) {
  uint8_t res;

  res=_rrc8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_rrc_iIXN */
  uoc++;
}

static void Ui_ld_r_rl_iIXN(void) {
  uint8_t res;

  res=_rl8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_rl_iIXN */
  uoc++;
}

static void Ui_ld_r_rr_iIXN(void) {
  uint8_t res;

  res=_rr8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_rr_iIXN */
  uoc++;
}

static void Ui_ld_r_sla_iIXN(void) {
  uint8_t res;

  res=_sla8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_sla_iIXN */
  uoc++;
}

static void Ui_ld_r_sra_iIXN(void) {
  uint8_t res;

  res=_sra8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_sra_iIXN */
  uoc++;
}

static void Ui_ld_r_sll_iIXN(void) {
  uint8_t res;

  res=_sll8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_sll_iIXN */
  uoc++;
}

static void Ui_ld_r_srl_iIXN(void) {
  uint8_t res;

  res=_srl8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_srl_iIXN */
  uoc++;
}

/************************************************************************/

static void Ui_bit_b_iIXN(void) {

  _bit8((opcode>>3)&0x07,_iIXN8(cbop));
  
  setundocflags8((cpus.IX+u8sval(cbop))>>8); /* weird, huh? */

  z80_clock_inc(16); /* timing&flags taken from ei_bit_b_iIXN */
  uoc++;
}

static void Ui_ld_r_res_b_iIXN(void) {
  uint8_t res;

  res=_res8((opcode>>3)&0x07,_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_res_b_iIXN */
  uoc++;
}

static void Ui_ld_r_set_b_iIXN(void) {
  uint8_t res;

  res=_set8((opcode>>3)&0x07,_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_set_b_iIXN */
  uoc++;
}

/**** FD CB dd .. ***************************************************************/

static void Ui_ld_r_rlc_iIYN(void) {
  uint8_t res;

  res=_rlc8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_rlc_iIYN */
  uoc++;
}

static void Ui_ld_r_rrc_iIYN(void) {
  uint8_t res;

  res=_rrc8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_rrc_iIYN */
  uoc++;
}

static void Ui_ld_r_rl_iIYN(void) {
  uint8_t res;

  res=_rl8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_rl_iIYN */
  uoc++;
}

static void Ui_ld_r_rr_iIYN(void) {
  uint8_t res;

  res=_rr8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_rr_iIYN */
  uoc++;
}

static void Ui_ld_r_sla_iIYN(void) {
  uint8_t res;

  res=_sla8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_sla_iIYN */
  uoc++;
}

static void Ui_ld_r_sra_iIYN(void) {
  uint8_t res;

  res=_sra8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_sra_iIYN */
  uoc++;
}

static void Ui_ld_r_sll_iIYN(void) {
  uint8_t res;

  res=_sll8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_sll_iIYN */
  uoc++;
}

static void Ui_ld_r_srl_iIYN(void) {
  uint8_t res;

  res=_srl8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_srl_iIYN */
  uoc++;
}

/************************************************************************/

static void Ui_bit_b_iIYN(void) {

  _bit8((opcode>>3)&0x07,_iIYN8(cbop));
  setundocflags8((cpus.IY+u8sval(cbop))>>8); /* weird, huh? */

  z80_clock_inc(16); /* timing&flags taken from ei_bit_b_iIYN */
  uoc++;
}

static void Ui_ld_r_res_b_iIYN(void) {
  uint8_t res;

  res=_res8((opcode>>3)&0x07,_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_res_b_iIYN */
  uoc++;
}

static void Ui_ld_r_set_b_iIYN(void) {
  uint8_t res;

  res=_set8((opcode>>3)&0x07,_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock_inc(19); /* timing&flags taken from ei_set_b_iIYN */
  uoc++;
}

#else

#define Ui_inc_IXh ei_undoc
#define Ui_dec_IXh ei_undoc
#define Ui_ld_IXh_N ei_undoc
#define Ui_inc_IXl ei_undoc
#define Ui_dec_IXl ei_undoc
#define Ui_ld_IXl_N ei_undoc
#define Ui_ld_B_IXh ei_undoc
#define Ui_ld_B_IXl ei_undoc
#define Ui_ld_C_IXh ei_undoc
#define Ui_ld_C_IXl ei_undoc
#define Ui_ld_D_IXh ei_undoc
#define Ui_ld_D_IXl ei_undoc
#define Ui_ld_E_IXh ei_undoc
#define Ui_ld_E_IXl ei_undoc
#define Ui_ld_IXh_B ei_undoc
#define Ui_ld_IXh_C ei_undoc
#define Ui_ld_IXh_D ei_undoc
#define Ui_ld_IXh_E ei_undoc
#define Ui_ld_IXh_IXh ei_undoc
#define Ui_ld_IXh_IXl ei_undoc
#define Ui_ld_IXh_A ei_undoc
#define Ui_ld_IXl_B ei_undoc
#define Ui_ld_IXl_C ei_undoc
#define Ui_ld_IXl_D ei_undoc
#define Ui_ld_IXl_E ei_undoc
#define Ui_ld_IXl_IXh ei_undoc
#define Ui_ld_IXl_IXl ei_undoc
#define Ui_ld_IXl_A ei_undoc
#define Ui_ld_A_IXh ei_undoc
#define Ui_ld_A_IXl ei_undoc
#define Ui_add_A_IXh ei_undoc
#define Ui_add_A_IXl ei_undoc
#define Ui_adc_A_IXh ei_undoc
#define Ui_adc_A_IXl ei_undoc
#define Ui_sub_IXh ei_undoc
#define Ui_sub_IXl ei_undoc
#define Ui_sbc_IXh ei_undoc
#define Ui_sbc_IXl ei_undoc
#define Ui_and_IXh ei_undoc
#define Ui_and_IXl ei_undoc
#define Ui_xor_IXh ei_undoc
#define Ui_xor_IXl ei_undoc
#define Ui_or_IXh ei_undoc
#define Ui_or_IXl ei_undoc
#define Ui_cp_IXh ei_undoc
#define Ui_cp_IXl ei_undoc
#define Ui_inc_IYh ei_undoc
#define Ui_dec_IYh ei_undoc
#define Ui_ld_IYh_N ei_undoc
#define Ui_inc_IYl ei_undoc
#define Ui_dec_IYl ei_undoc
#define Ui_ld_IYl_N ei_undoc
#define Ui_ld_B_IYh ei_undoc
#define Ui_ld_B_IYl ei_undoc
#define Ui_ld_C_IYh ei_undoc
#define Ui_ld_C_IYl ei_undoc
#define Ui_ld_D_IYh ei_undoc
#define Ui_ld_D_IYl ei_undoc
#define Ui_ld_E_IYh ei_undoc
#define Ui_ld_E_IYl ei_undoc
#define Ui_ld_IYh_B ei_undoc
#define Ui_ld_IYh_C ei_undoc
#define Ui_ld_IYh_D ei_undoc
#define Ui_ld_IYh_E ei_undoc
#define Ui_ld_IYh_IYh ei_undoc
#define Ui_ld_IYh_IYl ei_undoc
#define Ui_ld_IYh_A ei_undoc
#define Ui_ld_IYl_B ei_undoc
#define Ui_ld_IYl_C ei_undoc
#define Ui_ld_IYl_D ei_undoc
#define Ui_ld_IYl_E ei_undoc
#define Ui_ld_IYl_IYh ei_undoc
#define Ui_ld_IYl_IYl ei_undoc
#define Ui_ld_IYl_A ei_undoc
#define Ui_ld_A_IYh ei_undoc
#define Ui_ld_A_IYl ei_undoc
#define Ui_add_A_IYh ei_undoc
#define Ui_add_A_IYl ei_undoc
#define Ui_adc_A_IYh ei_undoc
#define Ui_adc_A_IYl ei_undoc
#define Ui_sub_IYh ei_undoc
#define Ui_sub_IYl ei_undoc
#define Ui_sbc_IYh ei_undoc
#define Ui_sbc_IYl ei_undoc
#define Ui_and_IYh ei_undoc
#define Ui_and_IYl ei_undoc
#define Ui_xor_IYh ei_undoc
#define Ui_xor_IYl ei_undoc
#define Ui_or_IYh ei_undoc
#define Ui_or_IYl ei_undoc
#define Ui_cp_IYh ei_undoc
#define Ui_cp_IYl ei_undoc
#define Ui_ednop ei_undoc
#define Ui_neg ei_undoc
#define Ui_im_0 ei_undoc
#define Ui_im_1 ei_undoc
#define Ui_im_2 ei_undoc
#define Ui_reti ei_undoc
#define Ui_retn ei_undoc
#define Ui_ld_r_rlc_iIXN ei_undoc
#define Ui_ld_r_rrc_iIXN ei_undoc
#define Ui_ld_r_rl_iIXN ei_undoc
#define Ui_ld_r_rr_iIXN ei_undoc
#define Ui_ld_r_sla_iIXN ei_undoc
#define Ui_ld_r_sra_iIXN ei_undoc
#define Ui_ld_r_sll_iIXN ei_undoc
#define Ui_ld_r_srl_iIXN ei_undoc
#define Ui_bit_b_iIXN ei_undoc
#define Ui_ld_r_res_b_iIXN ei_undoc
#define Ui_ld_r_set_b_iIXN ei_undoc
#define Ui_ld_r_rlc_iIYN ei_undoc
#define Ui_ld_r_rrc_iIYN ei_undoc
#define Ui_ld_r_rl_iIYN ei_undoc
#define Ui_ld_r_rr_iIYN ei_undoc
#define Ui_ld_r_sla_iIYN ei_undoc
#define Ui_ld_r_sra_iIYN ei_undoc
#define Ui_ld_r_sll_iIYN ei_undoc
#define Ui_ld_r_srl_iIYN ei_undoc
#define Ui_bit_b_iIYN ei_undoc
#define Ui_ld_r_res_b_iIYN ei_undoc
#define Ui_ld_r_set_b_iIYN ei_undoc

#endif


/************************************************************************/
/************************************************************************/

static void Si_stray(void);
static void Mi_dd(void);
static void Mi_fd(void);

#include "z80itab.c"

static void (*const *(ei_opm  [3]))(void) = { ei_op,   ei_ddop,   ei_fdop   };
static void (*const *(ei_cbopm[3]))(void) = { ei_cbop, ei_ddcbop, ei_fdcbop };

static void Mi_dd(void) {
  z80_clock_inc(4);  /* according to Sean: 4T(as NOP), 1R */
  cpus.int_lock=1; /* no interrupts! */
  cpus.modifier=1; /* DD */
}

static void Mi_fd(void) {
  z80_clock_inc(4);  /* according to Sean: 4T(as NOP), 1R */
  cpus.int_lock=1; /* no interrupts! */
  cpus.modifier=2; /* FD */
}

static void Si_stray(void) {  /* DD/FD was stray.. another DD/FD or unprefixed
                         instruction followed */
  smc++;
  ei_opm[0][opcode](); /* execute the instruction(without modifiers) */
}

void z80_resetstat(void) {
#ifndef NO_Z80STAT
  int i,j;
  
  for(i=0;i<7;i++)
    for(j=0;j<256;j++)
      stat_tab[i][j]=0;
#endif
}

unsigned z80_getstat(int tab, uint8_t opcode)
{
#ifndef NO_Z80STAT
	return stat_tab[tab][opcode];
#else
	return 0;
#endif
}

int z80_readinstr(void) {

  opcode=z80_iget8();

  if(opcode==0xed) {
    incr_R(1);
    opcode=z80_iget8();
    ei_tab=ei_edop;
#ifndef NO_Z80STAT
    stat_i=6;
#endif
//    prefix2=0xed;    
    return 0;
  }

  if(opcode==0xcb) {
    if(cpus.modifier!=0) {
      cbop=z80_iget8();
    } else {
      incr_R(1);
    }
    opcode=z80_iget8();
    ei_tab=ei_cbopm[cpus.modifier];
#ifndef NO_Z80STAT
    stat_i=cpus.modifier+3;
#endif
//    prefix2=0xcb;
    return 0;
  }

  /* one-byte opcode */
//  prefix2=0;
  ei_tab=ei_opm[cpus.modifier];
#ifndef NO_Z80STAT
  stat_i=cpus.modifier;
#endif
  return 0;
}

void z80_execinstr(void) {
  int lastuoc;

  /* Process pending NMI or interrupt */
  z80_check_nmi();
  z80_check_int();

  cpus.int_lock=0;

  if(cpus.halted) { /* NOP */
    z80_clock_inc(4);
    incr_R(1);
  } else {
    //printf("read instr..\n");
    z80_readinstr();

    //printf("exec instr..\n");
#ifndef NO_Z80STAT
    stat_tab[stat_i][opcode]++;
#endif
    
    lastuoc=uoc;
    ((void (*)(void))pgm_read_ptr(&ei_tab[opcode]))(); /* FAST branch .. execute the instruction */
    
    (void)lastuoc;
    (void) prefix1;
    (void) prefix2;
/*    switch(cpus.modifier) {
      case 0: prefix1=0;    break;
      case 1: prefix1=0xdd; break;
      case 2: prefix1=0xfd; break;
    }  
    if(uoc>lastuoc) fprintf(logfi,"undoc opcode %02x\n",(prefix1<<16)|(prefix2<<8)|opcode);
*/
    incr_R(1);
    
    /* turn off old modifier prefix (unless set just now) */
    if(opcode!=0xdd && opcode!=0xfd) cpus.modifier=0;
  }
#ifdef NO_Z80CLOCK
	z80_clock += 12;
#endif
}

static void z80_check_int(void) {
  uint16_t addr;
  uint8_t data;
//  printf("interrupt->DI\n");

  if(!cpus.int_pending) return;
  if(cpus.int_lock) return; /* after EI or DI or inside an instruction */

  /* Clear interrupt pending flag */
  cpus.int_pending=0;

  /* If interrupts are disabled, just drop the interrupt */
  if(!cpus.IFF1) return; /* IFF2 just tells the NMI service routine
                               whether the interrupted program disabled
			       maskable interrupts */
  cpus.halted=0;
  cpus.IFF1=cpus.IFF2=0;

  switch(cpus.int_mode) {
    case 0: /* this is not quite right yet ..*/
      /* the actual instruction should be read from the data bus */
      /* but if that instruction were longer than 1 byte? */
      incr_R(2);
      _push16(cpus.PC);
      cpus.PC=0x0038;
      z80_clock_inc(13);
      break;
    case 1:
      incr_R(2);
      _push16(cpus.PC);
      cpus.PC=0x0038;
      z80_clock_inc(13);
      break;
    case 2:
      incr_R(2);
      _push16(cpus.PC);
      data = z80_snoop8();
      addr=((uint16_t)cpus.I<<8) | (uint16_t)data;
      cpus.PC=z80_memget16(addr);
//      printf("IM 2 INT: I=%02x D8=%02x tabi=%04x dst=%04x\n",cpus.I,data,addr,cpus.PC);
      z80_clock_inc(19);
      break;
  }
}

static void z80_check_nmi(void) {

  if(!cpus.nmi_pending) return;
  if(cpus.int_lock) return;
//  printf("NMI!\n");
  
  cpus.nmi_pending=0;
  cpus.halted=0;
  cpus.IFF1=0;

  _push16(cpus.PC);
  cpus.PC=0x0066;
  
  incr_R(2);
  z80_clock_inc(11);
}

void z80_int(void)
{
  cpus.int_pending = 1;
}

void z80_nmi(void)
{
  cpus.nmi_pending = 1;
}

int z80_reset(void) {
  cpus.IFF1=cpus.IFF2=0;
  cpus.int_mode=0;
  cpus.int_pending=0;
  cpus.nmi_pending=0;
  cpus.PC=0;

  cpus.SP=0;
  cpus.I=0;
  cpus.R=0;
  
  cpus.halted=0;
  cpus.int_lock=0;
  cpus.modifier=0;
  
  cpus.r[rA]=0;  cpus.F=0;
  cpus.r[rB]=0;  cpus.r[rC]=0;
  cpus.r[rD]=0;  cpus.r[rE]=0;
  cpus.r[rH]=0;  cpus.r[rL]=0;
  cpus.IX=0;     cpus.IY=0;
  cpus.r_[rA]=0; cpus.F_=0;
  cpus.r_[rB]=0; cpus.r_[rC]=0;
  cpus.r_[rD]=0; cpus.r_[rE]=0;
  cpus.r_[rH]=0; cpus.r_[rL]=0;

  return 0;
}
