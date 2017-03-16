/*
  GZX - George's ZX Spectrum Emulator
  Integrated Debugger
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "debug.h"
#include "gzx.h"
#include "memio.h"
#include "mgfx.h"
#include "zx_scr.h"
#include "z80.h"
#include "disasm.h"

#define MK_PAIR(hi,lo) ( (((u16)(hi)) << 8) | (lo) )

#define HEX_CY 18
#define INSTR_CY 9

#define INSTR_LINES 6

static u16 hex_base;

static u16 instr_base;

static int ic_ln; /* instruction cursor line number */

static void d_istep(void) {
  zx_debug_mstep();
}

static void dreg(char *name, u16 value) {
  char buf[16];
  
  fgc=7; gputs(name); gputc(':'); fgc=5;
  sprintf(buf,"%04X",value); gputs(buf);
}

static void dflag(char *name, int value) {
  char buf[8];
  
  fgc=7; gputs(name); fgc=5;
  sprintf(buf,"%d ",value); gputs(buf);
}

static void d_regs(void) {
  
  mgfx_fillrect(0,0,scr_xs-1,scr_ys-1,0);
  
  bgc=0;
  
  fgc=7;
  gmovec(scr_xs/16-(strlen("Debugger")/2),0);
  gputs("Debugger");
  
  fgc=5;
  
  gmovec(1,2); dreg("AF", MK_PAIR(cpus.r[rA],cpus.F));
  gmovec(1,3); dreg("BC", MK_PAIR(cpus.r[rB],cpus.r[rC]));
  gmovec(1,4); dreg("DE", MK_PAIR(cpus.r[rD],cpus.r[rE]));
  gmovec(1,5); dreg("HL", MK_PAIR(cpus.r[rH],cpus.r[rL]));
  
  gmovec(10,2); dreg("A'F'", MK_PAIR(cpus.r_[rA],cpus.F_));
  gmovec(10,3); dreg("B'C'", MK_PAIR(cpus.r_[rB],cpus.r_[rC]));
  gmovec(10,4); dreg("D'E'", MK_PAIR(cpus.r_[rD],cpus.r_[rE]));
  gmovec(10,5); dreg("H'L'", MK_PAIR(cpus.r_[rH],cpus.r_[rL]));
  
  gmovec(21,2); dreg("IX", cpus.IX);
  gmovec(21,3); dreg("IY", cpus.IY);
  gmovec(21,4); dreg("IR", MK_PAIR(cpus.I, cpus.R));
  gmovec(21,5); dreg("SP", cpus.SP);

  gmovec(30,2); dflag("IFF1:",cpus.IFF1);
  gmovec(30,3); dflag("IFF2:",cpus.IFF2);
  gmovec(30,4); dflag("IM:  ",cpus.int_mode);
  gmovec(30,5); dflag("HLT: ",cpus.halted);

  gmovec(1,7);
  dflag("S:",(cpus.F&fS)!=0);
  dflag("Z:",(cpus.F&fZ)!=0);
  dflag("H:",(cpus.F&fHC)!=0);
  dflag("PV:",(cpus.F&fPV)!=0);
  dflag("N:",(cpus.F&fN)!=0);
  dflag("C:",(cpus.F&fC)!=0);
  
  gmovec(30,7); dreg("PC", cpus.PC);
}

static void d_hex(void) {
  int i,j;
  char buf[16];
  u8 b;
  
  for(i=0;i<6;i++) {
    fgc=7;
    sprintf(buf,"%04X:",hex_base+8*i);
    gmovec(1,HEX_CY+i); gputs(buf);
    fgc=5;
    for(j=0;j<8;j++) {      
      b=zx_memget8(hex_base+8*i+j);
      gputc(b);
    }
    for(j=0;j<8;j++) {
      b=zx_memget8(hex_base+8*i+j);
      sprintf(buf,"%02X", b);
      gmovec(16+3*j,HEX_CY+i);
      gputs(buf);
    }
  }
}

static void d_instr(void) {
  int i;
  unsigned xpos,c;
  char buf[16];
  
  disasm_org=xpos=instr_base;
  
  for(i=0;i<INSTR_LINES;i++) {
    bgc = (ic_ln==i) ? 1 : 0;
    fgc=7;
    sprintf(buf,"%04X:",disasm_org&0xffff);
    gmovec(1,INSTR_CY+i); gputs(buf);
    
    disasm_instr();

    fgc=5;    
    for(c=xpos;c!=disasm_org;c++) {
      sprintf(buf,"%02X",zx_memget8(c));
      gputs(buf);
    }
    
    fgc=4;
    gmovec(15,INSTR_CY+i); gputs(disasm_buf);
    
    xpos=disasm_org;
  }
  bgc=0;
}

static void instr_next(void) {
  disasm_org=instr_base;
  disasm_instr();
  instr_base=disasm_org;
}

/*
  Jak najdeme predchozi instrukci:
    Jdeme o BKTRACE bytu zpet a doufame,
    ze se do dosazeni aktualni pozice sesynchronizujeme.
    Zapamatujeme si pozici posledni instrukce pred akt. pozici.
*/

#define BKTRACE 8

static void instr_prev(void) {
  u16 c,last;
  
  disasm_org = instr_base-BKTRACE;
  c=0;
  do {
    last=disasm_org;
    disasm_instr();
    c++;
  } while(c<BKTRACE && disasm_org!=instr_base);
  
  if(c==BKTRACE) {
    instr_base--;
    return;
  }
  instr_base = last;
}

static void d_trace(void) {
  d_istep();
  instr_base=cpus.PC;
}

static void d_run_upto(u16 addr) {
  wkey_t k;
  int esc;
  
  esc=0;
  do {
    d_istep();
    mgfx_input_update();
    while(w_getkey(&k)) {
      if(k.press && (k.key==WKEY_ESC))
        esc=1;
      else
        zx_debug_key(k.press,k.key);
    }
  } while(!esc && cpus.PC!=addr);
  instr_base=cpus.PC;
}

static void d_stepover(void) {
  u8 b;
  
  b = zx_memget8(cpus.PC);
  if(b==0xCD || (b&0xC7)==0xC4) {
    /* CALL or CALL cond */
    disasm_org=cpus.PC;
    disasm_instr();
    d_run_upto(disasm_org);
  } else {
    d_trace();
  }
}

static void d_to_cursor(void) {
  int i;
  
  disasm_org=instr_base;
  for(i=0;i<ic_ln;i++)
    disasm_instr();
    
  d_run_upto(disasm_org);
}

static void d_view_scr(void) {
  wkey_t k;
  
  zx_scr_disp_fast();
  mgfx_updscr();
  while(1) {
    mgfx_input_update();
    if(w_getkey(&k)) {
      if(k.press && k.key==WKEY_ESC)
        break;
    }
  }
}

static void curs_up(void) {
  if(ic_ln>0) ic_ln--;
    else instr_prev();
}

static void curs_down(void) {
  if(ic_ln<INSTR_LINES-1) ic_ln++;
    else instr_next();
}

static void curs_pgup(void) {
  int i;
  for(i=0;i<INSTR_LINES-1;i++)
    curs_up();
}

static void curs_pgdown(void) {
  int i;
  for(i=0;i<INSTR_LINES-1;i++)
    curs_down();
}

void debugger(void) {
  wkey_t k;
  
  instr_base = cpus.PC;
  ic_ln = 0;
      
  while(1) {
    d_regs();
    d_hex();
    d_instr();
    mgfx_updscr();
    do {
      mgfx_input_update();
      usleep(1000);
    } while(!w_getkey(&k));
    
    if(k.press)
    switch(k.key) {
      case WKEY_ESC:
	return;

      case WKEY_ENTER:
        //fn_line.buf[fn_line.len]=0;
	//zx_save_snap(fn_line.buf);
	return;

      case WKEY_UP: curs_up(); break;
      case WKEY_DOWN: curs_down(); break;
      case WKEY_PGUP: curs_pgup(); break;
      case WKEY_PGDN: curs_pgdown(); break;
      case WKEY_HOME: instr_base-=256; break;
      case WKEY_END: instr_base+=256; break;
	
      case WKEY_F7: d_trace(); break;
      case WKEY_F8: d_stepover(); break;
      case WKEY_F9: d_to_cursor(); break;
      case WKEY_F11: d_view_scr(); break;
        
      default:
        //teline_key(&fn_line,&k);
	break;
    }
  }
}
