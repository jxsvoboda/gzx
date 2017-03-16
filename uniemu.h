#ifndef _UNIEMU_H
#define _UNIEMU_H

/* pod win32 dodeklaruj nektere linuxove funkce emulovane v sys_win.c */

#ifdef __MINGW32__
#include "sys_win.h"
#else
#include <unistd.h>
#endif

#endif
