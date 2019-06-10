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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "gzx.h"
#include "intdef.h"
#include "memio.h"
#include "strutil.h"
#include "zx_tape.h"
#include "z80.h"
#include "zxt_fif.h"
#include "zxt_ng.h"
#include "tape/player.h"
#include "tape/tape.h"

static tape_player_t *player;

static int tape_delta_t;
static int tape_playing,tape_paused;

static tfr_t *tfr;

/*** quick load ***/
void zx_tape_ldbytes(void) {
  unsigned req_flag,toload,addr,verify;
  unsigned u;
  u8 flag,b,x=0,chksum;
  unsigned error;
  int btype;
  tb_data_info_t binfo;
  
  fprintf(logfi,"zx_tape_ldbytes()\n");
  
  fprintf(logfi,"tfr?\n");
  if(!tfr) return;
  fprintf(logfi,"!tape_playing?\n");
  if(tape_playing) return;
  btype=tfr->block_type();
  fprintf(logfi,"btype==BT_DATA?\n");
  while (btype != BT_DATA) {
    if(btype != BT_UNKNOWN) return;
    if (tfr->skip_block() < 0) return;
    btype=tfr->block_type();
  }
  fprintf(logfi,"!getinfo?\n");
  if(tfr->get_b_data_info(&binfo)) return;
  
  fprintf(logfi,"...\n");
  req_flag=cpus.r_[rA];
  toload=((u16)cpus.r[rD]<<8) | (u16)cpus.r[rE];
  addr=cpus.IX;
  verify=(cpus.F_&fC)?0:1; 
    
  if(tfr->open_block()) return;
  
  error=0;
    
  if(tfr->b_data_getbytes(1,&flag)) error=1;
  
  fprintf(logfi,"req:len %d, flag 0x%02x, addr 0x%04x, verify:%d\n",
          toload,req_flag,addr,verify);
  fprintf(logfi,"block len %u, block flag:0x%02x\n",binfo.data_bytes,flag);
  fprintf(logfi,"z80 F:%02x\n",cpus.F_);
  
  if(flag!=req_flag) error=1;
  
  if(!error) {
    if(verify) fprintf(logfi,"verifying\n");
      else fprintf(logfi,"loading\n");
      
    x=flag;
    for(u=0;u<toload;u++) {
      if(tfr->b_data_getbytes(1,&b)) {
        error=1;
	fprintf(logfi,"out of data\n");
	break;
      }
      if(!verify)
        zx_memset8(addr+u,b);
      x^=b;
    }
  }
  
  if(!error) {
    if(tfr->b_data_getbytes(1,&chksum)) {
      error=1;
      fprintf(logfi,"out of data\n");
    }
  }
  
  if(!error) {
    fprintf(logfi,"stored chksum:$%02x computed:$%02x\n",chksum,x);
    if(chksum!=x) {
      fprintf(logfi,"wrong checksum\n");
      error=1;
    }
  }

  if(error) {
    cpus.F &= ~fC;
  } else {
    cpus.F |= fC;
    fprintf(logfi,"load ok\n");
  }
  
  tfr->close_block();
  
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

static tape_lvl_t cur_lvl;
static uint32_t next_delay;
static tape_lvl_t next_lvl;

int zx_tape_selectfile(char *name) {
  char *ext;
  int res;
  
  if(tfr) tfr->close_file();
  
  ext=strrchr(name,'.');
  if(!ext) {
    printf("file has no extension\n");
    return -1;
  }
  
  tfr = &tfr_ng;
  
  res=tfr->open_file(name);
    
  if(res<0) {
    printf("error opening tape file\n");
    return -1;
  }
  tape_player_init(player, tape_first(ng_get_tape()));
  cur_lvl = tape_player_cur_lvl(player);
  if (!tape_player_is_end(player))
    tape_player_get_next(player, &next_delay, &next_lvl);
  else
    tape_playing = 0;
  return 0;
}

int zx_tape_init(int delta_t) {
  int rc;
  tape_playing=0;
  tfr=NULL;
  tape_delta_t=delta_t;
  rc = tape_player_create(&player);
  if (rc != 0)
	return -1;
  return 0;
}

void zx_tape_done(void) {
  if(tfr) tfr->close_file();
  if (player != NULL) {
	tape_player_destroy(player);
	player = NULL;
  }
}

void zx_tape_getsmp(u8 *smp) {
  uint32_t td;

  if (!tape_playing || tape_paused) {
        *smp = cur_lvl;
	return;
  }

  td = tape_delta_t;
  while (next_delay <= td && !tape_player_is_end(player)) {
    td -= next_delay;
    cur_lvl = next_lvl;
    tape_player_get_next(player, &next_delay, &next_lvl);
  }

  if (next_delay > td)
    next_delay -= td;
  *smp = cur_lvl;
}

void zx_tape_play(void) {
  tape_playing=1;
  tape_paused=0;
}

void zx_tape_pause(void) {
  tape_paused=1;
}

void zx_tape_stop(void) {
  tape_playing=0;
}

void zx_tape_rewind(void) {
  if(tfr) tfr->rewind_file();

  tape_player_init(player, tape_first(ng_get_tape()));
  cur_lvl = tape_player_cur_lvl(player);
  if (!tape_player_is_end(player))
    tape_player_get_next(player, &next_delay, &next_lvl);
  else
    tape_playing = 0;
}
