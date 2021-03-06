/*
 * GZX - George's ZX Spectrum Emulator
 * String utility functions
 *
 * Copyright (c) 1999-2018 Jiri Svoboda
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "strutil.h"

/** Case-insensitive strcmp.
 *
 * @param a First string
 * @param b Second string
 * @return Less than zero, zero, greater than zero, if a < b, a == b, a > b,
 *         respectively.
 */
int strcmpci(const char *a, const char *b)
{
	while (*a && *b && tolower(*a) == tolower(*b)) {
		a++;
		b++;
	}

	return tolower(*a) - tolower(*b);
}

/** Duplicate string.
 *
 * @param s String
 * @return Copy of string or @c NULL if out of memory
 */
char *strdupl(const char *s)
{
	char *ns;
	size_t len;

	len = strlen(s);
	ns = malloc(len + 1);
	if (ns == NULL)
		return NULL;

	memcpy(ns, s, len + 1);
	return ns;
}
