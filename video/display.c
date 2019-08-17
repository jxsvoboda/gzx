/*
 * GZX - George's ZX Spectrum Emulator
 * Video display options
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

#include <stdlib.h>
#include "display.h"

const char *video_area_str(video_area_t area)
{
	switch (area) {
	case varea_320x200:
		return "320x200";
	case varea_320x240:
		return "320x240";
	case varea_336x252:
		return "336x252";
	case varea_352x288:
		return "352x288";
	}

	return NULL;
}

void video_area_size(video_area_t area, int *x, int *y)
{
	switch (area) {
	case varea_320x200:
		*x = 320;
		*y = 200;
		break;
	case varea_320x240:
		*x = 320;
		*y = 240;
		break;
	case varea_336x252:
		*x = 336;
		*y = 252;
		break;
	case varea_352x288:
		*x = 352;
		*y = 288;
		break;

	}
}

video_area_t video_area_prev(video_area_t area)
{
	if (area > varea_320x200)
		return area - 1;
	else
		return varea_352x288;
}

video_area_t video_area_next(video_area_t area)
{
	if (area < varea_352x288)
		return area + 1;
	else
		return varea_320x200;
}
