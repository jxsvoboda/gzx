/*
  GZX - George's ZX Spectrum Emulator
  for win32: emulate some linux functions
  that are not supplied by mingw
  +drive enumeration
*/

#include <windows.h>
#include <mmsystem.h>
#include "clock.h"
#include "sys_win.h"

#ifndef _TIMEVAL_DEFINED
#define _TIMEVAL_DEFINED
struct timeval {
  long tv_sec;
  long tv_usec;
};
#endif

struct timezone {
  int tz_minuteswest;
  int tz_dsttime;
};

static int gettimeofday(struct timeval *tv, struct timezone *tz) {
  unsigned long t;
  
  t=timeGetTime();
  tv->tv_sec=t/1000;
  t=t%1000;
  tv->tv_usec=t*1000;

  return 0;
}

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

unsigned long win_enumdrives(void) {
  return GetLogicalDrives();
}

/* MINGW stuff */
#include <unistd.h>
#include <sys/stat.h>

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

#include <dirent.h>
static DIR *sd;

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
