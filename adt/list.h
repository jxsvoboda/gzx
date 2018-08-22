/*
 * GZX - George's ZX Spectrum Emulator
 * Linked list
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

#ifndef ADT_LIST_H
#define ADT_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include "../types/adt/list.h"

#define list_get_instance(link, type, member) \
	((type *)( (void *)(link) - ((void *) &((type *) NULL)->member)))

#define list_foreach(list, member, itype, iterator) \
	for (itype *iterator = NULL; iterator == NULL; iterator = (itype *) 1) \
	    for (link_t *_link = (list).head.next; \
	    iterator = list_get_instance(_link, itype, member), \
	    _link != &(list).head; _link = _link->next)

#define list_foreach_rev(list, member, itype, iterator) \
	for (itype *iterator = NULL; iterator == NULL; iterator = (itype *) 1) \
	    for (link_t *_link = (list).head.prev; \
	    iterator = list_get_instance(_link, itype, member), \
	    _link != &(list).head; _link = _link->prev)

extern void list_initialize(list_t *);
extern void link_initialize(link_t *);
extern void list_insert_before(link_t *, link_t *);
extern void list_insert_after(link_t *, link_t *);
extern void list_prepend(link_t *, list_t *);
extern void list_append(link_t *, list_t *);
extern void list_remove(link_t *);
extern bool link_used(link_t *);
extern bool list_empty(list_t *);
extern unsigned long list_count(list_t *);
extern link_t *list_first(list_t *);
extern link_t *list_last(list_t *);
extern link_t *list_prev(link_t *, list_t *);
extern link_t *list_next(link_t *, list_t *);

#endif
