/*
 * GZX - George's ZX Spectrum Emulator
 * Spectrum tape
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
int tape_create(tape_t **rtape)
{
	tape_t *tape;

	tape = calloc(1, sizeof(tape_t));
	if (tape == NULL)
		return ENOMEM;

	list_initialize(&tape->blocks);
	tape->version.major = 1;
	tape->version.minor = 1;

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

	if (tape == NULL)
		return;

	block = tape_first(tape);
	while (block != NULL) {
		tape_block_destroy(block);
		block = tape_first(tape);
	}

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

/** Insert block before another block.
 *
 * @param newb New block
 * @param oldb Old block
 */
void tape_insert_before(tape_block_t *newb, tape_block_t *oldb)
{
	newb->tape = oldb->tape;
	list_insert_before(&newb->ltape, &oldb->ltape);
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
	case tb_turbo_data:
		tblock_turbo_data_destroy((tblock_turbo_data_t *) block->ext);
		break;
	case tb_tone:
		tblock_tone_destroy((tblock_tone_t *) block->ext);
		break;
	case tb_pulses:
		tblock_pulses_destroy((tblock_pulses_t *) block->ext);
		break;
	case tb_pure_data:
		tblock_pure_data_destroy((tblock_pure_data_t *) block->ext);
		break;
	case tb_direct_rec:
		tblock_direct_rec_destroy((tblock_direct_rec_t *) block->ext);
		break;
	case tb_pause:
		tblock_pause_destroy((tblock_pause_t *) block->ext);
		break;
	case tb_stop:
		tblock_stop_destroy((tblock_stop_t *) block->ext);
		break;
	case tb_group_start:
		tblock_group_start_destroy((tblock_group_start_t *) block->ext);
		break;
	case tb_group_end:
		tblock_group_end_destroy((tblock_group_end_t *) block->ext);
		break;
	case tb_loop_start:
		tblock_loop_start_destroy((tblock_loop_start_t *) block->ext);
		break;
	case tb_loop_end:
		tblock_loop_end_destroy((tblock_loop_end_t *) block->ext);
		break;
	case tb_text_desc:
		tblock_text_desc_destroy((tblock_text_desc_t *) block->ext);
		break;
	case tb_archive_info:
		tblock_archive_info_destroy((tblock_archive_info_t *)
		    block->ext);
		break;
	case tb_hw_type:
		tblock_hw_type_destroy((tblock_hw_type_t *) block->ext);
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
 * @param rdata Place to store pointer to new standard speed data
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

/** Create turbo speed data.
 *
 * @param rtdata Place to store pointer to new turbo speed data
 * @return Zero on success or error code
 */
int tblock_turbo_data_create(tblock_turbo_data_t **rtdata)
{
	tblock_turbo_data_t *tdata;
	tape_block_t *block = NULL;
	int rc;

	tdata = calloc(1, sizeof(tblock_turbo_data_t));
	if (tdata == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_turbo_data, tdata, &block);
	if (rc != 0)
		goto error;

	tdata->block = block;

	*rtdata = tdata;
	return 0;
error:
	if (tdata != NULL)
		free(tdata);
	return rc;
}

/** Destroy turbo speed data.
 *
 * @param tdata Turbo speed data
 */
void tblock_turbo_data_destroy(tblock_turbo_data_t *tdata)
{
	if (tdata == NULL)
		return;

	if (tdata->data != NULL)
		free(tdata->data);

	tape_block_destroy_base(tdata->block);
	free(tdata);
}

/** Create pure tone.
 *
 * @param rtone Place to store pointer to new pure tone
 * @return Zero on success or error code
 */
int tblock_tone_create(tblock_tone_t **rtone)
{
	tblock_tone_t *tone;
	tape_block_t *block = NULL;
	int rc;

	tone = calloc(1, sizeof(tblock_tone_t));
	if (tone == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_tone, tone, &block);
	if (rc != 0)
		goto error;

	tone->block = block;

	*rtone = tone;
	return 0;
error:
	if (tone != NULL)
		free(tone);
	return rc;
}

/** Destroy pure tone.
 *
 * @param tone Pure tone
 */
void tblock_tone_destroy(tblock_tone_t *tone)
{
	if (tone == NULL)
		return;

	tape_block_destroy_base(tone->block);
	free(tone);
}

/** Create pulse sequence.
 *
 * @param rpulses Place to store pointer to new pulse sequence
 * @return Zero on success or error code
 */
int tblock_pulses_create(tblock_pulses_t **rpulses)
{
	tblock_pulses_t *pulses;
	tape_block_t *block = NULL;
	int rc;

	pulses = calloc(1, sizeof(tblock_pulses_t));
	if (pulses == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_pulses, pulses, &block);
	if (rc != 0)
		goto error;

	pulses->block = block;

	*rpulses = pulses;
	return 0;
error:
	if (pulses != NULL)
		free(pulses);
	return rc;
}

/** Destroy pulse sequence.
 *
 * @param pulses Pulse sequence
 */
void tblock_pulses_destroy(tblock_pulses_t *pulses)
{
	if (pulses == NULL)
		return;

	if (pulses->pulse_len != NULL)
		free(pulses->pulse_len);

	tape_block_destroy_base(pulses->block);
	free(pulses);
}

/** Create pure data.
 *
 * @param rpdata Place to store pointer to new pure data
 * @return Zero on success or error code
 */
int tblock_pure_data_create(tblock_pure_data_t **rpdata)
{
	tblock_pure_data_t *pdata;
	tape_block_t *block = NULL;
	int rc;

	pdata = calloc(1, sizeof(tblock_pure_data_t));
	if (pdata == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_pure_data, pdata, &block);
	if (rc != 0)
		goto error;

	pdata->block = block;

	*rpdata = pdata;
	return 0;
error:
	if (pdata != NULL)
		free(pdata);
	return rc;
}

/** Destroy pure data.
 *
 * @param pdata Pure data
 */
void tblock_pure_data_destroy(tblock_pure_data_t *pdata)
{
	if (pdata == NULL)
		return;

	if (pdata->data != NULL)
		free(pdata->data);

	tape_block_destroy_base(pdata->block);
	free(pdata);
}

/** Create direct recording.
 *
 * @param rdata Place to store pointer to new direct recording
 * @return Zero on success or error code
 */
int tblock_direct_rec_create(tblock_direct_rec_t **rdrec)
{
	tblock_direct_rec_t *drec;
	tape_block_t *block = NULL;
	int rc;

	drec = calloc(1, sizeof(tblock_direct_rec_t));
	if (drec == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_direct_rec, drec, &block);
	if (rc != 0)
		goto error;

	drec->block = block;

	*rdrec = drec;
	return 0;
error:
	if (drec != NULL)
		free(drec);
	return rc;
}

/** Destroy direct recording.
 *
 * @param drec Direct recording
 */
void tblock_direct_rec_destroy(tblock_direct_rec_t *drec)
{
	if (drec == NULL)
		return;

	if (drec->data != NULL)
		free(drec->data);

	tape_block_destroy_base(drec->block);
	free(drec);
}

/** Create pause (silence).
 *
 * @param rpause Place to store pointer to new pause (silence)
 * @return Zero on success or error code
 */
int tblock_pause_create(tblock_pause_t **rpause)
{
	tblock_pause_t *pause;
	tape_block_t *block = NULL;
	int rc;

	pause = calloc(1, sizeof(tblock_pause_t));
	if (pause == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_pause, pause, &block);
	if (rc != 0)
		goto error;

	pause->block = block;

	*rpause = pause;
	return 0;
error:
	if (pause != NULL)
		free(pause);
	return rc;
}

/** Destroy pause (silence).
 *
 * @param pause Pause (silence)
 */
void tblock_pause_destroy(tblock_pause_t *pause)
{
	if (pause == NULL)
		return;

	tape_block_destroy_base(pause->block);
	free(pause);
}

/** Create 'Stop the Tape'.
 *
 * @param rstop Place to store pointer to new 'Stop the Tape'
 * @return Zero on success or error code
 */
int tblock_stop_create(tblock_stop_t **rstop)
{
	tblock_stop_t *stop;
	tape_block_t *block = NULL;
	int rc;

	stop = calloc(1, sizeof(tblock_stop_t));
	if (stop == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_stop, stop, &block);
	if (rc != 0)
		goto error;

	stop->block = block;

	*rstop = stop;
	return 0;
error:
	if (stop != NULL)
		free(stop);
	return rc;
}

/** Destroy 'Stop the Tape'.
 *
 * @param stop Archive info
 */
void tblock_stop_destroy(tblock_stop_t *stop)
{
	if (stop == NULL)
		return;

	tape_block_destroy_base(stop->block);
	free(stop);
}

/** Create group start.
 *
 * @param rgstart Place to store pointer to new group start
 * @return Zero on success or error code
 */
int tblock_group_start_create(tblock_group_start_t **rgstart)
{
	tblock_group_start_t *gstart;
	tape_block_t *block = NULL;
	int rc;

	gstart = calloc(1, sizeof(tblock_group_start_t));
	if (gstart == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_group_start, gstart, &block);
	if (rc != 0)
		goto error;

	gstart->block = block;

	*rgstart = gstart;
	return 0;
error:
	if (gstart != NULL)
		free(gstart);
	return rc;
}

/** Destroy group start.
 *
 * @param gstart Group start
 */
void tblock_group_start_destroy(tblock_group_start_t *gstart)
{
	if (gstart == NULL)
		return;

	if (gstart->name != NULL)
		free(gstart->name);

	tape_block_destroy_base(gstart->block);
	free(gstart);
}

/** Create group end.
 *
 * @param rgend Place to store pointer to new group end
 * @return Zero on success or error code
 */
int tblock_group_end_create(tblock_group_end_t **rgend)
{
	tblock_group_end_t *gend;
	tape_block_t *block = NULL;
	int rc;

	gend = calloc(1, sizeof(tblock_group_end_t));
	if (gend == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_group_end, gend, &block);
	if (rc != 0)
		goto error;

	gend->block = block;

	*rgend = gend;
	return 0;
error:
	if (gend != NULL)
		free(gend);
	return rc;
}

/** Destroy group end.
 *
 * @param gend Group send
 */
void tblock_group_end_destroy(tblock_group_end_t *gend)
{
	if (gend == NULL)
		return;

	tape_block_destroy_base(gend->block);
	free(gend);
}

/** Create loop start.
 *
 * @param rlstart Place to store pointer to new loop start
 * @return Zero on success or error code
 */
int tblock_loop_start_create(tblock_loop_start_t **rlstart)
{
	tblock_loop_start_t *lstart;
	tape_block_t *block = NULL;
	int rc;

	lstart = calloc(1, sizeof(tblock_loop_start_t));
	if (lstart == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_loop_start, lstart, &block);
	if (rc != 0)
		goto error;

	lstart->block = block;

	*rlstart = lstart;
	return 0;
error:
	if (lstart != NULL)
		free(lstart);
	return rc;
}

/** Destroy loop start.
 *
 * @param lstart loop start
 */
void tblock_loop_start_destroy(tblock_loop_start_t *lstart)
{
	if (lstart == NULL)
		return;

	tape_block_destroy_base(lstart->block);
	free(lstart);
}

/** Create loop end.
 *
 * @param rlend Place to store pointer to new loop end
 * @return Zero on success or error code
 */
int tblock_loop_end_create(tblock_loop_end_t **rlend)
{
	tblock_loop_end_t *lend;
	tape_block_t *block = NULL;
	int rc;

	lend = calloc(1, sizeof(tblock_loop_end_t));
	if (lend == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_loop_end, lend, &block);
	if (rc != 0)
		goto error;

	lend->block = block;

	*rlend = lend;
	return 0;
error:
	if (lend != NULL)
		free(lend);
	return rc;
}

/** Destroy loop end.
 *
 * @param lend loop send
 */
void tblock_loop_end_destroy(tblock_loop_end_t *lend)
{
	if (lend == NULL)
		return;

	tape_block_destroy_base(lend->block);
	free(lend);
}

/** Create text description.
 *
 * @param rtdesc Place to store pointer to new text description
 * @return Zero on success or error code
 */
int tblock_text_desc_create(tblock_text_desc_t **rtdesc)
{
	tblock_text_desc_t *tdesc;
	tape_block_t *block = NULL;
	int rc;

	tdesc = calloc(1, sizeof(tblock_text_desc_t));
	if (tdesc == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_text_desc, tdesc, &block);
	if (rc != 0)
		goto error;

	tdesc->block = block;

	*rtdesc = tdesc;
	return 0;
error:
	if (tdesc != NULL)
		free(tdesc);
	return rc;
}

/** Destroy text description.
 *
 * @param tdesc Text description
 */
void tblock_text_desc_destroy(tblock_text_desc_t *tdesc)
{
	if (tdesc == NULL)
		return;

	if (tdesc->text != NULL)
		free(tdesc->text);

	tape_block_destroy_base(tdesc->block);
	free(tdesc);
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

/** Create hardware type.
 *
 * @param rhwtype Place to store pointer to new hardware type
 * @return Zero on success or error code
 */
int tblock_hw_type_create(tblock_hw_type_t **rhwtype)
{
	tblock_hw_type_t *hwtype;
	tape_block_t *block = NULL;
	int rc;

	hwtype = calloc(1, sizeof(tblock_hw_type_t));
	if (hwtype == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tape_block_create(tb_hw_type, hwtype, &block);
	if (rc != 0)
		goto error;

	hwtype->block = block;
	list_initialize(&hwtype->hwinfos);

	*rhwtype = hwtype;
	return 0;
error:
	if (hwtype != NULL)
		free(hwtype);
	return rc;
}

/** Destroy hardware type.
 *
 * @param hwtype Hardware type
 */
void tblock_hw_type_destroy(tblock_hw_type_t *hwtype)
{
	tape_hwinfo_t *hwinfo;

	if (hwtype == NULL)
		return;

	hwinfo = tblock_hw_type_first(hwtype);
	while (hwinfo != NULL) {
		tape_hwinfo_destroy(hwinfo);
		hwinfo = tblock_hw_type_first(hwtype);
	}

	tape_block_destroy_base(hwtype->block);
	free(hwtype);
}

/** Get first hardware info of hardware type.
 *
 * @param hwtype Hardware type
 * @return First hardware info or @c NULL
 */
tape_hwinfo_t *tblock_hw_type_first(tblock_hw_type_t *hwtype)
{
	link_t *link;

	link = list_first(&hwtype->hwinfos);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tape_hwinfo_t, lhw_type);
}

/** Get last hardware info of hardware type.
 *
 * @param hwtype Hardware type
 * @return Last hardware info or @c NULL
 */
tape_hwinfo_t *tblock_hw_type_last(tblock_hw_type_t *hwtype)
{
	link_t *link;

	link = list_last(&hwtype->hwinfos);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tape_hwinfo_t, lhw_type);
}

/** Get next hardware info of hardware type.
 *
 * @param cur Current hardware info
 * @return Next hardware info or @c NULL
 */
tape_hwinfo_t *tblock_hw_type_next(tape_hwinfo_t *cur)
{
	link_t *link;

	link = list_next(&cur->lhw_type, &cur->hw_type->hwinfos);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tape_hwinfo_t, lhw_type);
}

/** Get previous hardware info of hardware type.
 *
 * @param cur Current hardware info
 * @return Previous hardware info or @c NULL
 */
tape_hwinfo_t *tblock_hw_type_prev(tape_hwinfo_t *cur)
{
	link_t *link;

	link = list_prev(&cur->lhw_type, &cur->hw_type->hwinfos);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tape_hwinfo_t, lhw_type);
}

/** Create hardware info.
 *
 * @param rhwinfo Place to store pointer to new hardware info
 * @return Zero on success or error code
 */
int tape_hwinfo_create(tape_hwinfo_t **rhwinfo)
{
	tape_hwinfo_t *hwinfo;

	hwinfo = calloc(1, sizeof(tape_hwinfo_t));
	if (hwinfo == NULL)
		return ENOMEM;

	*rhwinfo = hwinfo;
	return 0;
}

/** Destroy tape hardware info.
 *
 * @param text Tape hardware info
 */
void tape_hwinfo_destroy(tape_hwinfo_t *hwinfo)
{
	if (hwinfo == NULL)
		return;

	if (link_used(&hwinfo->lhw_type))
		list_remove(&hwinfo->lhw_type);

	free(hwinfo);
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

	rc = tape_block_create(tb_hw_type, unknown, &block);
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

/** Get short text description of block type.
 *
 * @param btype Block type
 * @return Short string description
 */
const char *tape_btype_str(tape_btype_t btype)
{
	switch (btype) {
	case tb_data:
		return "Data";
	case tb_turbo_data:
		return "Turbo data";
	case tb_tone:
		return "Tone";
	case tb_pulses:
		return "Pulses";
	case tb_pure_data:
		return "Pure data";
	case tb_direct_rec:
		return "Direct recording";
	case tb_csw_rec:
		return "CSW recording";
	case tb_gen_data:
		return "Generalized data";
	case tb_pause:
		return "Pause";
	case tb_stop:
		return "Stop the tape";
	case tb_group_start:
		return "Group start";
	case tb_group_end:
		return "Group end";
	case tb_jump:
		return "Jump";
	case tb_loop_start:
		return "Loop start";
	case tb_loop_end:
		return "Loop end";
	case tb_call_seq:
		return "Call sequence";
	case tb_return:
		return "Return";
	case tb_select:
		return "Select";
	case tb_stop_48k:
		return "Stop if 48K";
	case tb_set_lvl:
		return "Set level";
	case tb_text_desc:
		return "Text description";
	case tb_message:
		return "Message";
	case tb_archive_info:
		return "Archive info";
	case tb_hw_type:
		return "Hardware type";
	case tb_custom_info:
		return "Custom info";
	case tb_glue:
		return "Glue";
	case tb_c64_rom_data:
		return "C64 ROM data";
	case tb_c64_turbo_data:
		return "C64 turbo data";
	case tb_emu_info:
		return "Emulation info";
	case tb_snapshot:
		return "Snapshot";
	case tb_unknown:
		return "Unknown";
	}

	assert(false);
	return "Invalid";
}
