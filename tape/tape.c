/*
 * GZX - George's ZX Spectrum Emulator
 * Spectrum tape
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

/**
 * @file Spectrum tape.
 *
 * This is an in-core, editable, representation of Spectrum tape. It should
 * be able to perfectly represent any TZX file.
 */

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include "../adt/list.h"
#include "../types/tape/tape.h"
#include "tape.h"

/** Create empty tape.
 *
 * @param rtape Place to store pointer to new tape
 * @return Zero on success or error code
 */
#include <stdio.h>
int tape_create(tape_t **rtape)
{
	tape_t *tape;

	tape = calloc(1, sizeof(tape_t));
	if (tape == NULL)
		return ENOMEM;

	list_initialize(&tape->blocks);

	printf("Allocated tape=%p\n", tape);
	*rtape = tape;
	return 0;
}

/** Destroy tape.
 *
 * @param tape Tape
 */
void tape_destroy(tape_t *tape)
{
	tape_block_t *block;

	printf("Freeing tape=%p\n", tape);
	if (tape == NULL)
		return;

	block = tape_first(tape);
	while (block != NULL) {
		tape_block_destroy(block);
		block = tape_first(tape);
	}

	printf("Freeing tape=%p\n", tape);
	free(tape);
}

/** Get first block of tape.
 *
 * @param tape Tape
 * @return First block or @c NULL
 */
tape_block_t *tape_first(tape_t *tape)
{
	link_t *link;

	link = list_first(&tape->blocks);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tape_block_t, ltape);
}

/** Get last block of tape.
 *
 * @param tape Tape
 * @return Last block or @c NULL
 */
tape_block_t *tape_last(tape_t *tape)
{
	link_t *link;

	link = list_last(&tape->blocks);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tape_block_t, ltape);
}

/** Get next block of tape.
 *
 * @param cur Current block
 * @return Next block or @c NULL
 */
tape_block_t *tape_next(tape_block_t *cur)
{
	link_t *link;

	link = list_next(&cur->ltape, &cur->tape->blocks);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tape_block_t, ltape);
}

/** Get previous block of tape.
 *
 * @param cur Current block
 * @return Previous block or @c NULL
 */
tape_block_t *tape_prev(tape_block_t *cur)
{
	link_t *link;

	link = list_prev(&cur->ltape, &cur->tape->blocks);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tape_block_t, ltape);
}

/** Append block to tape.
 *
 * @param tape Tape
 * @param block Block to append
 */
void tape_append(tape_t *tape, tape_block_t *block)
{
	block->tape = tape;
	list_append(&block->ltape, &tape->blocks);
}

/** Create tape block.
 *
 * @param btype Block type
 * @param ext Block-specific data
 * @param rblock Place to store pointer to newly allocated block
 * @return Zero on success, ENOMEM if out of memory
 */
static int tape_block_create(tape_btype_t btype, void *ext,
    tape_block_t **rblock)
{
	tape_block_t *block;

	block = calloc(1, sizeof(tape_block_t));
	if (block == NULL)
		return ENOMEM;

	block->btype = btype;
	block->ext = ext;
	*rblock = block;
	return 0;
}

/** Destroy block base object.
 *
 * Destroy just the base block object.
 *
 * @param block Tape block
 */
static void tape_block_destroy_base(tape_block_t *block)
{
	if (block == NULL)
		return;

	if (link_used(&block->ltape))
		list_remove(&block->ltape);

	free(block);
}

/** Destroy tape block.
 *
 * @param block Tape block
 */
void tape_block_destroy(tape_block_t *block)
{
	switch (block->btype) {
	case tb_data:
		tblock_data_destroy((tblock_data_t *) block->ext);
		break;
	case tb_archive_info:
		tblock_archive_info_destroy((tblock_archive_info_t *)
		    block->ext);
		break;
	case tb_unknown:
		tblock_unknown_destroy((tblock_unknown_t *) block->ext);
		break;
	default:
		assert(false);
		break;
	}
}

/** Create standard speed data.
 *
 * @param rdata Place to store pointer to new archive info
 * @return Zero on success or error code
 */
int tblock_data_create(tblock_data_t **rdata)
{
	tblock_data_t *data;
	tape_block_t *block = NULL;
	int rc;

	data = calloc(1, sizeof(tblock_data_t));
	if (data == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_data, data, &block);
	if (rc != 0)
		goto error;

	data->block = block;

	*rdata = data;
	return 0;
error:
	if (data != NULL)
		free(data);
	return rc;
}

/** Destroy standard speed data.
 *
 * @param data Standard speed data
 */
void tblock_data_destroy(tblock_data_t *data)
{
	if (data == NULL)
		return;

	if (data->data != NULL)
		free(data->data);

	tape_block_destroy_base(data->block);
	free(data);
}

/** Create archive info.
 *
 * @param rainfo Place to store pointer to new archive info
 * @return Zero on success or error code
 */
int tblock_archive_info_create(tblock_archive_info_t **rainfo)
{
	tblock_archive_info_t *ainfo;
	tape_block_t *block = NULL;
	int rc;

	ainfo = calloc(1, sizeof(tblock_archive_info_t));
	if (ainfo == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_archive_info, ainfo, &block);
	if (rc != 0)
		goto error;

	ainfo->block = block;
	list_initialize(&ainfo->texts);

	*rainfo = ainfo;
	return 0;
error:
	if (ainfo != NULL)
		free(ainfo);
	return rc;
}

/** Destroy archive info.
 *
 * @param ainfo Archive info
 */
void tblock_archive_info_destroy(tblock_archive_info_t *ainfo)
{
	tape_text_t *text;

	if (ainfo == NULL)
		return;

	text = tblock_archive_info_first(ainfo);
	while (text != NULL) {
		tape_text_destroy(text);
		text = tblock_archive_info_first(ainfo);
	}

	tape_block_destroy_base(ainfo->block);
	free(ainfo);
}

/** Get first text of archive info.
 *
 * @param ainfo Archive info
 * @return First text or @c NULL
 */
tape_text_t *tblock_archive_info_first(tblock_archive_info_t *ainfo)
{
	link_t *link;

	link = list_first(&ainfo->texts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tape_text_t, lainfo);
}

/** Get last text of archive info.
 *
 * @param ainfo Archive info
 * @return Last text or @c NULL
 */
tape_text_t *tblock_archive_info_last(tblock_archive_info_t *ainfo)
{
	link_t *link;

	link = list_last(&ainfo->texts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tape_text_t, lainfo);
}

/** Get next text of archive info.
 *
 * @param cur Current text
 * @return Next text or @c NULL
 */
tape_text_t *tblock_archive_info_next(tape_text_t *cur)
{
	link_t *link;

	link = list_next(&cur->lainfo, &cur->ainfo->texts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tape_text_t, lainfo);
}

/** Get previous text of archive info.
 *
 * @param cur Current text
 * @return Previous text or @c NULL
 */
tape_text_t *tblock_archive_info_prev(tape_text_t *cur)
{
	link_t *link;

	link = list_prev(&cur->lainfo, &cur->ainfo->texts);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tape_text_t, lainfo);
}

/** Create tape text.
 *
 * @param rtext Place to store pointer to new tape text
 * @return Zero on success or error code
 */
int tape_text_create(tape_text_t **rtext)
{
	tape_text_t *text;

	text = calloc(1, sizeof(tape_text_t));
	if (text == NULL)
		return ENOMEM;

	*rtext = text;
	return 0;
}

/** Destroy tape text.
 *
 * @param text Tape text
 */
void tape_text_destroy(tape_text_t *text)
{
	if (text == NULL)
		return;

	if (link_used(&text->lainfo))
		list_remove(&text->lainfo);

	if (text->text != NULL)
		free(text->text);

	free(text);
}

/** Create unknown block.
 *
 * @param runknown Place to store pointer to new unknown block
 * @return Zero on success or error code
 */
int tblock_unknown_create(tblock_unknown_t **runknown)
{
	tblock_unknown_t *unknown;
	tape_block_t *block = NULL;
	int rc;

	unknown = calloc(1, sizeof(tblock_unknown_t));
	if (unknown == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_archive_info, unknown, &block);
	if (rc != 0)
		goto error;

	unknown->block = block;
	*runknown = unknown;
	return 0;
error:
	if (unknown != NULL)
		free(unknown);
	return rc;
}

/** Destroy unknown block.
 *
 * @param unknown Unknown block
 */
void tblock_unknown_destroy(tblock_unknown_t *unknown)
{
	if (unknown == NULL)
		return;

	tape_block_destroy_base(unknown->block);
	free(unknown);
}
