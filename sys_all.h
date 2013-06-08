#ifndef _SYS_ALL_H
#define _SYS_ALL_H

#define SYS_PATH_MAX 128

typedef struct {
  long sec,usec;
} timer;

void timer_reset(timer *t);
unsigned long timer_val(timer *t);

unsigned usleep(unsigned int useconds);

int sys_chdir(const char *path);
char *sys_getcwd(char *buf, int buflen);

/* static buffers are evil, I know */
int sys_isdir(char *path);
int sys_opendir(char *path);
int sys_readdir(char **name, int *is_dir);
void sys_closedir(void);

#endif
