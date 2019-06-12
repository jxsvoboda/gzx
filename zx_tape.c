/*
 * GZX - George's ZX Spectrum Emulator
 * Tape support
 *
 * Copyright (c) 1999-2019 Jiri Svoboda
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "gzx.h"
#include "intdef.h"
#include "memio.h"
#include "zx_tape.h"
#include "z80.h"
#include "tape/deck.h"

static tape_deck_t *tape_deck;

/*** quick load ***/
void zx_tape_ldbytes(void) {
  unsigned req_flag,toload,addr,verify;
  unsigned u;
  u8 flag,b,x=0,chksum;
  tblock_data_t *data;
  tape_block_t *tblock;
  
  fprintf(logfi,"zx_tape_ldbytes()\n");
  
  fprintf(logfi,"!tape_playing?\n");
  if(tape_deck_is_playing(tape_deck)) return;
  
  tblock = tape_deck_cur_block(tape_deck);
  while (tblock != NULL && tblock->btype != tb_data) {
    tape_deck_next(tape_deck);
    tblock = tape_deck_cur_block(tape_deck);
  }
    
  fprintf(logfi,"tblock?\n");
  if(!tblock) return;
  
  assert(tblock->btype == tb_data);
  data = (tblock_data_t *)tblock->ext;
  
  fprintf(logfi,"...\n");
  req_flag=cpus.r_[rA];
  toload=((u16)cpus.r[rD]<<8) | (u16)cpus.r[rE];
  addr=cpus.IX;
  verify=(cpus.F_&fC)?0:1; 
  
  if (data->data_len < 1) {
    printf("Data block too short.\n");
    goto error;
  }
  
  flag = data->data[0];
  
  fprintf(logfi,"req:len %d, flag 0x%02x, addr 0x%04x, verify:%d\n",
          toload,req_flag,addr,verify);
  fprintf(logfi,"block len %u, block flag:0x%02x\n",data->data_len,flag);
  fprintf(logfi,"z80 F:%02x\n",cpus.F_);
  
  if(flag!=req_flag)
	goto error;
  
  if(verify) fprintf(logfi,"verifying\n");
    else fprintf(logfi,"loading\n");

  x=flag;
  for(u=0;u<toload;u++) {
    if (1 + u >= data->data_len) {
      fprintf(logfi,"out of data\n");
      goto error;
    }
    b = data->data[1 + u];
    if(!verify) {
      zx_memset8(addr+u,b);
      x^=b;
    }
  }
  
  if (1 + toload >= data->data_len) {
    fprintf(logfi,"out of data\n");
    goto error;
  }
  chksum = data->data[1 + toload];
  
  fprintf(logfi,"stored chksum:$%02x computed:$%02x\n",chksum,x);
  if(chksum!=x) {
    fprintf(logfi,"wrong checksum\n");
    goto error;
  }

  cpus.F |= fC;
  fprintf(logfi,"load ok\n");
  goto common;
error:
  cpus.F &= ~fC;
  fprintf(logfi,"load error\n");
common:
  
  tape_deck_next(tape_deck);
  /* RET */
  fprintf(logfi,"returning\n");
  cpus.PC=zx_memget16(cpus.SP);
  cpus.SP+=2;
}

/*** quick save ***/
void zx_tape_sabytes(void) {
}

/*void zx_tape_sabytes(void) {
  unsigned flag,tosave,addr;
  unsigned x,u,b;
  unsigned error;
  tape_block block;
  
  if(sablock) {
    //printf("sabytes()\n");
    flag=cpus.r_[rA];
    tosave=((u16)cpus.r[rD]<<8) | (u16)cpus.r[rE];
    addr=cpus.IX;
    
    block.type=BT_DATA;
    block.len=tosave+2;
    block.data=malloc(block.len);
    
    block.data[0]=flag;
  
    fprintf(logfi,"wr:len %d, flag 0x%02x, addr 0x%04x\n",
           tosave,flag,addr);
  
    error=0;
      
    fprintf(logfi,"writing\n");
    x=flag;
    for(u=0;u<tosave;u++) {
      b=zx_memget8(addr+u);
      block.data[1+u]=b;
      x^=b;
    }
    block.data[1+tosave]=x;*/ /* write checksum */
    
/*    if(!wtapf) if(w_open("out.tap")<0) return;
    sablock(&block);
    if(flag!=0x00) w_close();*/ /* not a header.. close the file! */
    
/*    cpus.F=error ? (cpus.F&(~fC)) : (cpus.F | fC);
    if(!error) {
      fprintf(logfi,"write ok\n");
    
    
    freeblock(&block);*/
    
    //printf("RET\n");
    /* RET */
/*    cpus.PC=zx_memget16(cpus.SP);
    cpus.SP+=2;
  }
}*/

int zx_tape_selectfile(char *name) {
  int rc;
 
  rc = tape_deck_open(tape_deck, name);
  if (rc != 0)
    return -1;

  return 0;
}

int zx_tape_init(int delta_t) {
  int rc;

  rc = tape_deck_create(&tape_deck);
  if (rc != 0)
    return -1;

  tape_deck->delta_t = delta_t;
  return 0;
}

void zx_tape_done(void) {
  if(tape_deck != NULL) {
    tape_deck_destroy(tape_deck);
    tape_deck = NULL;
  }
}

void zx_tape_getsmp(u8 *smp) {
  tape_deck_getsmp(tape_deck, smp);
}

void zx_tape_play(void) {
  tape_deck_play(tape_deck);
}

void zx_tape_pause(void) {
  tape_deck_pause(tape_deck);
}

void zx_tape_stop(void) {
  tape_deck_stop(tape_deck);
}

void zx_tape_rewind(void) {
  tape_deck_rewind(tape_deck);
}
