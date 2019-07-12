/*
 * GZX - George's ZX Spectrum Emulator
 * Text editing line
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

#include <string.h>

#include "../mgfx.h"
#include "teline.h"

void teline_empty(teline_t *t)
{
	t->len = t->pos = 0;
	memset(t->buf, ' ', t->maxlen);
	t->buf[t->maxlen] = 0;
}

void teline_settext(teline_t *t, char *s)
{
	int l;

	l = strlen(s);
	if (l > t->maxlen)
		l = t->maxlen;
	strncpy(t->buf, s, t->maxlen);
	memset(t->buf + l, ' ', t->maxlen - l);
	t->pos = t->len = l;
}

void teline_init(teline_t *t, int x, int y, int maxlen)
{
	if (maxlen > TELINE_MAX)
		maxlen = TELINE_MAX;
	t->maxlen = maxlen;

	t->x = x;
	t->y = y;
	t->focus = 0;

	teline_empty(t);
}

void teline_draw(teline_t *t)
{
	int p = t->pos;

	fgc = 7;
	bgc = 0;
	gmovec(t->x, t->y);
	gputs(t->buf);
	if (t->focus) {
		gmovec(t->x + p, t->y);
		fgc = 0;
		bgc = 5;
		gputc(t->buf[p]);
	}
}

static void teline_rmchar(teline_t *t, int pos)
{
	int i;

	for (i = pos; i < t->len - 1; i++)
		t->buf[i] = t->buf[i + 1];
	t->buf[--t->len] = ' ';
}

void teline_key(teline_t *t, wkey_t *k)
{
	if (!k->press)
		return;

	switch (k->key) {
	case WKEY_BS:
		if (t->pos > 0)
			teline_rmchar(t, --t->pos);
		break;
	case WKEY_DEL:
		if (t->pos < t->len)
			teline_rmchar(t, t->pos);
		break;
	case WKEY_LEFT:
		if (t->pos > 0)
			t->pos--;
		break;
	case WKEY_RIGHT:
		if (t->pos < t->len)
			t->pos++;
		break;

	default:
		if (k->c >= ' ' && k->c < 128 && t->len < t->maxlen) {
			int i;

			for (i = t->len; i > t->pos; i--)
				t->buf[i] = t->buf[i - 1];

			t->buf[t->pos++] = k->c;
			t->len++;
		}
		break;
	}
}
