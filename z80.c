/*
  GZX - George's ZX Spectrum Emulator
  Z80 CPU emulation
  
  ackoli je naprosta vetsina vlajek implementovana,
  (krome i/o blokovych instrukci a vlajek 5,3 u BIT (HL))
  dosti her nefunguje... proc? asi nejake stenice.
  
  objevene nefungujici hry:
    belegost (chcipne)
    tapper (chcipne)
    
  !spemu		not implemented right in spectemu
  
  udelat:
    drive
    - vlajky i/o blokovych instrukci
    pozdeji
    - BIT (HL) - vlajky 3,5
    - casovani undoc instrukci
  
  opraveno:
    (IX+N) (IY+N) N=-128..127, nikoliv 0..255
    dvojkovy doplnek NEG
    emulace CP/LD/IN/OT..I/D/IR/DR po jednotlivych krocich
    pri zvetsovani R registru se nemeni bit 7
    LD A,I a LD A,R kopiruji do P/V vlajky IFF2, nikoli paritu
    vsechny RETI/RETN kopiruji IFF2 do IFF1
    _cp8 kopiruje vlajky 5,3 z mensitele, ne z vysledku
    u IM 2 chybel push(PC) !@#
    timingy CPIR/DR,LDIR/DR,OTIR/DR,INIR/DR,CALL cond,JP cond
    SLL doplnuje do bitu 0 carry, nikoliv jednicku!
    typy argumentu adc_v16,sbc_v16,sub_v16
    dec8: sub_v8(a,1) misto sub_v8(a,a-1)
    
  udelano:
    - DD/FD (normal/stray) modifiers full support!
      (execution of instructions thus broken into parts)
      .. timing changes!
    - vsechny unsuported opcody implementovany (alespon priblizne spravne)
    - int_lock zabrani preruseni po EI ci DI nebo uvnitr instrukce
    - spravne casovani T a R pro INT a NMI
    - "nedokumentovane"(ale pouzivane) I/O 16-bitove adresy
    - _in8 nuluje H flag
    - implementovany VSECHNY vlajky (vcetne nedokumentovanych) pro:
          > 8-bitova aritmetika & logika
	  > 16-bitova aritmetika
	  > BIT (krome (HL))
	  > ostatni neblokove operace
	  > pametove blokove operace (LD..,CP..)
      (pokud tam nemam chyby)
    
*/

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "memio.h"
#include "z80.h"

static void z80_printstatus(void);
static int z80_readinstr(void);

static u8 opcode;
z80s cpus;
static u8 cbop;
unsigned long z80_clock;

/* debugging & statistics */
unsigned long uoc;	/* unsupported opcode counter */
unsigned long smc;	/* stray modifier counter */
static u8 prefix1,prefix2;

static void (**ei_tab)(void);
unsigned stat_tab[7][256];
static int stat_i;

/* tabulky pro urychleni vypoctu vlajek */
static u8 ox_tab[256]; /* OR,XOR a dalsi */

/* returns the signed value of the byte: 0..127 ->0..127
                                         128..255 ->-128..-1 */
static int u8sval(u8 u) {
  if(u<0x80) return u;
    else return (int)(u&0x7f)-128;
}

static int u16sval(u16 u) {
  if(u<0x8000) return u;
    else return (int)(u&0x7fff)-32768;
}

/********************** flags calculation ****************************/

/*  these functions return 1 when sign overflow occurs */
/*  (the result would be >127 or <-128) */

static int add_v8(u8 a, u8 b) {
  int sign_r;
  sign_r=u8sval(a)+u8sval(b);
  return sign_r<-128 || sign_r>127;
}

static int sub_v8(u8 a, u8 b) {
  int sign_r;
  sign_r=u8sval(a)-u8sval(b);
  return sign_r<-128 || sign_r>127;
}

static int adc_v8(u8 a, u8 b, u8 c) {
  int sign_r;
  sign_r=u8sval(a)+u8sval(b)+u8sval(c);
  return sign_r<-128 || sign_r>127;
}

static int sbc_v8(u8 a, u8 b, u8 c) {
  int sign_r;
  sign_r=u8sval(a)-u8sval(b)-c;
  return sign_r<-128 || sign_r>127;
}

static int adc_v16(u16 a, u16 b, u16 c) {
  int sign_r;
  sign_r=u16sval(a)+u16sval(b)+u16sval(c);
  return sign_r<-32768 || sign_r>32767;
}

static int sbc_v16(u16 a, u16 b, u16 c) {
  int sign_r;
  sign_r=u16sval(a)-u16sval(b)-c;
  return sign_r<-32768 || sign_r>32767;
}

/* calculates an even parity for the given 8-bit number */
static int oddp8(u8 x) {
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

static void setundocflags8(u8 res) {
  cpus.F &= fD;		/* leave only documented flags */
  cpus.F |= (res & fU);     /* set undocumented flags */
}

static void incr_R(u8 amount) {
  cpus.R = (cpus.R & 0x80) | ((cpus.R+amount)&0x7f);
}

/**************************** operand access ***************************/

/* returns (HL)(8) */
static u8 _iHL8(void) {
  return zx_memget8(((u16)cpus.r[rH]<<8)|(u16)cpus.r[rL]);
}

/* returns (BC) */
static u8 _iBC8(void) {
  return zx_memget8(((u16)cpus.r[rB]<<8)|(u16)cpus.r[rC]);
}

/* returns (DE) */
static u8 _iDE8(void) {
  return zx_memget8(((u16)cpus.r[rD]<<8)|(u16)cpus.r[rE]);
}

/* returns (IX+N) */
static u8 _iIXN8(u16 N) {
  return zx_memget8(cpus.IX+u8sval(N));
}

/* returns (IY+N) */
static u8 _iIYN8(u16 N) {
  return zx_memget8(cpus.IY+u8sval(N));
}

/* (IX+N) <- val*/
static void s_iIXN8(u16 N, u8 val) {
  zx_memset8(cpus.IX+u8sval(N),val);
}

/* (IY+N) <- val*/
static void s_iIYN8(u16 N, u8 val) {
  zx_memset8(cpus.IY+u8sval(N),val);
}


/* (HL) <- val */
static void s_iHL8(u8 val) {
  zx_memset8(((u16)cpus.r[rH]<<8)|(u16)cpus.r[rL],val);
}

/* (BC) <- val */
static void s_iBC8(u8 val) {
  zx_memset8(((u16)cpus.r[rB]<<8)|(u16)cpus.r[rC],val);
}

/* (DE) <- val */
static void s_iDE8(u8 val) {
  zx_memset8(((u16)cpus.r[rD]<<8)|(u16)cpus.r[rE],val);
}

/* returns (SP)(16-bits) */
static u16 _iSP16(void) {
  return zx_memget16(cpus.SP);
}

/* (SP)(16-bits) <- val */
static void s_iSP16(u16 val) {
  zx_memset16(cpus.SP,val);
}

static u16 getAF(void) {
  return ((u16)cpus.r[rA] << 8)|(u16)cpus.F;
}

static u16 getBC(void) {
  return ((u16)cpus.r[rB] << 8)|(u16)cpus.r[rC];
}

static u16 getDE(void) {
  return ((u16)cpus.r[rD] << 8)|(u16)cpus.r[rE];
}

static u16 getHL(void) {
  return ((u16)cpus.r[rH] << 8)|(u16)cpus.r[rL];
}

static u16 getAF_(void) {
  return ((u16)cpus.r_[rA] << 8)|(u16)cpus.F_;
}

static u16 getBC_(void) {
  return ((u16)cpus.r_[rB] << 8)|(u16)cpus.r_[rC];
}

static u16 getDE_(void) {
  return ((u16)cpus.r_[rD] << 8)|(u16)cpus.r_[rE];
}

static u16 getHL_(void) {
  return ((u16)cpus.r_[rH] << 8)|(u16)cpus.r_[rL];
}

static void setAF(u16 val) {
  cpus.r[rA]=val>>8;
  cpus.F=val & 0xff;
}

static void setBC(u16 val) {
  cpus.r[rB]=val>>8;
  cpus.r[rC]=val & 0xff;
}

static void setDE(u16 val) {
  cpus.r[rD]=val>>8;
  cpus.r[rE]=val & 0xff;
}

static void setHL(u16 val) {
  cpus.r[rH]=val>>8;
  cpus.r[rL]=val & 0xff;
}


static u8 z80_iget8(void) {
  u8 tmp;

  tmp=zx_memget8(cpus.PC);
  cpus.PC++;
  return tmp;
}

static u16 z80_iget16(void) {
  u16 tmp;

  tmp=zx_memget16(cpus.PC);
  cpus.PC+=2;
  return tmp;
}

static void z80_printstatus(void) {
  printf("AF%04x BC%04x DE%04x HL%04x IX%04x IY%04x PC%04x SP%04x iHL%02x\n",
         getAF(),getBC(),getDE(),getHL(),cpus.IX, cpus.IY,
	 cpus.PC,cpus.SP,_iHL8());
}

static void z80_dump_regs(void)
{
	static FILE *drf = NULL;

	if (!drf) {
		drf = fopen("dregs.txt","wt");
		if (!drf) exit(1);
	}
	fprintf(drf,"PC=%04X, A=%02X, F=%02X, BC=%04X, DE=%04X, HL=%04X, IX=%04X, IY=%04X\n",
		cpus.PC, cpus.r[rA], cpus.F, getBC(), getDE(), getHL(), cpus.IX, cpus.IY);
	fprintf(drf,"SP=%04X, (SP)=%04X\n", cpus.SP, zx_memget16(cpus.SP));
	//fprintf(drf,"(5CA9)=%02X\n", zx_memget8(0x5CA9));
}

void z80_fprintstatus(FILE *logfi) {
  fprintf(logfi,"AF %04x BC %04x DE %04x HL %04x IX %04x PC %04x R%02d iHL%02x\n",
          getAF()&0xffd7,getBC(),getDE(),getHL(),cpus.IX,
	  cpus.PC,cpus.R,_iHL8());
  fprintf(logfi,"AF'%04x BC'%04x DE'%04x HL'%04x IY %04x SP'%04x I%02d\n",
          getAF_()&0xffd7,getBC_(),getDE_(),getHL_(),cpus.IY,
	  cpus.SP,cpus.I);
}

/******************* undocumented operand access ************************/

static void setIXh(u8 val) {
  cpus.IX = (cpus.IX & 0x00ff) | ((u16)val<<8);
}

static void setIYh(u8 val) {
  cpus.IY = (cpus.IY & 0x00ff) | ((u16)val<<8);
}

static void setIXl(u8 val) {
  cpus.IX = (cpus.IX & 0xff00) | (u16)val;
}

static void setIYl(u8 val) {
  cpus.IY = (cpus.IY & 0xff00) | (u16)val;
}

static u8 getIXh(void) {
  return cpus.IX>>8;
}

static u8 getIYh(void) {
  return cpus.IY>>8;
}

static u8 getIXl(void) {
  return cpus.IX&0xff;
}

static u8 getIYl(void) {
  return cpus.IY&0xff;
}


/************************************************************************/
static void _push16(u16 val);
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

static u8 _adc8(u16 a, u16 b) {
  u16 res;
  u16 c;
  
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

static u16 _adc16(u16 a, u16 b) {
  u16 res0,res1,a1,b1,c,c1;
  
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

static u8 _add8(u16 a, u16 b) {
  u16 res;

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

static u16 _add16(u16 a, u16 b) {
  u16 res0,res1,a1,b1;

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

static u8 _and8(u8 a, u8 b) {
  u8 res;

  res=a&b;
  cpus.F=ox_tab[res]|fHC;
  return res;
}

/************************************************************************/

static u8 _bit8(u8 a, u8 b) {
  u8 res;

  res=b & (1<<a);
 // printf("%04x: bit %d,0x%02x [%d]\n",cpus.PC,a,b,res);
  cpus.F=(cpus.F&fC)|ox_tab[res]; /* CF se nemeni */
/*  setflags(res&0x80,
	   res==0,
	   1,
	   res==0, 
	   0,
	   -1);*/
	
  /* nedokumentovane vlajky jsou ruzne pro ruzne opcody. viz BIT opcody. */
  return res;
}

/************************************************************************/

static void _call16(u16 addr) {
  _push16(cpus.PC);
  cpus.PC=addr;
}

/************************************************************************/

static u8 _cp8(u16 a, u16 b) {
  u16 res;

  res=a-b;
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (a&0x0f)-(b&0x0f) < 0, /* doufam */
	   sub_v8(a,b),
	   1,
	   res>0xff);
  setundocflags8(b); /* not from the result! */
  return res & 0xff;
}

/************************************************************************/

static u8 _dec8(u16 a) {
  u16 res;

  res=(a-1)&0xff;
  setflags(res>>7,
	   res==0,
	   (a&0x0f)-1<0, /* doufam */
	   sub_v8(a,1),
	   1,    /* !spemu */
	   -1);
  setundocflags8(res);
  return res;
}

/************************************************************************/

static u8 _in8pf(u16 a) {
  return zx_in8(a);		/* query ZX */
}

static u8 _in8(u16 a) {
  u16 res;

  res=_in8pf(a)&0xff;
  setflags(res>>7,
	   res==0,
	   0,
	   oddp8(res),
	   0,
	   -1);
  return res;
}

/************************************************************************/

static u8 _inc8(u16 a) {
  u16 res;

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


static void _jp16(u16 addr) {
  cpus.PC=addr;
}

/************************************************************************/

static void _jr8(u8 ofs) {
  cpus.PC+=u8sval(ofs);
}

/************************************************************************/

static u8 _or8(u8 a, u8 b) {
  u8 res;

  res=a|b;
  cpus.F=ox_tab[res];
  return res;
}

/************************************************************************/

static void _out8(u16 addr, u8 val) {
  zx_out8(addr,val);			/* pass it to ZX */
}

/************************************************************************/

static void _push16(u16 val) {
  cpus.SP-=2;
  zx_memset16(cpus.SP,val);
}

static u16 _pop16(void) {
  u16 res;

  res=zx_memget16(cpus.SP);
  cpus.SP+=2;
  return res;
}

/************************************************************************/

static u8 _res8(u16 a, u16 b) {
  u16 res;

  res=b & ((1<<a)^0xff);
  return res;
}

/************************************************************************/

static u8 _rla8(u8 a) {
  u8 nC,oC;

  nC=a>>7;
  oC=(cpus.F & fC)?1:0;
  a=(a<<1)|oC;
  cpus.F = (cpus.F & ~(fU1|fHC|fU2|fN|fC)) | (a&(fU1|fU2)) | nC;
  return a;
}

static u8 _rl8(u8 a) {
  u8 nC,oC;

  nC=a>>7;
  oC=(cpus.F & fC)?1:0;
  a=(a<<1)|oC;
  cpus.F=ox_tab[a]|nC;
  return a;
}

static u8 _rlca8(u8 a) {
  u8 tmp;

  tmp=a>>7;
  a=(a<<1)|tmp;
  cpus.F = (cpus.F & ~(fU1|fHC|fU2|fN|fC)) | (a&(fU1|fU2)) | tmp;
  return a;
}

static u8 _rlc8(u8 a) {
  u8 tmp;

  tmp=a>>7;
  a=(a<<1)|tmp;
  cpus.F=ox_tab[a]|tmp;
  return a;
}

static u8 _rra8(u8 a) {
  u8 nC,oC;

  nC=a&1;
  oC=(cpus.F & fC)?1:0;
  a=(a>>1)|(oC<<7);
  cpus.F = (cpus.F & ~(fU1|fHC|fU2|fN|fC)) | (a&(fU1|fU2)) | nC;
  return a;
}

static u8 _rr8(u8 a) {
  u8 nC,oC;

  nC=a&1;
  oC=(cpus.F & fC)?1:0;
  a=(a>>1)|(oC<<7);
  cpus.F=ox_tab[a]|nC;
  return a;
}

static u8 _rrca8(u8 a) {
  u8 tmp;

  tmp=a&1;
  a=(a>>1)|(tmp<<7);
  cpus.F = (cpus.F & ~(fU1|fHC|fU2|fN|fC)) | (a&(fU1|fU2)) | tmp;
  return a;
}

static u8 _rrc8(u8 a) {
  u8 tmp;

  tmp=a&1;
  a=(a>>1)|(tmp<<7);
  cpus.F=ox_tab[a]|tmp;
  return a;
}



/************************************************************************/

static u8 _sla8(u8 a) {
  u8 nC;

  nC=a>>7;
  a<<=1;
  cpus.F=ox_tab[a]|nC;
  return a;
}

static u8 _sra8(u8 a) {
  u8 nC;

  nC=a&1;
  a=(a&0x80) | (a>>1);
  cpus.F=ox_tab[a]|nC;
  return a;
}

static u8 _sll8(u8 a) {
  u8 nC,oC;

  nC=a>>7;
  oC=(cpus.F & fC)?1:0;
  a=(a<<1)|oC;
  cpus.F=ox_tab[a]|nC;
  return a;
}

static u8 _srl8(u16 a) {
  u16 nC;

  nC=a&1;
  a>>=1;
  cpus.F=ox_tab[a]|nC;
  return a;
}

/************************************************************************/

static u8 _sbc8(u16 a, u16 b) {
  u16 res;
  u16 c;
  
  c=((cpus.F&fC)!=0);

  res=a-b-c;
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (a&0x0f)-(b&0x0f)-c < 0, /* doufam */
	   sbc_v8(a,b,c),
	   1,
	   res>0xff);
  setundocflags8(res);
  return res & 0xff;
}

static u16 _sbc16(u16 a, u16 b) {
  u16 res0,res1,a1,b1,c,c1;
  
  c=((cpus.F&fC)?1:0);

  res0=(a&0xff)-(b&0xff)- c;
  a1=a>>8; b1=b>>8; c1=((res0>0xff)?1:0);
  res1=a1-b1-c1;
  setflags(res1&0x80,
	   !((res0&0xff)|(res1&0xff)),
	   (a1&0x0f) - (b1&0x0f)<0,
	   sbc_v16(a,b,c),
	   0,
	   res1>0xff);
  setundocflags8(res1);
  return (res0&0xff)|((res1&0xff)<<8);
}

/************************************************************************/

static u8 _set8(u16 a, u16 b) {
  u16 res;

  res=b | (1<<a);
  return res;
}

/************************************************************************/


static u8 _sub8(u16 a, u16 b) {
  u16 res;

  res=a-b;
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (a&0x0f)-(b&0x0f) < 0, /* doufam */
	   sub_v8(a,b),
	   1,
	   res>0xff);
  setundocflags8(res);
  return res & 0xff;
}

/************************************************************************/

static u8 _xor8(u8 a, u8 b) {
  u8 res;

  res=a^b;
  cpus.F=ox_tab[res];
  return res;
}



/************************************************************************/
/************************************************************************/
/********************* documented opcodes *******************************/

static void ei_adc_A_r(void) {
  u8 res;

  res=_adc8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_adc_A_N(void) {
  u8 res,op;

  op=z80_iget8();

  res=_adc8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_adc_A_iHL(void) {
  u8 res;

  res=_adc8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_adc_A_iIXN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_adc8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

static void ei_adc_A_iIYN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_adc8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

static void ei_adc_HL_BC(void) {
  u16 res;

  res=_adc16(getHL(),getBC());
  setHL(res);

  z80_clock+=15;
}

static void ei_adc_HL_DE(void) {
  u16 res;

  res=_adc16(getHL(),getDE());
  setHL(res);

  z80_clock+=15;
}

static void ei_adc_HL_HL(void) {
  u16 res;

  res=_adc16(getHL(),getHL());
  setHL(res);

  z80_clock+=15;
}

static void ei_adc_HL_SP(void) {
  u16 res;

  res=_adc16(getHL(),cpus.SP);
  setHL(res);

  z80_clock+=15;
}

/************************************************************************/

static void ei_add_A_r(void) {
  u8 res;

  res=_add8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_add_A_N(void) {
  u8 res,op;

  op=z80_iget8();

  res=_add8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_add_A_iHL(void) {
  u8 res;

  res=_add8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_add_A_iIXN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_add8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

static void ei_add_A_iIYN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_add8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

static void ei_add_HL_BC(void) {
  u16 res;

  res=_add16(getHL(),getBC());
  setHL(res);

  z80_clock+=11;
}

static void ei_add_HL_DE(void) {
  u16 res;

  res=_add16(getHL(),getDE());
  setHL(res);

  z80_clock+=11;
}

static void ei_add_HL_HL(void) {
  u16 res;

  res=_add16(getHL(),getHL());
  setHL(res);

  z80_clock+=11;
}

static void ei_add_HL_SP(void) {
  u16 res;

  res=_add16(getHL(),cpus.SP);
  setHL(res);

  z80_clock+=11;
}

static void ei_add_IX_BC(void) {
  u16 res;

  res=_add16(cpus.IX,getBC());
  cpus.IX=res;

  z80_clock+=11;
}

static void ei_add_IX_DE(void) {
  u16 res;

  res=_add16(cpus.IX,getDE());
  cpus.IX=res;

  z80_clock+=11;
}

static void ei_add_IX_IX(void) {
  u16 res;

  res=_add16(cpus.IX,cpus.IX);
  cpus.IX=res;

  z80_clock+=11;
}

static void ei_add_IX_SP(void) {
  u16 res;

  res=_add16(cpus.IX,cpus.SP);
  cpus.IX=res;

  z80_clock+=11;
}

static void ei_add_IY_BC(void) {
  u16 res;

  res=_add16(cpus.IY,getBC());
  cpus.IY=res;

  z80_clock+=11;
}

static void ei_add_IY_DE(void) {
  u16 res;

  res=_add16(cpus.IY,getDE());
  cpus.IY=res;

  z80_clock+=11;
}

static void ei_add_IY_IY(void) {
  u16 res;

  res=_add16(cpus.IY,cpus.IY);
  cpus.IY=res;

  z80_clock+=11;
}

static void ei_add_IY_SP(void) {
  u16 res;

  res=_add16(cpus.IY,cpus.SP);
  cpus.IY=res;

  z80_clock+=11;
}

/************************************************************************/

static void ei_and_r(void) {
  u8 res;

  res=_and8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_and_N(void) {
  u8 res,op;

  op=z80_iget8();

  res=_and8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_and_iHL(void) {
  u8 res;

  res=_and8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_and_iIXN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_and8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

static void ei_and_iIYN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_and8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

/************************************************************************/

static void ei_bit_b_r(void) {
  _bit8((opcode>>3)&0x07,cpus.r[opcode & 0x07]);

  z80_clock+=8;
}

static void ei_bit_b_iHL(void) {
  _bit8((opcode>>3)&0x07,_iHL8());
  /* undoc flags are set in a VERY weird way here.
     I didn't implement this yet. */

  z80_clock+=12;
}

/* DDCB ! */
static void ei_bit_b_iIXN(void) {
  _bit8((opcode>>3)&0x07,_iIXN8(cbop));
  setundocflags8((cpus.IX+u8sval(cbop))>>8); /* weird, huh? */

  z80_clock+=16;
}

/* FDCB ! */
static void ei_bit_b_iIYN(void) {

  _bit8((opcode>>3)&0x07,_iIYN8(cbop));
  setundocflags8((cpus.IY+u8sval(cbop))>>8); /* weird, huh? */

  z80_clock+=16;
}

/************************************************************************/

static void ei_call_NN(void) {
  u16 addr;

  addr=z80_iget16();
  _call16(addr);
  z80_clock+=17;
}

static void ei_call_C_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(cpus.F & fC) {
    _call16(addr);
    z80_clock+=17;
  } else z80_clock+=10;
}

static void ei_call_NC_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(!(cpus.F & fC)) {
    _call16(addr);
    z80_clock+=17;
  } else z80_clock+=10;
}

static void ei_call_M_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(cpus.F & fS) {
    _call16(addr);
    z80_clock+=17;
  } else z80_clock+=10;
}

static void ei_call_P_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(!(cpus.F & fS)) {
    _call16(addr);
    z80_clock+=17;
  } else z80_clock+=10;
}

static void ei_call_Z_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(cpus.F & fZ) {
    _call16(addr);
    z80_clock+=17;
  } else z80_clock+=10;
}

static void ei_call_NZ_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(!(cpus.F & fZ)) {
    _call16(addr);
    z80_clock+=17;
  } else z80_clock+=10;
}

static void ei_call_PE_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(cpus.F & fPV) {
    _call16(addr);
    z80_clock+=17;
  } else z80_clock+=10;
}

static void ei_call_PO_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(!(cpus.F & fPV)) {
    _call16(addr);
    z80_clock+=17;
  } else z80_clock+=10;
}

/************************************************************************/

static void ei_ccf(void) { /* complement carry flag */
  u8 nHC;
  
  nHC=(cpus.F&fC)?fHC:0;
  cpus.F= ((cpus.F ^ fC) & ~(fU1|fHC|fU2|fN)) | nHC | (cpus.r[rA]&(fU1|fU2));
  z80_clock+=4;
}

/************************************************************************/

static void ei_cp_r(void) {
  _cp8(cpus.r[rA],cpus.r[opcode & 0x07]);
  z80_clock+=4;
}

static void ei_cp_N(void) {
  u8 op;

  op=z80_iget8();

  _cp8(cpus.r[rA],op);
  z80_clock+=7;
}

static void ei_cp_iHL(void) {
  _cp8(cpus.r[rA],_iHL8());
  z80_clock+=7;
}

static void ei_cp_iIXN(void) {
  u8 op;

  op=z80_iget8();

  _cp8(cpus.r[rA],_iIXN8(op));
  z80_clock+=15;
}

static void ei_cp_iIYN(void) {
  u8 op;

  op=z80_iget8();

  _cp8(cpus.r[rA],_iIYN8(op));
  z80_clock+=15;
}

static void ei_cpd(void) {
  u8 a,b,ufr,res;
  u16 newBC;

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

  z80_clock+=16;
}

static void ei_cpdr(void) {
  u8 a,b,ufr,res;
  u16 newBC;

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
    z80_clock+=16;
  } else {
    z80_clock+=21;
    cpus.PC-=2;
  }
}

static void ei_cpi(void) {
  u8 a,b,ufr,res;
  u16 newBC;

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

  z80_clock+=16;
}

static void ei_cpir(void) {
  u8 a,b,ufr,res;
  u16 newBC;

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
    z80_clock+=16;
  } else {
    z80_clock+=21;
    cpus.PC-=2;
  }
}

/************************************************************************/

static void ei_cpl(void) { /* A <- cpl(A) ... one's complement */
//  printf("CPL(1)\n");
  cpus.r[rA] ^= 0xff;
  setflags(-1,
           -1,
	   1,
	   -1,
	   1,
	   -1);
  setundocflags8(cpus.r[rA]);
  z80_clock+=4;
}


/************************************************************************/

static void ei_daa(void) {
  u16 res;
  
//  printf("DAA - _fully_ implemented !\n");
  
  res=cpus.r[rA];
  
  if((cpus.F & fN)==0) {
    if(cpus.F & fC) res += 0x60;
      else if(res>0x99) { res += 0x60; cpus.F|=fC; }
      
    if(cpus.F & fHC) res += 0x06;
      else if((res&0x0f)>0x09) { res += 0x06; cpus.F|=fHC; }
  } else {
    if(cpus.F & fC) res -= 0x60;
      else if(res>0x99) { res -= 0x60; cpus.F|=fC; }
      
    if(cpus.F & fHC) res -= 0x06;
      else if((res&0x0f)>0x09) { res -= 0x06; cpus.F|=fHC; }
  }
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   -1,
	   oddp8(res&0xff),
	   -1,
	   -1);
  setundocflags8(res);
  
  cpus.r[rA] = res & 0xff;
  
  z80_clock+=4;
}

/************************************************************************/

static void ei_dec_A(void) {
  u8 res;

  res=_dec8(cpus.r[rA]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_dec_B(void) {
  u8 res;

  res=_dec8(cpus.r[rB]);
  cpus.r[rB]=res;

  z80_clock+=4;
}

static void ei_dec_C(void) {
  u8 res;

  res=_dec8(cpus.r[rC]);
  cpus.r[rC]=res;

  z80_clock+=4;
}

static void ei_dec_D(void) {
  u8 res;

  res=_dec8(cpus.r[rD]);
  cpus.r[rD]=res;

  z80_clock+=4;
}

static void ei_dec_E(void) {
  u8 res;

  res=_dec8(cpus.r[rE]);
  cpus.r[rE]=res;

  z80_clock+=4;
}

static void ei_dec_H(void) {
  u8 res;

  res=_dec8(cpus.r[rH]);
  cpus.r[rH]=res;

  z80_clock+=4;
}

static void ei_dec_L(void) {
  u8 res;

  res=_dec8(cpus.r[rL]);
  cpus.r[rL]=res;

  z80_clock+=4;
}

static void ei_dec_iHL(void) {
  u8 res;

  res=_dec8(_iHL8());
  s_iHL8(res);

  z80_clock+=11;
}

static void ei_dec_iIXN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_dec8(_iIXN8(op));
  s_iIXN8(op,res);

  z80_clock+=19;
}

static void ei_dec_iIYN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_dec8(_iIYN8(op));
  s_iIYN8(op,res);

  z80_clock+=19;
}

static void ei_dec_BC(void) {

  setBC(getBC()-1);
  z80_clock+=6;
}

static void ei_dec_DE(void) {

  setDE(getDE()-1);
  z80_clock+=6;
}

static void ei_dec_HL(void) {

  setHL(getHL()-1);
  z80_clock+=6;
}

static void ei_dec_SP(void) {

  cpus.SP--;
  z80_clock+=6;
}

static void ei_dec_IX(void) {

  cpus.IX--;
  z80_clock+=6;
}

static void ei_dec_IY(void) {

  cpus.IY--;
  z80_clock+=6;
}

/************************************************************************/

static void ei_di(void) {
//  printf("DI\n");
  cpus.IFF1=cpus.IFF2=0;
  cpus.int_lock=1;
  z80_clock+=4;
}

/************************************************************************/

static void ei_djnz(void) {
  u8 ofs;
  
  ofs=z80_iget8();
  cpus.r[rB]--;
  if(cpus.r[rB]!=0) {
    _jr8(ofs);
    z80_clock+=13;
  } else z80_clock+=8;
}

/************************************************************************/

static void ei_ei(void) {
//  printf("EI\n");
  cpus.IFF1=cpus.IFF2=1;
  cpus.int_lock=1;
  z80_clock+=4;
}

/************************************************************************/

static void ei_ex_iSP_HL(void) {
  u16 tmp;

  tmp=_iSP16();
  s_iSP16(getHL());
  setHL(tmp);

  z80_clock+=19;
}

static void ei_ex_iSP_IX(void) {
  u16 tmp;

  tmp=_iSP16();
  s_iSP16(cpus.IX);
  cpus.IX=tmp;

  z80_clock+=19;
}

static void ei_ex_iSP_IY(void) {
  u16 tmp;

  tmp=_iSP16();
  s_iSP16(cpus.IY);
  cpus.IY=tmp;

  z80_clock+=19;
}

static void ei_ex_AF_xAF(void) {
  u8 tmp;

  tmp=cpus.r[rA]; cpus.r[rA]=cpus.r_[rA]; cpus.r_[rA]=tmp;
  tmp=cpus.F; cpus.F=cpus.F_; cpus.F_=tmp;

  z80_clock+=4;
}

static void ei_ex_DE_HL(void) {
  u16 tmp;

  tmp=getDE(); setDE(getHL()); setHL(tmp);
  z80_clock+=4;
}

static void ei_exx(void) {
  u8 tmp;

  tmp=cpus.r[rB]; cpus.r[rB]=cpus.r_[rB]; cpus.r_[rB]=tmp;
  tmp=cpus.r[rC]; cpus.r[rC]=cpus.r_[rC]; cpus.r_[rC]=tmp;
  tmp=cpus.r[rD]; cpus.r[rD]=cpus.r_[rD]; cpus.r_[rD]=tmp;
  tmp=cpus.r[rE]; cpus.r[rE]=cpus.r_[rE]; cpus.r_[rE]=tmp;
  tmp=cpus.r[rH]; cpus.r[rH]=cpus.r_[rH]; cpus.r_[rH]=tmp;
  tmp=cpus.r[rL]; cpus.r[rL]=cpus.r_[rL]; cpus.r_[rL]=tmp;

  z80_clock+=4;
}


/************************************************************************/


static void ei_halt(void) {
//  printf("halt!\n");
  cpus.halted=1;

  z80_clock+=4;
}


/************************************************************************/

static void ei_im_0(void) {
  cpus.int_mode=0;
  z80_clock+=8;
}

static void ei_im_1(void) {
  cpus.int_mode=1;
  z80_clock+=8;
}

static void ei_im_2(void) {
  cpus.int_mode=2;
  z80_clock+=8;
}

/************************************************************************/

static void ei_in_A_iN(void) {
  u8 res;
  u16 op;

//  printf("ei_in_A_iN!!!\n");
//  z80_printstatus();
  op=z80_iget8();

  res=_in8pf(((u16)cpus.r[rA]<<8)|(u16)op);
  cpus.r[rA]=res;

  z80_clock+=11;
}

static void Ui_in_iC(void) {

  printf("ei_in_iC (unsupported)\n");
  _in8(getBC());

  uoc++;
  z80_clock+=12;
}

static void ei_in_A_iC(void) {
  u8 res;

  res=_in8(getBC());
  cpus.r[rA]=res;

  z80_clock+=12;
}

static void ei_in_B_iC(void) {
  u8 res;

  res=_in8(getBC());
  cpus.r[rB]=res;

  z80_clock+=12;
}

static void ei_in_C_iC(void) {
  u8 res;

  res=_in8(getBC());
  cpus.r[rC]=res;

  z80_clock+=12;
}

static void ei_in_D_iC(void) {
  u8 res;

  res=_in8(getBC());
  cpus.r[rD]=res;

  z80_clock+=12;
}

static void ei_in_E_iC(void) {
  u8 res;

  res=_in8(getBC());
  cpus.r[rE]=res;

  z80_clock+=12;
}

static void ei_in_H_iC(void) {
  u8 res;

  res=_in8(getBC());
  cpus.r[rH]=res;

  z80_clock+=12;
}

static void ei_in_L_iC(void) {
  u8 res;

  res=_in8(getBC());
  cpus.r[rL]=res;

  z80_clock+=12;
}

/************************************************************************/

static void ei_inc_A(void) {
  u8 res;

  res=_inc8(cpus.r[rA]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_inc_B(void) {
  u8 res;

  res=_inc8(cpus.r[rB]);
  cpus.r[rB]=res;

  z80_clock+=4;
}

static void ei_inc_C(void) {
  u8 res;

  res=_inc8(cpus.r[rC]);
  cpus.r[rC]=res;

  z80_clock+=4;
}

static void ei_inc_D(void) {
  u8 res;

  res=_inc8(cpus.r[rD]);
  cpus.r[rD]=res;

  z80_clock+=4;
}

static void ei_inc_E(void) {
  u8 res;

  res=_inc8(cpus.r[rE]);
  cpus.r[rE]=res;

  z80_clock+=4;
}

static void ei_inc_H(void) {
  u8 res;

  res=_inc8(cpus.r[rH]);
  cpus.r[rH]=res;

  z80_clock+=4;
}

static void ei_inc_L(void) {
  u8 res;

  res=_inc8(cpus.r[rL]);
  cpus.r[rL]=res;

  z80_clock+=4;
}

static void ei_inc_iHL(void) {
  u8 res;

  res=_inc8(_iHL8());
  s_iHL8(res);

  z80_clock+=11;
}

static void ei_inc_iIXN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_inc8(_iIXN8(op));
  s_iIXN8(op,res);

  z80_clock+=19;
}

static void ei_inc_iIYN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_inc8(_iIYN8(op));
  s_iIYN8(op,res);

  z80_clock+=19;
}

static void ei_inc_BC(void) {

  setBC(getBC()+1);
  z80_clock+=6;
}

static void ei_inc_DE(void) {

  setDE(getDE()+1);
  z80_clock+=6;
}

static void ei_inc_HL(void) {

  setHL(getHL()+1);
  z80_clock+=6;
}

static void ei_inc_SP(void) {

  cpus.SP++;
  z80_clock+=6;
}

static void ei_inc_IX(void) {

  cpus.IX++;
  z80_clock+=6;
}

static void ei_inc_IY(void) {

  cpus.IY++;
  z80_clock+=6;
}


/************************************************************************/

static void ei_ind(void) {
  u8 res,tmp;

  tmp=_in8(getBC());
  s_iHL8(tmp);
  setHL(getHL()-1);
  res=(cpus.r[rB]-1)&0xff;
  
  setflags(res>>7,
           res==0,
	   (cpus.r[rB]&0x0f)-1<0, /* doufam */
	   res==127,
	   1,
	   -1);
  /* jeste nevim, jak funguji undoc flags */
  
  cpus.r[rB]=res;
  z80_clock+=16;
}

static void ei_indr(void) {
  u8 res;
  
  res=_in8(getBC());
  s_iHL8(res);
  setHL(getHL()-1);
  cpus.r[rB]--;
  
  if(cpus.r[rB]==0) {
//    printf("B==0. indr terminated.\n");
    setflags(0,1,0,0,1,-1);
    /* jeste nevim, jak funguji undoc flags */
    z80_clock+=16;
  } else {
    z80_clock+=21;
    cpus.PC-=2;
  }
}

static void ei_ini(void) {
  u8 res,tmp;

  tmp=_in8(getBC());
  s_iHL8(tmp);
  setHL(getHL()+1);
  res=(cpus.r[rB]-1)&0xff;
  
  setflags(res>>7,
           res==0,
	   (cpus.r[rB]&0x0f)-1<0, /* doufam */
	   res==127,
	   1,
	   -1);
  /* jeste nevim, jak funguji undoc flags */
  
  cpus.r[rB]=res;
  z80_clock+=16;
}

static void ei_inir(void) {
  u8 res;
  
  res=_in8(getBC());
  s_iHL8(res);
  setHL(getHL()+1);
  cpus.r[rB]--;
  
  if(cpus.r[rB]==0) {
//    printf("B==0. inir terminated.\n");
    setflags(0,1,0,0,1,-1);
    /* jeste nevim, jak funguji undoc flags */
    z80_clock+=16;
  } else {
    z80_clock+=21;
    cpus.PC-=2;
  }
}

/************************************************************************/

static void ei_jp_NN(void) {
  u16 addr;

  addr=z80_iget16();
 // printf("jp 0x%04x\n",addr);
  _jp16(addr);
  z80_clock+=10;
}

static void ei_jp_HL(void) {
  u16 addr;

  addr=getHL();
//  printf("%04x:jp HL [0x%04x]\n",cpus.PC,addr);
//  z80_printstatus();
  _jp16(addr);
  z80_clock+=4;
}

static void ei_jp_IX(void) {
  u16 addr;

  addr=cpus.IX;
  _jp16(addr);
  z80_clock+=4;
}

static void ei_jp_IY(void) {
  u16 addr;

  addr=cpus.IY;
  _jp16(addr);
  z80_clock+=4;
}

static void ei_jp_C_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(cpus.F & fC) {
    _jp16(addr);
  }
  z80_clock+=10;
}

static void ei_jp_NC_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(!(cpus.F & fC)) {
    _jp16(addr);
  }
  z80_clock+=10;
}

static void ei_jp_M_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(cpus.F & fS) {
    _jp16(addr);
  }
  z80_clock+=10;
}

static void ei_jp_P_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(!(cpus.F & fS)) {
    _jp16(addr);
  }
  z80_clock+=10;
}

static void ei_jp_Z_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(cpus.F & fZ) {
    _jp16(addr);
  }
  z80_clock+=10;
}

static void ei_jp_NZ_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(!(cpus.F & fZ)) {
    _jp16(addr);
  }
  z80_clock+=10;
}

static void ei_jp_PE_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(cpus.F & fPV) {
    _jp16(addr);
  }
  z80_clock+=10;
}

static void ei_jp_PO_NN(void) {
  u16 addr;

  addr=z80_iget16();
  if(!(cpus.F & fPV)) {
    _jp16(addr);
  }
  z80_clock+=10;
}

/************************************************************************/

static void ei_jr_N(void) {
  u8 ofs;

  ofs=z80_iget8();
  _jr8(ofs);
  z80_clock+=12;
}

static void ei_jr_C_N(void) {
  u8 ofs;

  ofs=z80_iget8();
  if(cpus.F & fC) {
    _jr8(ofs);
    z80_clock+=12;
  } else z80_clock+=7;
}

static void ei_jr_NC_N(void) {
  u8 ofs;

  ofs=z80_iget8();
  if(!(cpus.F & fC)) {
    _jr8(ofs);
    z80_clock+=12;
  } else z80_clock+=7;
}

static void ei_jr_Z_N(void) {
  u8 ofs;

  ofs=z80_iget8();
  if(cpus.F & fZ) {
    _jr8(ofs);
    z80_clock+=12;
  } else z80_clock+=7;
}

static void ei_jr_NZ_N(void) {
  u8 ofs;

  ofs=z80_iget8();
  if(!(cpus.F & fZ)) {
    _jr8(ofs);
    z80_clock+=12;
  } else z80_clock+=7;
}

/************************************************************************/

static void ei_ld_I_A(void) {

  cpus.I=cpus.r[rA];
  z80_clock+=9;
}

static void ei_ld_R_A(void) {

  cpus.R=cpus.r[rA];
  z80_clock+=9;
}

static void ei_ld_A_I(void) {

  cpus.r[rA]=cpus.I;
  setflags(cpus.r[rA]>>7,
	   cpus.r[rA]!=0,
	   0,
	   cpus.IFF2,
	   0,
	   -1);
  setundocflags8(cpus.r[rA]);
  z80_clock+=9;
}

static void ei_ld_A_R(void) {

  //cpus.r[rA]=cpus.R;
  cpus.r[rA] = 0x00;
  setflags(cpus.r[rA]>>7,
	   cpus.r[rA]!=0,
	   0,
	   cpus.IFF2,
	   0,
	   -1);
  setundocflags8(cpus.r[rA]);
  z80_clock+=9;
}

static void ei_ld_A_r(void) {

  cpus.r[rA]=cpus.r[opcode & 0x07];
  z80_clock+=4;
}

static void ei_ld_A_N(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rA]=op;
  z80_clock+=7;
}

static void ei_ld_A_iBC(void) {

  cpus.r[rA]=_iBC8();
  z80_clock+=7;
}

static void ei_ld_A_iDE(void) {

  cpus.r[rA]=_iDE8();
  z80_clock+=7;
}

static void ei_ld_A_iHL(void) {

  cpus.r[rA]=_iHL8();
  z80_clock+=7;
}

static void ei_ld_A_iIXN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rA]=_iIXN8(op);
  z80_clock+=15;
}

static void ei_ld_A_iIYN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rA]=_iIYN8(op);
  z80_clock+=15;
}

static void ei_ld_A_iNN(void) {
  u16 addr;

  addr=z80_iget16();
  cpus.r[rA]=zx_memget8(addr);
  z80_clock+=13;
}

static void ei_ld_B_r(void) {

  cpus.r[rB]=cpus.r[opcode & 0x07];
  z80_clock+=4;
}

static void ei_ld_B_N(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rB]=op;
  z80_clock+=7;
}

static void ei_ld_B_iHL(void) {

  cpus.r[rB]=_iHL8();
  z80_clock+=7;
}

static void ei_ld_B_iIXN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rB]=_iIXN8(op);
  z80_clock+=15;
}

static void ei_ld_B_iIYN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rB]=_iIYN8(op);
  z80_clock+=15;
}

static void ei_ld_C_r(void) {

  cpus.r[rC]=cpus.r[opcode & 0x07];
  z80_clock+=4;
}

static void ei_ld_C_N(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rC]=op;
  z80_clock+=7;
}

static void ei_ld_C_iHL(void) {

  cpus.r[rC]=_iHL8();
  z80_clock+=7;
}

static void ei_ld_C_iIXN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rC]=_iIXN8(op);
  z80_clock+=15;
}

static void ei_ld_C_iIYN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rC]=_iIYN8(op);
  z80_clock+=15;
}

static void ei_ld_D_r(void) {

  cpus.r[rD]=cpus.r[opcode & 0x07];
  z80_clock+=4;
}

static void ei_ld_D_N(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rD]=op;
  z80_clock+=7;
}

static void ei_ld_D_iHL(void) {

  cpus.r[rD]=_iHL8();
  z80_clock+=7;
}

static void ei_ld_D_iIXN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rD]=_iIXN8(op);
  z80_clock+=15;
}

static void ei_ld_D_iIYN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rD]=_iIYN8(op);
  z80_clock+=15;
}

static void ei_ld_E_r(void) {

  cpus.r[rE]=cpus.r[opcode & 0x07];
  z80_clock+=4;
}

static void ei_ld_E_N(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rE]=op;
  z80_clock+=7;
}

static void ei_ld_E_iHL(void) {

  cpus.r[rE]=_iHL8();
  z80_clock+=7;
}

static void ei_ld_E_iIXN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rE]=_iIXN8(op);
  z80_clock+=15;
}

static void ei_ld_E_iIYN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rE]=_iIYN8(op);
  z80_clock+=15;
}

static void ei_ld_H_r(void) {

  cpus.r[rH]=cpus.r[opcode & 0x07];
  z80_clock+=4;
}

static void ei_ld_H_N(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rH]=op;
  z80_clock+=7;
}

static void ei_ld_H_iHL(void) {

  cpus.r[rH]=_iHL8();
  z80_clock+=7;
}

static void ei_ld_H_iIXN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rH]=_iIXN8(op);
  z80_clock+=15;
}

static void ei_ld_H_iIYN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rH]=_iIYN8(op);
  z80_clock+=15;
}

static void ei_ld_L_r(void) {

  cpus.r[rL]=cpus.r[opcode & 0x07];
  z80_clock+=4;
}

static void ei_ld_L_N(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rL]=op;
  z80_clock+=7;
}

static void ei_ld_L_iHL(void) {

  cpus.r[rL]=_iHL8();
  z80_clock+=7;
}

static void ei_ld_L_iIXN(void) {
  u8 op;

  op=z80_iget8();
  cpus.r[rL]=_iIXN8(op);
  z80_clock+=15;
}

static void ei_ld_L_iIYN(void) {
  u8 op;

  op=z80_iget8();

  cpus.r[rL]=_iIYN8(op);

  z80_clock+=15;
}

static void ei_ld_BC_iNN(void) {
  u16 addr;

  addr=z80_iget16();
  setBC(zx_memget16(addr));
  z80_clock+=20;
}

static void ei_ld_BC_NN(void) {
  u16 data;

  data=z80_iget16();
  setBC(data);
  z80_clock+=10;
}

static void ei_ld_DE_iNN(void) {
  u16 addr;

  addr=z80_iget16();
  setDE(zx_memget16(addr));
  z80_clock+=20;
}

static void ei_ld_DE_NN(void) {
  u16 data;

  data=z80_iget16();
  setDE(data);
  z80_clock+=10;
}

static void ei_ld_HL_iNN(void) {
  u16 addr;

  addr=z80_iget16();
  setHL(zx_memget16(addr));
  z80_clock+=20;
}

static void ei_ld_HL_NN(void) {
  u16 data;

  data=z80_iget16();
  setHL(data);
  z80_clock+=10;
}

static void ei_ld_SP_iNN(void) {
  u16 addr;

  addr=z80_iget16();
  cpus.SP=zx_memget16(addr);
  z80_clock+=20;
}

static void ei_ld_SP_NN(void) {
  u16 data;

  data=z80_iget16();
  cpus.SP=data;
  z80_clock+=10;
}

static void ei_ld_SP_HL(void) {

  cpus.SP=getHL();
  z80_clock+=6;
}

static void ei_ld_SP_IX(void) {

  cpus.SP=cpus.IX;
  z80_clock+=6;
}

static void ei_ld_SP_IY(void) {

  cpus.SP=cpus.IY;
  z80_clock+=6;
}

static void ei_ld_IX_iNN(void) {
  u16 addr;

  addr=z80_iget16();
  cpus.IX=zx_memget16(addr);
  z80_clock+=16;
}

static void ei_ld_IX_NN(void) {
  u16 data;

  data=z80_iget16();
  cpus.IX=data;
  z80_clock+=10;
}

static void ei_ld_IY_iNN(void) {
  u16 addr;

  addr=z80_iget16();
  cpus.IY=zx_memget16(addr);
  z80_clock+=16;
}

static void ei_ld_IY_NN(void) {
  u16 data;

  data=z80_iget16();
  cpus.IY=data;
  z80_clock+=10;
}

static void ei_ld_iHL_r(void) {

  s_iHL8(cpus.r[opcode & 0x07]);
  z80_clock+=7;
}

static void ei_ld_iHL_N(void) {
  u8 op;

  op=z80_iget8();
  s_iHL8(op);
  z80_clock+=10;
}

static void ei_ld_iBC_A(void) {

  s_iBC8(cpus.r[rA]);
  z80_clock+=7;
}

static void ei_ld_iDE_A(void) {

  s_iDE8(cpus.r[rA]);
  z80_clock+=7;
}

static void ei_ld_iNN_A(void) {
  u16 addr;

  addr=z80_iget16();
  zx_memset8(addr,cpus.r[rA]);
  z80_clock+=13;
}

static void ei_ld_iNN_BC(void) {
  u16 addr;

  addr=z80_iget16();
  zx_memset16(addr,getBC());
  z80_clock+=20;
}

static void ei_ld_iNN_DE(void) {
  u16 addr;

  addr=z80_iget16();
  zx_memset16(addr,getDE());
  z80_clock+=20;
}

static void ei_ld_iNN_HL(void) {
  u16 addr;

  addr=z80_iget16();
  zx_memset16(addr,getHL());
  z80_clock+=16;
}

static void ei_ld_iNN_SP(void) {
  u16 addr;

  addr=z80_iget16();
  zx_memset16(addr,cpus.SP);
  z80_clock+=20;
}

static void ei_ld_iNN_IX(void) {
  u16 addr;

  addr=z80_iget16();
  zx_memset16(addr,cpus.IX);
  z80_clock+=16;
}

static void ei_ld_iNN_IY(void) {
  u16 addr;

  addr=z80_iget16();
  zx_memset16(addr,cpus.IY);
  z80_clock+=16;
}

static void ei_ld_iIXN_r(void) {
  u8 op;

  op=z80_iget8();
  s_iIXN8(op,cpus.r[opcode & 0x07]);
  z80_clock+=15;
}

static void ei_ld_iIXN_N(void) {
  u8 op,data;

  op=z80_iget8();
  data=z80_iget8();
  s_iIXN8(op,data);
  z80_clock+=15;
}

static void ei_ld_iIYN_r(void) {
  u8 op;

  op=z80_iget8();
  s_iIYN8(op,cpus.r[opcode & 0x07]);
  z80_clock+=15;
}

static void ei_ld_iIYN_N(void) {
  u8 op,data;

  op=z80_iget8();
  data=z80_iget8();

  s_iIYN8(op,data);

  z80_clock+=15;
}


/************************************************************************/

static void ei_ldd(void) {
  u8 res,ufr;
  u16 newBC;

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

  z80_clock+=16;
}

static void ei_lddr(void) {
  u8 res,ufr;
  u16 newBC;

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
    z80_clock+=16;
  } else {
    z80_clock+=21;
    cpus.PC-=2;
  }
}


static void ei_ldi(void) {
  u8 res,ufr;
  u16 newBC;

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

  z80_clock+=16;
}

static void ei_ldir(void) {
  u8 res,ufr;
  u16 newBC;

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
    z80_clock+=16;
  } else {
    z80_clock+=21;
    cpus.PC-=2;
  }
}


/************************************************************************/

static void ei_neg(void) {               /* A <- neg(A) .. two's complement */
  u16 res;
//  printf("NEG(2c)\n");
  res = (cpus.r[rA] ^ 0xff)+1;
  
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (res&0x0f)==0,        /* nejsem si jisty, overit !!!!! */
	   (res&0xff)==0x80,     /* 127 -> -128 */
	   1,
	   res>0xff);		 /* nejsem si jisty, overit !!!!! */

  cpus.r[rA] = res & 0xff;
  setundocflags8(cpus.r[rA]);
  z80_clock+=8;
}


/************************************************************************/

static void ei_nop(void) {
  z80_clock+=4;
}

/************************************************************************/

static void ei_or_r(void) {
  u8 res;

  res=_or8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_or_N(void) {
  u8 res,op;

  op=z80_iget8();

  res=_or8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_or_iHL(void) {
  u8 res;

  res=_or8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_or_iIXN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_or8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

static void ei_or_iIYN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_or8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

/************************************************************************/

static void ei_out_iN_A(void) {
  u16 op;

  op=z80_iget8();

  _out8(((u16)cpus.r[rA]<<8)|(u16)op,cpus.r[rA]);

  z80_clock+=11;
}

static void Ui_out_iC_0(void) {

  printf("ei_out_iC_0 (unsupported)\n");
  _out8(getBC(),0);

  uoc++;
  z80_clock+=12;
}

static void ei_out_iC_A(void) {

  _out8(getBC(),cpus.r[rA]);
  z80_clock+=12;
}

static void ei_out_iC_B(void) {

  _out8(getBC(),cpus.r[rB]);
  z80_clock+=12;
}

static void ei_out_iC_C(void) {

  _out8(getBC(),cpus.r[rC]);
  z80_clock+=12;
}

static void ei_out_iC_D(void) {

  _out8(getBC(),cpus.r[rD]);
  z80_clock+=12;
}

static void ei_out_iC_E(void) {

  _out8(getBC(),cpus.r[rE]);
  z80_clock+=12;
}

static void ei_out_iC_H(void) {

  _out8(getBC(),cpus.r[rH]);
  z80_clock+=12;
}

static void ei_out_iC_L(void) {

  _out8(getBC(),cpus.r[rL]);
  z80_clock+=12;
}

/************************************************************************/

static void ei_outd(void) {
  u8 res;

  _out8(getBC(),_iHL8());
  setHL(getHL()-1);
  res=(cpus.r[rB]-1)&0x0f;
  
  setflags(res>>7,
           res==0,
	   (cpus.r[rB]&0x0f)+1>0x0f,
	   res==127,
	   1,
	   -1);
  /* undoc flags not affected */
	   
  cpus.r[rB]=res;
  z80_clock+=16;
}

static void ei_otdr(void) {
  _out8(getBC(),_iHL8());
  setHL(getHL()-1);
  cpus.r[rB]--;
  
  if(cpus.r[rB]==0) {
//    printf("B==0. otdr terminated.\n");
    setflags(0,1,0,0,1,-1);
    /* undoc flags not affected */
    z80_clock+=1;
  } else {
    z80_clock+=21;
    cpus.PC-=2;
  }
}

static void ei_outi(void) {
  u8 res;

  _out8(getBC(),_iHL8());
  setHL(getHL()+1);
  res=(cpus.r[rB]-1)&0xff;
  
  setflags(res>>7,
           res==0,
	   (cpus.r[rB]&0x0f)-1<0, /* doufam */
	   res==127,
	   1,
	   -1);
  /* undoc flags not affected */
  
  cpus.r[rB]=res;
  z80_clock+=16;
}

static void ei_otir(void) {
  _out8(getBC(),_iHL8());
  setHL(getHL()+1);
  cpus.r[rB]--;
  
  printf("otir step\n");
  if(cpus.r[rB]==0) {
//    printf("B==0. otir terminated.\n");
    setflags(0,1,0,0,1,-1);
    /* undoc flags not affected */
    z80_clock+=1;
  } else {
    z80_clock+=21;
    cpus.PC-=2;
  }
}


/************************************************************************/

static void ei_pop_AF(void) {
  setAF(_pop16());
  z80_clock+=10;
}

static void ei_pop_BC(void) {
  setBC(_pop16());
  z80_clock+=10;
}

static void ei_pop_DE(void) {
  setDE(_pop16());
  z80_clock+=10;
}

static void ei_pop_HL(void) {
  setHL(_pop16());
  z80_clock+=10;
}

static void ei_pop_IX(void) {
  cpus.IX=_pop16();
  z80_clock+=10;
}

static void ei_pop_IY(void) {
  cpus.IY=_pop16();
  z80_clock+=10;
}

static void ei_push_AF(void) {
  _push16(getAF());
  z80_clock+=11;
}

static void ei_push_BC(void) {
  _push16(getBC());
  z80_clock+=11;
}

static void ei_push_DE(void) {
  _push16(getDE());
  z80_clock+=11;
}

static void ei_push_HL(void) {
  _push16(getHL());
  z80_clock+=11;
}

static void ei_push_IX(void) {
  _push16(cpus.IX);
  z80_clock+=11;
}

static void ei_push_IY(void) {
  _push16(cpus.IY);
  z80_clock+=11;
}

/************************************************************************/

static void ei_res_b_r(void) {
  u8 res;

  res=_res8((opcode>>3)&0x07,cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=8;
}

static void ei_res_b_iHL(void) {
  u8 res;

  res=_res8((opcode>>3)&0x07,_iHL8());
  s_iHL8(res);

  z80_clock+=15;
}

/* DDCB ! */
static void ei_res_b_iIXN(void) {
  u8 res;

  res=_res8((opcode>>3)&0x07,_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock+=19;
}

/* FDCB ! */
static void ei_res_b_iIYN(void) {
   u8 res;

  res=_res8((opcode>>3)&0x07,_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock+=19;
}

/************************************************************************/

static void ei_ret(void) {
  cpus.PC=_pop16();
  z80_clock+=10;
}

static void ei_ret_C(void) {
  if(cpus.F & fC) {
    cpus.PC=_pop16();
    z80_clock+=11;
  } else z80_clock+=5;
}

static void ei_ret_NC(void) {
  if(!(cpus.F & fC)) {
    cpus.PC=_pop16();
    z80_clock+=11;
  } else z80_clock+=5;
}

static void ei_ret_M(void) {
  if(cpus.F & fS) {
    cpus.PC=_pop16();
    z80_clock+=11;
  } else z80_clock+=5;
}

static void ei_ret_P(void) {
  if(!(cpus.F & fS)) {
    cpus.PC=_pop16();
    z80_clock+=11;
  } else z80_clock+=5;
}


static void ei_ret_Z(void) {
  if(cpus.F & fZ) {
    cpus.PC=_pop16();
    z80_clock+=11;
  } else z80_clock+=5;
}

static void ei_ret_NZ(void) {
  if(!(cpus.F & fZ)) {
    cpus.PC=_pop16();
    z80_clock+=11;
  } else z80_clock+=5;
}

static void ei_ret_PE(void) {
  if(cpus.F & fPV) {
    cpus.PC=_pop16();
    z80_clock+=11;
  } else z80_clock+=5;
}

static void ei_ret_PO(void) {
  if(!(cpus.F & fPV)) {
    cpus.PC=_pop16();
    z80_clock+=11;
  } else z80_clock+=5;
}

/************************************************************************/

static void ei_reti(void) {
  cpus.IFF1=cpus.IFF2;
  cpus.PC=_pop16();
  z80_clock+=14;
}


static void ei_retn(void) {
  cpus.IFF1=cpus.IFF2;
  cpus.PC=_pop16();
  z80_clock+=14;
}

/************************************************************************/

static void ei_rla(void) {
  u8 res;

  res=_rla8(cpus.r[rA]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_rl_r(void) {
  u8 res;

  res=_rl8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=8;
}

static void ei_rl_iHL(void) {
  u8 res;

  res=_rl8(_iHL8());
  s_iHL8(res);

  z80_clock+=15;
}

/* DDCB */
static void ei_rl_iIXN(void) {
  u8 res;

  res=_rl8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock+=19;
}

/* FDCB */
static void ei_rl_iIYN(void) {
  u8 res;

  res=_rl8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock+=19;
}

static void ei_rlca(void) {
  u8 res;

  res=_rlca8(cpus.r[rA]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_rlc_r(void) {
  u8 res;

  res=_rlc8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=8;
}

static void ei_rlc_iHL(void) {
  u8 res;

  res=_rlc8(_iHL8());
  s_iHL8(res);

  z80_clock+=15;
}

/* DDCB */
static void ei_rlc_iIXN(void) {
  u8 res;

  res=_rlc8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock+=19;
}

/* FDCB */
static void ei_rlc_iIYN(void) {
  u8 res;

  res=_rlc8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock+=19;
}

static void ei_rld(void) {
  u8 tmp,tmp2,tmp3;

  tmp=cpus.r[rA] & 0x0f;
  tmp2=_iHL8();
  tmp3=tmp2>>4;
  tmp2=(tmp2<<4)|tmp;
  cpus.r[rA]=(cpus.r[rA] & 0xf0)| tmp3;
  s_iHL8(tmp2);
  
  cpus.F=(cpus.F&fC)|ox_tab[cpus.r[rA]];

  z80_clock+=18;
}

static void ei_rra(void) {
  u8 res;

  res=_rra8(cpus.r[rA]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_rr_r(void) {
  u8 res;

  res=_rr8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=8;
}

static void ei_rr_iHL(void) {
  u8 res;

  res=_rr8(_iHL8());
  s_iHL8(res);

  z80_clock+=15;
}

/* DDCB */
static void ei_rr_iIXN(void) {
  u8 res;

  res=_rr8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock+=19;
}

/* FDCB */
static void ei_rr_iIYN(void) {
  u8 res;

  res=_rr8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock+=19;
}

static void ei_rrca(void) {
  u8 res;

  res=_rrca8(cpus.r[rA]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_rrc_r(void) {
  u8 res;

  res=_rrc8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=8;
}

static void ei_rrc_iHL(void) {
  u8 res;

  res=_rrc8(_iHL8());
  s_iHL8(res);

  z80_clock+=15;
}

/* DDCB */
static void ei_rrc_iIXN(void) {
  u8 res;

  res=_rrc8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock+=19;
}

/* FDCB */
static void ei_rrc_iIYN(void) {
  u8 res;

  res=_rrc8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock+=19;
}

static void ei_rrd(void) {
  u8 tmp,tmp2,tmp3;

  tmp=cpus.r[rA] & 0x0f;
  tmp2=_iHL8();
  tmp3=tmp2 & 0x0f;
  tmp2=(tmp2>>4)|(tmp<<4);
  cpus.r[rA]=(cpus.r[rA] & 0xf0)| tmp3;
  s_iHL8(tmp2);
  
  cpus.F=(cpus.F&fC)|ox_tab[cpus.r[rA]];

  z80_clock+=18;
}

/************************************************************************/

static void ei_rst_0(void) {
  _call16(0x0000);
  z80_clock+=11;
}

static void ei_rst_8(void) {
  z80_printstatus();
  _call16(0x0008);
  z80_clock+=11;
}

static void ei_rst_10(void) {
  _call16(0x0010);
  z80_clock+=11;
}

static void ei_rst_18(void) {
  _call16(0x0018);
  z80_clock+=11;
}

static void ei_rst_20(void) {
  _call16(0x0020);
  z80_clock+=11;
}

static void ei_rst_28(void) {
  _call16(0x0028);
  z80_clock+=11;
}

static void ei_rst_30(void) {
  _call16(0x0030);
  z80_clock+=11;
}

static void ei_rst_38(void) {
  _call16(0x0038);
  z80_clock+=11;
}

/************************************************************************/

static void ei_sbc_A_r(void) {
  u8 res;

  res=_sbc8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_sbc_A_N(void) {
  u8 res,op;

  op=z80_iget8();

  res=_sbc8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_sbc_A_iHL(void) {
  u8 res;

  res=_sbc8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_sbc_A_iIXN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_sbc8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

static void ei_sbc_A_iIYN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_sbc8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

static void ei_sbc_HL_BC(void) {
  u16 res;

  res=_sbc16(getHL(),getBC());
  setHL(res);

  z80_clock+=15;
}

static void ei_sbc_HL_DE(void) {
  u16 res;

  res=_sbc16(getHL(),getDE());
  setHL(res);

  z80_clock+=15;
}

static void ei_sbc_HL_HL(void) {
  u16 res;

  res=_sbc16(getHL(),getHL());
  setHL(res);

  z80_clock+=15;
}

static void ei_sbc_HL_SP(void) {
  u16 res;

  res=_sbc16(getHL(),cpus.SP);
  setHL(res);

  z80_clock+=15;
}


/************************************************************************/

static void ei_scf(void) {
  setflags(-1,-1,0,-1,0,1);
  setundocflags8(cpus.r[rA]);
  z80_clock+=4;
}

/************************************************************************/

static void ei_set_b_r(void) {
  u8 res;

  res=_set8((opcode>>3)&0x07,cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=8;
}

static void ei_set_b_iHL(void) {
  u8 res;

  res=_set8((opcode>>3)&0x07,_iHL8());
  s_iHL8(res);

  z80_clock+=15;
}

/* DDCB ! */
static void ei_set_b_iIXN(void) {
  u8 res;

  res=_set8((opcode>>3)&0x07,_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock+=19;
}

/* FDCB ! */
static void ei_set_b_iIYN(void) {
  u8 res;

  res=_set8((opcode>>3)&0x07,_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock+=19;
}

/************************************************************************/

static void ei_sla_r(void) {
  u8 res;

  res=_sla8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=8;
}

static void ei_sla_iHL(void) {
  u8 res;

  res=_sla8(_iHL8());
  s_iHL8(res);

  z80_clock+=15;
}

/* DDCB */
static void ei_sla_iIXN(void) {
  u8 res;

  res=_sla8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock+=19;
}

/* FDCB */
static void ei_sla_iIYN(void) {
  u8 res;

  res=_sla8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock+=19;
}

static void ei_sra_r(void) {
  u8 res;

  res=_sra8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=8;
}

static void ei_sra_iHL(void) {
  u8 res;

  res=_sra8(_iHL8());
  s_iHL8(res);

  z80_clock+=15;
}

/* DDCB */
static void ei_sra_iIXN(void) {
  u8 res;

  res=_sra8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock+=19;
}

/* FDCB */
static void ei_sra_iIYN(void) {
  u8 res;

  res=_sra8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock+=19;
}

/************************************************************************/

static void Ui_sll_r(void) {
  u8 res;

  res=_sll8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  uoc++;
  z80_clock+=8;
}

static void Ui_sll_iHL(void) {
  u8 res;

  res=_sll8(_iHL8());
  s_iHL8(res);

  uoc++;
  z80_clock+=15;
}

/* DDCB */
static void Ui_sll_iIXN(void) {
  u8 res;

  res=_sll8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  uoc++;
  z80_clock+=19;
}

/* FDCB */
static void Ui_sll_iIYN(void) {
  u8 res;

  res=_sll8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  uoc++;
  z80_clock+=19;
}

static void ei_srl_r(void) {
  u8 res;

  res=_srl8(cpus.r[opcode & 0x07]);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=8;
}

static void ei_srl_iHL(void) {
  u8 res;

  res=_srl8(_iHL8());
  s_iHL8(res);

  z80_clock+=15;
}

/* DDCB */
static void ei_srl_iIXN(void) {
  u8 res;

  res=_srl8(_iIXN8(cbop));
  s_iIXN8(cbop,res);

  z80_clock+=19;
}

/* FDCB */
static void ei_srl_iIYN(void) {
  u8 res;

  res=_srl8(_iIYN8(cbop));
  s_iIYN8(cbop,res);

  z80_clock+=19;
}

/************************************************************************/

static void ei_sub_r(void) {
  u8 res;

  res=_sub8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_sub_N(void) {
  u8 res,op;

  op=z80_iget8();

  res=_sub8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_sub_iHL(void) {
  u8 res;

  res=_sub8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_sub_iIXN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_sub8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

static void ei_sub_iIYN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_sub8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}


/************************************************************************/

static void ei_xor_r(void) {
  u8 res;

  res=_xor8(cpus.r[rA],cpus.r[opcode & 0x07]);
  cpus.r[rA]=res;

  z80_clock+=4;
}

static void ei_xor_N(void) {
  u8 res,op;

  op=z80_iget8();

  res=_xor8(cpus.r[rA],op);
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_xor_iHL(void) {
  u8 res;

  res=_xor8(cpus.r[rA],_iHL8());
  cpus.r[rA]=res;

  z80_clock+=7;
}

static void ei_xor_iIXN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_xor8(cpus.r[rA],_iIXN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

static void ei_xor_iIYN(void) {
  u8 res,op;

  op=z80_iget8();

  res=_xor8(cpus.r[rA],_iIYN8(op));
  cpus.r[rA]=res;

  z80_clock+=15;
}

/************************ undocumented opcodes ****************************/
/**** DD .. ***************************************************************/

static void Ui_inc_IXh(void) {
  u8 res;

  res=_inc8(getIXh());
  setIXh(res);

  z80_clock+=4; /* timing&flags taken from inc_A */
  uoc++;
}

static void Ui_dec_IXh(void) {
  u8 res;

  res=_dec8(getIXh());
  setIXh(res);

  z80_clock+=4; /* timing&flags taken from dec_A */
  uoc++;
}

static void Ui_ld_IXh_N(void) {
  u8 res;

  res=z80_iget8();
  setIXh(res);

  z80_clock+=7; /* timing&flags taken from ld_A_N */
  uoc++;
}

static void Ui_inc_IXl(void) {
  u8 res;

  res=_inc8(getIXl());
  setIXl(res);

  z80_clock+=4; /* timing&flags taken from inc_A */
  uoc++;
}

static void Ui_dec_IXl(void) {
  u8 res;

  res=_dec8(getIXl());
  setIXl(res);

  z80_clock+=4; /* timing&flags taken from dec_A */
  uoc++;
}

static void Ui_ld_IXl_N(void) {
  u8 res;

  res=z80_iget8();
  setIXl(res);

  z80_clock+=7; /* timing&flags taken from ld_A_N */
  uoc++;
}

static void Ui_ld_B_IXh(void) {

  cpus.r[rB]=getIXh();

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_B_IXl(void) {

  cpus.r[rB]=getIXl();

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_C_IXh(void) {

  cpus.r[rC]=getIXh();

  z80_clock+=4; /* timing&flags taken from ld_C_r */
  uoc++;
}

static void Ui_ld_C_IXl(void) {

  cpus.r[rC]=getIXl();

  z80_clock+=4; /* timing&flags taken from ld_C_r */
  uoc++;
}

static void Ui_ld_D_IXh(void) {

  cpus.r[rD]=getIXh();

  z80_clock+=4; /* timing&flags taken from ld_D_r */
  uoc++;
}

static void Ui_ld_D_IXl(void) {

  cpus.r[rD]=getIXl();

  z80_clock+=4; /* timing&flags taken from ld_D_r */
  uoc++;
}

static void Ui_ld_E_IXh(void) {

  cpus.r[rE]=getIXh();

  z80_clock+=4; /* timing&flags taken from ld_E_r */
  uoc++;
}

static void Ui_ld_E_IXl(void) {

  cpus.r[rE]=getIXl();

  z80_clock+=4; /* timing&flags taken from ld_E_r */
  uoc++;
}

static void Ui_ld_IXh_B(void) {

  setIXh(cpus.r[rB]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXh_C(void) {

  setIXh(cpus.r[rC]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXh_D(void) {

  setIXh(cpus.r[rD]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXh_E(void) {

  setIXh(cpus.r[rE]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXh_IXh(void) {

  /* does nothing */

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXh_IXl(void) {

  setIXh(getIXl());

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXh_A(void) {

  setIXh(cpus.r[rA]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_B(void) {

  setIXl(cpus.r[rB]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_C(void) {

  setIXl(cpus.r[rC]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_D(void) {

  setIXl(cpus.r[rD]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_E(void) {

  setIXl(cpus.r[rE]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_IXh(void) {

  setIXl(getIXh());

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_IXl(void) {

  /* does nothing */

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IXl_A(void) {

  setIXl(cpus.r[rA]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_A_IXh(void) {

  cpus.r[rA]=getIXh();

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_A_IXl(void) {

  cpus.r[rA]=getIXl();

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

/***********************************************************************/

static void Ui_add_A_IXh(void) {
  u8 res;

  res=_add8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from add_A_r */
  uoc++;
}

static void Ui_add_A_IXl(void) {
  u8 res;

  res=_add8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from add_A_r */
  uoc++;
}

static void Ui_adc_A_IXh(void) {
  u8 res;

  res=_adc8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from adc_A_r */
  uoc++;
}

static void Ui_adc_A_IXl(void) {
  u8 res;

  res=_adc8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from adc_A_r */
  uoc++;
}

static void Ui_sub_IXh(void) {
  u8 res;

  res=_sub8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from sub_r */
  uoc++;
}

static void Ui_sub_IXl(void) {
  u8 res;

  res=_sub8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from sub_r */
  uoc++;
}

static void Ui_sbc_IXh(void) {
  u8 res;

  res=_sbc8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from sbc_r */
  uoc++;
}

static void Ui_sbc_IXl(void) {
  u8 res;

  res=_sbc8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from sbc_r */
  uoc++;
}

static void Ui_and_IXh(void) {
  u8 res;

  res=_and8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from and_r */
  uoc++;
}

static void Ui_and_IXl(void) {
  u8 res;

  res=_and8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from and_r */
  uoc++;
}

static void Ui_xor_IXh(void) {
  u8 res;

  res=_xor8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from xor_r */
  uoc++;
}

static void Ui_xor_IXl(void) {
  u8 res;

  res=_xor8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from xor_r */
  uoc++;
}

static void Ui_or_IXh(void) {
  u8 res;

  res=_or8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from or_r */
  uoc++;
}

static void Ui_or_IXl(void) {
  u8 res;

  res=_or8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from or_r */
  uoc++;
}

static void Ui_cp_IXh(void) {
  u8 res;

  res=_cp8(cpus.r[rA],getIXh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from cp_r */
  uoc++;
}

static void Ui_cp_IXl(void) {
  u8 res;

  res=_cp8(cpus.r[rA],getIXl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from cp_r */
  uoc++;
}

/**** FD .. ***************************************************************/

static void Ui_inc_IYh(void) {
  u8 res;

  res=_inc8(getIYh());
  setIYh(res);

  z80_clock+=4; /* timing&flags taken from inc_A */
  uoc++;
}

static void Ui_dec_IYh(void) {
  u8 res;

  res=_dec8(getIYh());
  setIYh(res);

  z80_clock+=4; /* timing&flags taken from dec_A */
  uoc++;
}

static void Ui_ld_IYh_N(void) {
  u8 res;

  res=z80_iget8();
  setIYh(res);

  z80_clock+=7; /* timing&flags taken from ld_A_N */
  uoc++;
}

static void Ui_inc_IYl(void) {
  u8 res;

  res=_inc8(getIYl());
  setIYl(res);

  z80_clock+=4; /* timing&flags taken from inc_A */
  uoc++;
}

static void Ui_dec_IYl(void) {
  u8 res;

  res=_dec8(getIYl());
  setIYl(res);

  z80_clock+=4; /* timing&flags taken from dec_A */
  uoc++;
}

static void Ui_ld_IYl_N(void) {
  u8 res;

  res=z80_iget8();
  setIYl(res);

  z80_clock+=7; /* timing&flags taken from ld_A_N */
  uoc++;
}

static void Ui_ld_B_IYh(void) {

  cpus.r[rB]=getIYh();

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_B_IYl(void) {

  cpus.r[rB]=getIYl();

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_C_IYh(void) {

  cpus.r[rC]=getIYh();

  z80_clock+=4; /* timing&flags taken from ld_C_r */
  uoc++;
}

static void Ui_ld_C_IYl(void) {

  cpus.r[rC]=getIYl();

  z80_clock+=4; /* timing&flags taken from ld_C_r */
  uoc++;
}

static void Ui_ld_D_IYh(void) {

  cpus.r[rD]=getIYh();

  z80_clock+=4; /* timing&flags taken from ld_D_r */
  uoc++;
}

static void Ui_ld_D_IYl(void) {

  cpus.r[rD]=getIYl();

  z80_clock+=4; /* timing&flags taken from ld_D_r */
  uoc++;
}

static void Ui_ld_E_IYh(void) {

  cpus.r[rE]=getIYh();

  z80_clock+=4; /* timing&flags taken from ld_E_r */
  uoc++;
}

static void Ui_ld_E_IYl(void) {

  cpus.r[rE]=getIYl();

  z80_clock+=4; /* timing&flags taken from ld_E_r */
  uoc++;
}

static void Ui_ld_IYh_B(void) {

  setIYh(cpus.r[rB]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYh_C(void) {

  setIYh(cpus.r[rC]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYh_D(void) {

  setIYh(cpus.r[rD]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYh_E(void) {

  setIYh(cpus.r[rE]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYh_IYh(void) {

  /* does nothing */

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYh_IYl(void) {

  setIYh(getIYl());

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYh_A(void) {

  setIYh(cpus.r[rA]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_B(void) {

  setIYl(cpus.r[rB]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_C(void) {

  setIYl(cpus.r[rC]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_D(void) {

  setIYl(cpus.r[rD]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_E(void) {

  setIYl(cpus.r[rE]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_IYh(void) {

  setIYl(getIYh());

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_IYl(void) {

  /* does nothing */

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_IYl_A(void) {

  setIYl(cpus.r[rA]);

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_A_IYh(void) {

  cpus.r[rA]=getIYh();

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

static void Ui_ld_A_IYl(void) {

  cpus.r[rA]=getIYl();

  z80_clock+=4; /* timing&flags taken from ld_B_r */
  uoc++;
}

/***********************************************************************/

static void Ui_add_A_IYh(void) {
  u8 res;

  res=_add8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from add_A_r */
  uoc++;
}

static void Ui_add_A_IYl(void) {
  u8 res;

  res=_add8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from add_A_r */
  uoc++;
}

static void Ui_adc_A_IYh(void) {
  u8 res;

  res=_adc8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from adc_A_r */
  uoc++;
}

static void Ui_adc_A_IYl(void) {
  u8 res;

  res=_adc8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from adc_A_r */
  uoc++;
}

static void Ui_sub_IYh(void) {
  u8 res;

  res=_sub8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from sub_r */
  uoc++;
}

static void Ui_sub_IYl(void) {
  u8 res;

  res=_sub8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from sub_r */
  uoc++;
}

static void Ui_sbc_IYh(void) {
  u8 res;

  res=_sbc8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from sbc_r */
  uoc++;
}

static void Ui_sbc_IYl(void) {
  u8 res;

  res=_sbc8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from sbc_r */
  uoc++;
}

static void Ui_and_IYh(void) {
  u8 res;

  res=_and8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from and_r */
  uoc++;
}

static void Ui_and_IYl(void) {
  u8 res;

  res=_and8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from and_r */
  uoc++;
}

static void Ui_xor_IYh(void) {
  u8 res;

  res=_xor8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from xor_r */
  uoc++;
}

static void Ui_xor_IYl(void) {
  u8 res;

  res=_xor8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from xor_r */
  uoc++;
}

static void Ui_or_IYh(void) {
  u8 res;

  res=_or8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from or_r */
  uoc++;
}

static void Ui_or_IYl(void) {
  u8 res;

  res=_or8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from or_r */
  uoc++;
}

static void Ui_cp_IYh(void) {
  u8 res;

  res=_cp8(cpus.r[rA],getIYh());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from cp_r */
  uoc++;
}

static void Ui_cp_IYl(void) {
  u8 res;

  res=_cp8(cpus.r[rA],getIYl());
  cpus.r[rA]=res;

  z80_clock+=4; /* timing&flags taken from cp_r */
  uoc++;
}

/**** ED .. ***************************************************************/

static void Ui_ednop(void) { /* different from ei_nop! */
  printf("EDNOP!\n");
  z80_clock+=8; /* dle Seana jako 2 NOPy */
  uoc++;
}

static void Ui_neg(void) {               /* A <- neg(A) .. two's complement */
  u16 res;
//  printf("NEG(2c)\n");
  res = (cpus.r[rA] ^ 0xff)+1;
  cpus.r[rA] = res & 0xff;
  setflags((res>>7)&1,
	   (res&0xff)==0,
	   (res&0x0f)==0,        /* nejsem si jisty, overit !!!!! */
	   -2,
	   1,
	   res>0xff);		 /* nejsem si jisty, overit !!!!! */

  cpus.r[rA] = res & 0xff;
  z80_clock+=8; /* timing taken from ei_neg */
  uoc++;
}

static void Ui_im_0(void) { /* probably ..*/
  printf("IM 0\n");
  cpus.int_mode=0;
  z80_clock+=8; /* timing taken from ei_im_0 */
  uoc++;
}

static void Ui_im_1(void) { /* probably ..*/
  printf("IM 1\n");
  cpus.int_mode=1;
  z80_clock+=8; /* timing taken from ei_im_1 */
  uoc++;
}

static void Ui_im_2(void) { /* probably ..*/
  printf("IM 2\n");
  cpus.int_mode=2;
  z80_clock+=8; /* timing taken from ei_im_2 */
  uoc++;
}

static void Ui_reti(void) { /* undoc RETI behaves like RETN actually.. */
  cpus.IFF1=cpus.IFF2;
  cpus.PC=_pop16();
  z80_clock+=14; /* timing taken from ei_reti */
  uoc++;
}

static void Ui_retn(void) {
  cpus.IFF1=cpus.IFF2;
  cpus.PC=_pop16();
  z80_clock+=14;  /* timing taken from ei_retn */
  uoc++;
}

/**** DD CB dd .. ***************************************************************/

static void Ui_ld_r_rlc_iIXN(void) {
  u8 res;

  res=_rlc8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_rlc_iIXN */
  uoc++;
}

static void Ui_ld_r_rrc_iIXN(void) {
  u8 res;

  res=_rrc8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_rrc_iIXN */
  uoc++;
}

static void Ui_ld_r_rl_iIXN(void) {
  u8 res;

  res=_rl8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_rl_iIXN */
  uoc++;
}

static void Ui_ld_r_rr_iIXN(void) {
  u8 res;

  res=_rr8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_rr_iIXN */
  uoc++;
}

static void Ui_ld_r_sla_iIXN(void) {
  u8 res;

  res=_sla8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_sla_iIXN */
  uoc++;
}

static void Ui_ld_r_sra_iIXN(void) {
  u8 res;

  res=_sra8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_sra_iIXN */
  uoc++;
}

static void Ui_ld_r_sll_iIXN(void) {
  u8 res;

  res=_sll8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_sll_iIXN */
  uoc++;
}

static void Ui_ld_r_srl_iIXN(void) {
  u8 res;

  res=_srl8(_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_srl_iIXN */
  uoc++;
}

/************************************************************************/

static void Ui_bit_b_iIXN(void) {

  _bit8((opcode>>3)&0x07,_iIXN8(cbop));
  
  setundocflags8((cpus.IX+u8sval(cbop))>>8); /* weird, huh? */

  z80_clock+=16; /* timing&flags taken from ei_bit_b_iIXN */
  uoc++;
}

static void Ui_ld_r_res_b_iIXN(void) {
  u8 res;

  res=_res8((opcode>>3)&0x07,_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_res_b_iIXN */
  uoc++;
}

static void Ui_ld_r_set_b_iIXN(void) {
  u8 res;

  res=_set8((opcode>>3)&0x07,_iIXN8(cbop));
  s_iIXN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_set_b_iIXN */
  uoc++;
}

/**** FD CB dd .. ***************************************************************/

static void Ui_ld_r_rlc_iIYN(void) {
  u8 res;

  res=_rlc8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_rlc_iIYN */
  uoc++;
}

static void Ui_ld_r_rrc_iIYN(void) {
  u8 res;

  res=_rrc8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_rrc_iIYN */
  uoc++;
}

static void Ui_ld_r_rl_iIYN(void) {
  u8 res;

  res=_rl8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_rl_iIYN */
  uoc++;
}

static void Ui_ld_r_rr_iIYN(void) {
  u8 res;

  res=_rr8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_rr_iIYN */
  uoc++;
}

static void Ui_ld_r_sla_iIYN(void) {
  u8 res;

  res=_sla8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_sla_iIYN */
  uoc++;
}

static void Ui_ld_r_sra_iIYN(void) {
  u8 res;

  res=_sra8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_sra_iIYN */
  uoc++;
}

static void Ui_ld_r_sll_iIYN(void) {
  u8 res;

  res=_sll8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_sll_iIYN */
  uoc++;
}

static void Ui_ld_r_srl_iIYN(void) {
  u8 res;

  res=_srl8(_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_srl_iIYN */
  uoc++;
}

/************************************************************************/

static void Ui_bit_b_iIYN(void) {

  _bit8((opcode>>3)&0x07,_iIYN8(cbop));
  setundocflags8((cpus.IY+u8sval(cbop))>>8); /* weird, huh? */

  z80_clock+=16; /* timing&flags taken from ei_bit_b_iIYN */
  uoc++;
}

static void Ui_ld_r_res_b_iIYN(void) {
  u8 res;

  res=_res8((opcode>>3)&0x07,_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_res_b_iIYN */
  uoc++;
}

static void Ui_ld_r_set_b_iIYN(void) {
  u8 res;

  res=_set8((opcode>>3)&0x07,_iIYN8(cbop));
  s_iIYN8(cbop,res);
  cpus.r[opcode & 0x07]=res;

  z80_clock+=19; /* timing&flags taken from ei_set_b_iIYN */
  uoc++;
}


/************************************************************************/
/************************************************************************/

static void Si_stray(void);
static void Mi_dd(void);
static void Mi_fd(void);

#include "z80itab.c"

static void (**(ei_opm  [3]))(void) = { ei_op,   ei_ddop,   ei_fdop   };
static void (**(ei_cbopm[3]))(void) = { ei_cbop, ei_ddcbop, ei_fdcbop };

static void Mi_dd(void) {
  z80_clock+=4;  /* according to Sean: 4T(as NOP), 1R */
  cpus.int_lock=1; /* no interrupts! */
  cpus.modifier=1; /* DD */
}

static void Mi_fd(void) {
  z80_clock+=4;  /* according to Sean: 4T(as NOP), 1R */
  cpus.int_lock=1; /* no interrupts! */
  cpus.modifier=2; /* FD */
}

static void Si_stray(void) {  /* DD/FD was stray.. another DD/FD or unprefixed
                         instruction followed */
  smc++;
  ei_opm[0][opcode](); /* execute the instruction(without modifiers) */
}

void z80_resetstat(void) {
  int i,j;
  
  for(i=0;i<7;i++)
    for(j=0;j<256;j++)
      stat_tab[i][j]=0;
}

int z80_readinstr(void) {

  opcode=z80_iget8();

  if(opcode==0xed) {
    incr_R(1);
    opcode=z80_iget8();
    ei_tab=ei_edop;
    stat_i=6;
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
    stat_i=cpus.modifier+3;
//    prefix2=0xcb;
    return 0;
  }

  /* one-byte opcode */
//  prefix2=0;
  ei_tab=ei_opm[cpus.modifier];
  stat_i=cpus.modifier;
  return 0;
}

void z80_execinstr(void) {
  int lastuoc;

  cpus.int_lock=0;

  if (0) z80_dump_regs();
/*  if(cpus.PC>=40000 && cpus.PC<=51000)
    z80_fprintstatus(logfi);*/
  
  if(cpus.halted) { /* NOP */
    z80_clock+=4;
    incr_R(1);
  } else {
    //printf("read instr..\n");
    z80_readinstr();

    //printf("exec instr..\n");
    stat_tab[stat_i][opcode]++;
    
    lastuoc=uoc;
    ei_tab[opcode](); /* FAST branch .. execute the instruction */
    
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
}

int z80_int(u8 data) {
  u16 addr;
//  printf("interrupt->DI\n");

  if(cpus.int_lock) return -1; /* after EI or DI or inside an instruction */
  if(!cpus.IFF1) return -1; /* IFF2 just tells the NMI service routine
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
      z80_clock+=13;
      break;
    case 1:
      incr_R(2);
      _push16(cpus.PC);
      cpus.PC=0x0038;
      z80_clock+=13;
      break;
    case 2:
      incr_R(2);
      _push16(cpus.PC);
      addr=((u16)cpus.I<<8) | (u16)data;
      cpus.PC=zx_memget16(addr);
//      printf("IM 2 INT: I=%02x D8=%02x tabi=%04x dst=%04x\n",cpus.I,data,addr,cpus.PC);
      z80_clock+=19;
      break;
  }
  
  return 0;
}

int z80_nmi(void) {

  if(cpus.int_lock) return -1;
  printf("NMI!\n");
  
  cpus.halted=0;
  cpus.IFF1=0;

  _push16(cpus.PC);
  cpus.PC=0x0066;
  
  incr_R(2);
  z80_clock+=11;
  
  return 0;
}

int z80_reset(void) {
  cpus.IFF1=cpus.IFF2=0;
  cpus.int_mode=0;
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

#include "z80g.c"
