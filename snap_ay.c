/*
 * GZX - George's ZX Spectrum Emulator
 * AY format snapshot loading
 *
 * Copyright (c) 1999-2017 Jiri Svoboda
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
 * AY files are supposedly a music format, but they're really
 * a specialized machine state snapshot.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "z80.h"
#include "fileutil.h"
#include "memio.h"
#include "snap_ay.h"

/** Get absolutized value of AY relative pointer or 0 if pointer is 0. */
static long fgetayrp(FILE *f)
{
	int16_t rp;
	long pos;

	pos = ftell(f);
	rp = (int16_t)fgetu16be(f);
	if (rp == 0)
		return 0;

	return pos + rp;
}

/** Get null-terminated string */
static char *fgetntstr(FILE *f, long pos)
{
	uint8_t c;
	char *buf, *old_buf;
	size_t buf_size;
	size_t idx;

	if (fseek(f, pos, SEEK_SET) != 0)
		return NULL;

	buf_size = 32;
	buf = malloc(buf_size);
	if (buf == NULL)
		return NULL;
	idx = 0;

	c = fgetu8(f);
	while (c != '\0') {
		if (idx >= buf_size - 2) {
			buf_size = 2 * buf_size;
			old_buf = buf;
			buf = realloc(buf, buf_size);
			if (buf == NULL) {
				free(old_buf);
				return NULL;
			}
		}

		buf[idx++] = c;
		c = fgetu8(f);
	}

	buf[idx++] = '\0';
	return buf;
}

static int zx_load_snap_ay_block(FILE *f, uint16_t baddr, uint16_t blen,
    long pblock)
{
  size_t i;

  if (fseek(f, pblock, SEEK_SET) != 0) {
    printf("Seek to block data failed\n");
    return -1;
  }

  if ((uint32_t)baddr + blen > 65536)
    blen = 65536 - baddr;

  for (i = 0; i < blen; i++)
    zx_memset8f(baddr + i, fgetu8(f));

  return 0;
}

static int zx_load_snap_ay_song(FILE *f)
{
  long psong_name;
  long psong_data;
  char *song_name;
  uint8_t achan, bchan, cchan, noise;
  uint16_t song_len;
  uint16_t fade_len;
  uint8_t hireg, loreg;
  long ppoints;
  long paddrs;
  long cur_pos;
  uint16_t stack, init, inter;
  uint16_t baddr;
  uint16_t blen;
  long pblock;
  uint32_t p;
  uint16_t ploop, pjr;

  psong_name = fgetayrp(f);
  psong_data = fgetayrp(f);

  song_name = fgetntstr(f, psong_name);
  printf("Song name: %s\n", song_name);
  free(song_name);

  printf("PSongData: %lx\n", psong_data);
  if (fseek(f, psong_data, SEEK_SET) != 0) {
    printf("Seek to song data failed.\n");
    return -1;
  }

  achan = fgetu8(f);
  bchan = fgetu8(f);
  cchan = fgetu8(f);
  noise = fgetu8(f);
  (void) achan;
  (void) bchan;
  (void) cchan;
  (void) noise;
  printf("AChan:%d BChan:%d CChan:%d Noise:%d\n", achan, bchan, cchan, noise);

  song_len = fgetu16be(f);
  fade_len = fgetu16be(f);
  printf("Song length: %u Fade len: %u\n", song_len, fade_len);

  hireg = fgetu8(f);
  loreg = fgetu8(f);
  printf("Reg: %02x%02x\n", hireg, loreg);

  ppoints = fgetayrp(f);
  paddrs = fgetayrp(f);
  printf("PPoints: %lx PAddresses: %lx\n", ppoints, paddrs);

  if (fseek(f, ppoints, SEEK_SET) != 0) {
    printf("Seek to points structure failed.\n");
    return -1;
  }

  stack = fgetu16be(f);
  init = fgetu16be(f);
  inter = fgetu16be(f);
  printf("Stack=%04x Init=%04x Inter=%04x\n", stack, init, inter);

  if (fseek(f, paddrs, SEEK_SET) != 0) {
    printf("Seek to blocks failed.\n");
    return -1;
  }

  zx_select_memmodel(ZXM_48K);

  for (p = 0x0000; p < 0x0100; p++)
    zx_memset8f(p, 0xc9);
  for (p = 0x0100; p < 0x4000; p++)
    zx_memset8f(p, 0xff);
  for (p = 0x4000; p < 0x10000; p++)
    zx_memset8f(p, 0x00);
  zx_memset8f(0x0038, 0xfb);

  if (inter == 0) {
    p = 0x0000;
    zx_memset8f(p++, 0xf3); /* di */
    zx_memset8f(p++, 0xcd); /* call init */
    zx_memset8f(p++, init & 0xff);
    zx_memset8f(p++, init >> 8);
    ploop = p;              /* loop: */
    zx_memset8f(p++, 0xed); /* im 2 */
    zx_memset8f(p++, 0x5e);
    zx_memset8f(p++, 0xfb); /* ei */
    zx_memset8f(p++, 0x76); /* halt */
    pjr = p;
    zx_memset8f(p++, 0x18); /* jr loop */
    zx_memset8f(p++, ploop - (pjr + 2));
  } else {
    p = 0x0000;
    zx_memset8f(p++, 0xf3); /* di */
    zx_memset8f(p++, 0xcd); /* call init */
    zx_memset8f(p++, init & 0xff);
    zx_memset8f(p++, init >> 8);
    ploop = p;              /* loop: */
    zx_memset8f(p++, 0xed); /* im 1 */
    zx_memset8f(p++, 0x56);
    zx_memset8f(p++, 0xfb); /* ei */
    zx_memset8f(p++, 0x76); /* halt */
    zx_memset8f(p++, 0xcd); /* call interrupt */
    zx_memset8f(p++, inter & 0xff);
    zx_memset8f(p++, inter >> 8);
    pjr = p;
    zx_memset8f(p++, 0x18); /* jr loop */
    zx_memset8f(p++, ploop - (pjr + 2));
  }

  baddr = fgetu16be(f);
  while (baddr != 0) {
    blen = fgetu16be(f);
    pblock = fgetayrp(f);
    printf("Block addr: %04x len: %04x offs: %lx\n", baddr, blen, pblock);
    cur_pos = ftell(f);
    if (cur_pos < 0)
      return -1;

    if (zx_load_snap_ay_block(f, baddr, blen, pblock) != 0)
      return -1;

    if (fseek(f, cur_pos, SEEK_SET) != 0) {
      printf("Seek failed.\n");
      return -1;
    }

    baddr = fgetu16be(f);
  }
  printf("End of blocks.\n");

  cpus.r[rA] = cpus.r_[rA] = hireg;
  cpus.F = cpus.F_ = loreg;

  cpus.r[rH] = cpus.r_[rH] = hireg;
  cpus.r[rL] = cpus.r_[rL] = loreg;

  cpus.r[rD] = cpus.r_[rD] = hireg;
  cpus.r[rE] = cpus.r_[rE] = loreg;

  cpus.r[rD] = cpus.r_[rD] = hireg;
  cpus.r[rE] = cpus.r_[rE] = loreg;

  cpus.r[rB] = cpus.r_[rB] = hireg;
  cpus.r[rC] = cpus.r_[rC] = loreg;

  cpus.IX = ((uint16_t)hireg << 8) | loreg;
  cpus.IY = ((uint16_t)hireg << 8) | loreg;

  cpus.I = 3;
  cpus.SP = stack;
  cpus.PC = 0;

  /* Disable interrupts */
  cpus.IFF1 = cpus.IFF2 = 0;
  cpus.int_lock = 1;
  /* IM 0 */
  cpus.int_mode = 0;

  return 0;
}

/* returns 0 when ok, -1 on error -> reset ZX */
int zx_load_snap_ay(char *name) {
  FILE *f;
  char fileid[5];
  char typeid[5];
  uint8_t file_ver;
  uint8_t player_ver;
  long pspec_player;
  long pauthor;
  long pmisc;
  uint8_t num_songs;
  uint8_t first_song;
  long psong_struct;
  char *author;
  char *misc;
  int i;

  f = fopen(name,"rb");
  if (f == NULL) {
    printf("Cannot open snapshot file '%s'\n", name);
    return -1;
  }

  for (i = 0; i < 4; i++) {
    fileid[i] = fgetu8(f);
  }
  fileid[4] = '\0';
 
  for (i = 0; i < 4; i++) {
    typeid[i] = fgetu8(f);
  }
  typeid[4] = '\0';

  if (strcmp(fileid, "ZXAY") != 0 || strcmp(typeid, "EMUL") != 0) {
    printf("Invalid FileID/TypeID (%s/%s)\n", fileid, typeid);
    fclose(f);
    return -1;
  }

  file_ver = fgetu8(f);
  player_ver = fgetu8(f);

  printf("File version: %d player version: %d\n", file_ver, player_ver);

  pspec_player = fgetayrp(f);
  pauthor = fgetayrp(f);
  pmisc = fgetayrp(f);

  printf("PSpecialPlayer=%lx PAuthor=%lx PMisc=%lx\n", pspec_player, pauthor,
    pmisc); 

  num_songs = fgetu8(f);
  first_song = fgetu8(f);
  psong_struct = fgetayrp(f);

  printf("NumOfSongs=%d FirstSong=%d PSongStructure=%lx\n", num_songs,
    first_song, psong_struct);

  author = fgetntstr(f, pauthor);
  misc = fgetntstr(f, pmisc);
  printf("Author:%s Misc:%s\n", author, misc);
  free(author);
  free(misc);

  printf("PSongStruct: %lx\n", psong_struct);
  if (fseek(f, psong_struct, SEEK_SET) != 0) {
    printf("Seek to song structure failed.\n");
    fclose(f);
    return -1;
  }

  if (zx_load_snap_ay_song(f) != 0) {
    fclose(f);
    return -1;
  }

  fclose(f);
  return 0;
}
