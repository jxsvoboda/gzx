/*
  GZX - George's ZX Spectrum Emulator
  UN*X system
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
