/*
 * GZX - George's ZX Spectrum Emulator
 * HelenOS system platform wrapper
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
#include <errno.h>
#include <fibril.h>
#include <stdlib.h>
#include <vfs/vfs.h>
#include "../../clock.h"
#include "../../sys_all.h"

static DIR *sd;

void timer_reset(timer *t)
{
}

unsigned long timer_val(timer *t)
{
	return 0;
}

int sys_chdir(const char *path)
{
	int rc;

	rc = vfs_cwd_set(path);
	if (rc != EOK)
		return -1;

	return 0;
}

char *sys_getcwd(char *buf, int buflen)
{
	int rc;

	rc = vfs_cwd_get(buf, buflen);
	if (rc != EOK)
		return NULL;

	return buf;
}

int sys_isdir(char *filename)
{
	vfs_stat_t statbuf;
	int rc;

	printf("sys_isdir('%s')?\n", filename);
	rc = vfs_stat_path(filename, &statbuf);
	printf("vfs_stat -> %d, is_dir=%d\n", rc, statbuf.is_directory);
	return statbuf.is_directory;
}

int sys_opendir(char *path)
{
	sd = opendir(path);
	return sd ? 0 : -1;
}

int sys_readdir(char **name, int *is_dir)
{
	struct dirent *de;

	de = readdir(sd);
	if (!de)
		return -1;

	*name = de->d_name;
	*is_dir = sys_isdir(de->d_name);
	return 0;
}

void sys_closedir(void)
{
	closedir(sd);
}

void sys_usleep(unsigned usec)
{
	fibril_usleep(usec);
}
