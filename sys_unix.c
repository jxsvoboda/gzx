/*
 * GZX - George's ZX Spectrum Emulator
 * UN*X system platform wrapper
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

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "clock.h"
#include "sys_all.h"

static DIR *sd;

void timer_reset(timer *t) {
  struct timezone tz;
  struct timeval tv;
  
  gettimeofday(&tv,&tz);
  t->sec=tv.tv_sec;
  t->usec=tv.tv_usec;
}

unsigned long timer_val(timer *t) {
  struct timezone tz;
  struct timeval tv;
  unsigned long usec,sec;
  unsigned long long tstates;
  
  gettimeofday(&tv,&tz);
  usec=tv.tv_usec-t->usec;
  sec=tv.tv_sec-t->sec;

  /* calculate tstates modulo 2^32 */
  tstates=sec*(unsigned long)Z80_CLOCK + ((usec*35)/10);
  
  return tstates;
}

int sys_chdir(const char *path) {
  return chdir(path);
}

char *sys_getcwd(char *buf, int buflen) {
  return getcwd(buf,buflen);
}

int sys_isdir(char *filename) {
  struct stat statbuf;

  stat(filename,&statbuf);
  return S_ISDIR(statbuf.st_mode);
}

int sys_opendir(char *path) {
  sd = opendir(path);
  return sd ? 0 : -1;
}

int sys_readdir(char **name, int *is_dir) {
  struct dirent *de;

  de = readdir(sd);
  if(!de) return -1;

  *name = de->d_name;
  *is_dir = sys_isdir(de->d_name);
  return 0;
}

void sys_closedir(void) {
  closedir(sd);
}
