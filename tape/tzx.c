/*
 * GZX - George's ZX Spectrum Emulator
 * TZX file format support
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
 * @file TZX file format support
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../adt/list.h"
#include "../types/tape/tzx.h"
#include "../byteorder.h"
#include "tape.h"
#include "tzx.h"

const char *tzx_signature = "ZXTape!";
const char tzx_eof = 0x1a;

/** Get TZX block type.
 *
 * @param block Tape block
 * @return TZX block type
 */
static uint8_t tzx_block_type(tape_block_t *block)
{
	tblock_unknown_t *unknown;

	switch (block->btype) {
	case tb_data:
		return tzxb_data;
	case tb_turbo_data:
		return tzxb_turbo_data;
	case tb_pause:
		return tzxb_pause_stop;
	case tb_group_start:
		return tzxb_group_start;
	case tb_group_end:
		return tzxb_group_end;
	case tb_stop:
		return tzxb_pause_stop;
	case tb_text_desc:
		return tzxb_text_desc;
	case tb_archive_info:
		return tzxb_archive_info;
	case tb_hw_type:
		return tzxb_hw_type;
	case tb_unknown:
		unknown = (tblock_unknown_t *) block->ext;
		return unknown->block_type;
	default:
		assert(false);
		return 0;
	}
}

/** Validate TZX header.
 *
 * @param header Header
 * @return Zero if header is valid or error code
 */
static int tzx_header_validate(tzx_header_t *header)
{
	if (memcmp(header->signature, tzx_signature,
	    sizeof(header->signature)) != 0) {
		return EINVAL;
	}

	if (header->eof_mark != tzx_eof)
		return EINVAL;

	if (header->major != 1)
		return EINVAL;

	return 0;
}

/** Initialize TZX header.
 *
 * @param tape Tape which we are saving
 * @param header Header to fill in
 */
static void tzx_header_init(tape_t *tape, tzx_header_t *header)
{
	memcpy(header->signature, tzx_signature, sizeof(header->signature));
	header->eof_mark = tzx_eof;

	header->major = tape->version.major;
	header->minor = tape->version.minor;
}

/** Load standard speed data block.
 *
 * @param f File to read from
 * @param tape to add block to
 * @return Zero on success or error code
 */
static int tzx_load_data(FILE *f, tape_t *tape)
{
	tzx_block_data_t block;
	tblock_data_t *data;
	size_t nread;
	int rc;

	nread = fread(&block, 1, sizeof(tzx_block_data_t), f);
	if (nread != sizeof(tzx_block_data_t))
		return EIO;

	rc = tblock_data_create(&data);
	if (rc != 0)
		goto error;

	data->pause_after = uint16_t_le2host(block.pause_after);
	data->data_len = uint16_t_le2host(block.data_len);

	data->data = calloc(data->data_len, 1);
	if (data->data == NULL) {
		rc = ENOMEM;
		goto error;
	}

	nread = fread(data->data, 1, data->data_len, f);
	if (nread != data->data_len) {
		rc = EIO;
		goto error;
	}

	tape_append(tape, data->block);
	return 0;
error:
	if (data != NULL)
		tblock_data_destroy(data);
	return rc;
}

/** Save standard speed data block.
 *
 * @param data Standard speed data
 * @param f File to write to
 * @return Zero on success or error code
 */
static int tzx_save_data(tblock_data_t *data, FILE *f)
{
	tzx_block_data_t block;
	size_t nwr;

	block.pause_after = host2uint16_t_le(data->pause_after);
	block.data_len = host2uint16_t_le(data->data_len);

	nwr = fwrite(&block, 1, sizeof(tzx_block_data_t), f);
	if (nwr != sizeof(tzx_block_data_t))
		return EIO;

	nwr = fwrite(data->data, 1, data->data_len, f);
	if (nwr != data->data_len)
		return EIO;

	return 0;
}

/** Load turbo speed data block.
 *
 * @param f File to read from
 * @param tape to add block to
 * @return Zero on success or error code
 */
static int tzx_load_turbo_data(FILE *f, tape_t *tape)
{
	tzx_block_turbo_data_t block;
	tblock_turbo_data_t *tdata;
	size_t nread;
	int rc;

	nread = fread(&block, 1, sizeof(tzx_block_turbo_data_t), f);
	if (nread != sizeof(tzx_block_turbo_data_t))
		return EIO;

	rc = tblock_turbo_data_create(&tdata);
	if (rc != 0)
		goto error;

	tdata->pilot_len = uint16_t_le2host(block.pilot_len);
	tdata->sync1_len = uint16_t_le2host(block.sync1_len);
	tdata->sync2_len = uint16_t_le2host(block.sync2_len);
	tdata->zero_len = uint16_t_le2host(block.zero_len);
	tdata->one_len = uint16_t_le2host(block.one_len);
	tdata->pilot_pulses = uint16_t_le2host(block.pilot_pulses);
	tdata->lb_bits = block.lb_bits;

	tdata->pause_after = uint16_t_le2host(block.pause_after);
	tdata->data_len = block.data_len[0] +
	    ((uint32_t) block.data_len[1] << 8) +
	    ((uint32_t) block.data_len[2] << 16);

	tdata->data = calloc(tdata->data_len, 1);
	if (tdata->data == NULL) {
		rc = ENOMEM;
		goto error;
	}

	nread = fread(tdata->data, 1, tdata->data_len, f);
	if (nread != tdata->data_len) {
		rc = EIO;
		goto error;
	}

	tape_append(tape, tdata->block);
	return 0;
error:
	if (tdata != NULL)
		tblock_turbo_data_destroy(tdata);
	return rc;
}

/** Save turbo speed data block.
 *
 * @param tdata Turbo speed data
 * @param f File to write to
 * @return Zero on success or error code
 */
static int tzx_save_turbo_data(tblock_turbo_data_t *tdata, FILE *f)
{
	tzx_block_turbo_data_t block;
	size_t nwr;
	int i;

	block.pilot_len = host2uint16_t_le(tdata->pilot_len);
	block.sync1_len = host2uint16_t_le(tdata->sync1_len);
	block.sync2_len = host2uint16_t_le(tdata->sync2_len);
	block.zero_len = host2uint16_t_le(tdata->zero_len);
	block.one_len = host2uint16_t_le(tdata->one_len);
	block.pilot_pulses = host2uint16_t_le(tdata->pilot_pulses);
	block.lb_bits = tdata->lb_bits;

	block.pause_after = host2uint16_t_le(tdata->pause_after);
	for (i = 0; i < 3; i++)
		block.data_len[i] = (tdata->data_len >> (8 * i)) & 0xff;

	nwr = fwrite(&block, 1, sizeof(tzx_block_turbo_data_t), f);
	if (nwr != sizeof(tzx_block_turbo_data_t))
		return EIO;

	nwr = fwrite(tdata->data, 1, tdata->data_len, f);
	if (nwr != tdata->data_len)
		return EIO;

	return 0;
}

/** Load pause (silence) or 'Stop the Tape'.
 *
 * @param f File to read from
 * @param tape to add block to
 * @return Zero on success or error code
 */
static int tzx_load_pause_stop(FILE *f, tape_t *tape)
{
	tzx_block_pause_t block;
	tblock_pause_t *pause;
	tblock_stop_t *stop;
	uint16_t pause_len;
	size_t nread;
	int rc;

	nread = fread(&block, 1, sizeof(tzx_block_pause_t), f);
	if (nread != sizeof(tzx_block_pause_t))
		return EIO;

	pause_len = uint16_t_le2host(block.pause_len);
	if (pause_len != 0) {
		rc = tblock_pause_create(&pause);
		if (rc != 0)
			goto error;

		pause->pause_len = pause_len;
		tape_append(tape, pause->block);
	} else {
		rc = tblock_stop_create(&stop);
		if (rc != 0)
			goto error;

		tape_append(tape, stop->block);
	}

	return 0;
error:
	if (pause != NULL)
		tblock_pause_destroy(pause);
	if (stop != NULL)
		tblock_stop_destroy(stop);
	return rc;
}

/** Save pause (silence).
 *
 * @param data Standard speed data
 * @param f File to write to
 * @return Zero on success or error code
 */
static int tzx_save_pause(tblock_pause_t *pause, FILE *f)
{
	tzx_block_pause_t block;
	size_t nwr;

	block.pause_len = host2uint16_t_le(pause->pause_len);

	nwr = fwrite(&block, 1, sizeof(tzx_block_pause_t), f);
	if (nwr != sizeof(tzx_block_pause_t))
		return EIO;

	return 0;
}

/** Save 'Stop the tape'.
 *
 * @param stop 'Stop the tape'
 * @param f File to write to
 * @return Zero on success or error code
 */
static int tzx_save_stop(tblock_stop_t *stop, FILE *f)
{
	tzx_block_pause_t block;
	size_t nwr;

	block.pause_len = 0;

	nwr = fwrite(&block, 1, sizeof(tzx_block_pause_t), f);
	if (nwr != sizeof(tzx_block_pause_t))
		return EIO;

	return 0;
}

/** Load group start.
 *
 * @param f File to read from
 * @param tape to add block to
 * @return Zero on success or error code
 */
static int tzx_load_group_start(FILE *f, tape_t *tape)
{
	tzx_block_group_start_t block;
	tblock_group_start_t *gstart;
	size_t nread;
	int rc;

	nread = fread(&block, 1, sizeof(tzx_block_group_start_t), f);
	if (nread != sizeof(tzx_block_group_start_t))
		return EIO;

	rc = tblock_group_start_create(&gstart);
	if (rc != 0)
		goto error;

	gstart->name = malloc(block.name_len + 1);
	if (gstart->name == NULL) {
		rc = ENOMEM;
		goto error;
	}

	nread = fread(gstart->name, 1, block.name_len, f);
	if (nread != block.name_len)
		return EIO;

	gstart->name[block.name_len] = '\0';
	tape_append(tape, gstart->block);

	return 0;
error:
	if (gstart != NULL)
		tblock_group_start_destroy(gstart);
	return rc;
}

/** Save group start.
 *
 * @param gstart Group start
 * @param f File to write to
 * @return Zero on success or error code
 */
static int tzx_save_group_start(tblock_group_start_t *gstart, FILE *f)
{
	tzx_block_group_start_t block;
	size_t nwr;
	size_t name_len;

	name_len = strlen(gstart->name);
	if (name_len > 0xff)
		return EINVAL;

	block.name_len = name_len;;

	nwr = fwrite(&block, 1, sizeof(tzx_block_group_start_t), f);
	if (nwr != sizeof(tzx_block_group_start_t))
		return EIO;

	nwr = fwrite(gstart->name, 1, name_len, f);
	if (nwr != name_len)
		return EIO;

	return 0;
}

/** Load group end.
 *
 * @param f File to read from
 * @param tape to add block to
 * @return Zero on success or error code
 */
static int tzx_load_group_end(FILE *f, tape_t *tape)
{
	tblock_group_end_t *gend;
	int rc;

	/* This block has an empty body */

	rc = tblock_group_end_create(&gend);
	if (rc != 0)
		goto error;

	tape_append(tape, gend->block);
	return 0;
error:
	return rc;
}

/** Save group end.
 *
 * @param gend Group end
 * @param f File to write to
 * @return Zero on success or error code
 */
static int tzx_save_group_end(tblock_group_end_t *gend, FILE *f)
{
	/* This block has an empty body */
	return 0;
}

/** Load text description.
 *
 * @param f File to read from
 * @param tape to add block to
 * @return Zero on success or an error code
 */
static int tzx_load_text_desc(FILE *f, tape_t *tape)
{
	tblock_text_desc_t *tdesc = NULL;
	tzx_block_text_desc_t block;
	size_t nread;
	int rc;

	nread = fread(&block, 1, sizeof(tzx_block_text_desc_t), f);
	if (nread != sizeof(tzx_block_text_desc_t)) {
		rc = EIO;
		goto error;
	}

	rc = tblock_text_desc_create(&tdesc);
	if (rc != 0)
		goto error;

	tdesc->text = calloc(block.text_len + 1, 1);
	if (tdesc->text == NULL) {
		rc = ENOMEM;
		goto error;
	}

	nread = fread(tdesc->text, 1, block.text_len, f);
	if (nread != block.text_len)
		goto error;

	tdesc->text[block.text_len] = '\0';

	tape_append(tape, tdesc->block);
	return 0;
error:
	if (tdesc != NULL)
		tblock_text_desc_destroy(tdesc);
	return rc;
}

/** Save text description.
 *
 * @param tdesc Text description
 * @param f File to write to
 * @return Zero on success, EIO on I/O error, EINVAL if text is not valid
 */
static int tzx_save_text_desc(tblock_text_desc_t *tdesc, FILE *f)
{
	tzx_block_text_desc_t block;
	size_t nwr;
	size_t slen;

	slen = strlen(tdesc->text);
	if (slen > 0xff)
		return EINVAL;

	block.text_len = slen;

	nwr = fwrite(&block, 1, sizeof(tzx_block_text_desc_t), f);
	if (nwr != sizeof(tzx_block_text_desc_t))
		return EIO;

	nwr = fwrite(tdesc->text, 1, slen, f);
	if (nwr != slen)
		return EIO;

	return 0;
}

/** Load text structure.
 *
 * This is part of archive info.
 *
 * @param f File to read from
 * @param bremain Pointer to variable holding number of bytes remaining
 *                in archive info block. Will be updated on success.
 * @param rtext Place to store pointer to new tape text
 * @return Zero on success or an error code
 */
static int tzx_load_text(FILE *f, size_t *bremain, tape_text_t **rtext)
{
	tape_text_t *text = NULL;
	tzx_text_t tzxtext;
	size_t nread;
	int rc;

	if (*bremain < sizeof(tzx_text_t)) {
		rc = EIO;
		goto error;
	}

	nread = fread(&tzxtext, 1, sizeof(tzx_text_t), f);
	if (nread != sizeof(tzx_text_t)) {
		rc = EIO;
		goto error;
	}

	*bremain -= nread;

	rc = tape_text_create(&text);
	if (rc != 0)
		goto error;

	text->text = calloc(tzxtext.text_len + 1, 1);
	if (text->text == NULL) {
		rc = ENOMEM;
		goto error;
	}

	if (*bremain < tzxtext.text_len) {
		rc = EIO;
		goto error;
	}

	nread = fread(text->text, 1, tzxtext.text_len, f);
	if (nread != tzxtext.text_len)
		goto error;

	*bremain -= nread;

	text->text[tzxtext.text_len] = '\0';
	text->text_type = tzxtext.text_type;

	*rtext = text;
	return 0;
error:
	if (text != NULL)
		tape_text_destroy(text);
	return rc;
}

/** Save text structure.
 *
 * This is part of archive info.
 *
 * @param text Text
 * @param f File to write to
 * @return Zero on success, EIO on I/O error, EINVAL if text is not valid
 */
static int tzx_save_text(tape_text_t *text, FILE *f)
{
	tzx_text_t tzxtext;
	size_t nwr;
	size_t slen;

	slen = strlen(text->text);
	if (slen > 0xff)
		return EINVAL;

	tzxtext.text_type = text->text_type;
	tzxtext.text_len = slen;

	nwr = fwrite(&tzxtext, 1, sizeof(tzx_text_t), f);
	if (nwr != sizeof(tzx_text_t))
		return EIO;

	nwr = fwrite(text->text, 1, slen, f);
	if (nwr != slen)
		return EIO;

	return 0;
}

/** Load archive info.
 *
 * @param f File to read from
 * @param tape to add block to
 * @return Zero on success or error code
 */
static int tzx_load_archive_info(FILE *f, tape_t *tape)
{
	tzx_block_archive_info_t block;
	tblock_archive_info_t *ainfo;
	tape_text_t *ttext = NULL;
	size_t nread;
	size_t bremain;
	uint16_t blen;
	uint8_t i;
	int rc;

	nread = fread(&block, 1, sizeof(tzx_block_archive_info_t), f);
	if (nread != sizeof(tzx_block_archive_info_t))
		return EIO;

	blen = uint16_t_le2host(block.block_len);

	rc = tblock_archive_info_create(&ainfo);
	if (rc != 0)
		goto error;

	bremain = blen;

	for (i = 0; i < block.nstrings; i++) {
		rc = tzx_load_text(f, &bremain, &ttext);
		if (rc != 0)
			goto error;

		ttext->ainfo = ainfo;
		list_append(&ttext->lainfo, &ainfo->texts);
	}

	tape_append(tape, ainfo->block);

	return 0;
error:
	if (ainfo != NULL)
		tblock_archive_info_destroy(ainfo);
	return rc;
}

/** Save archive info.
 *
 * @param ainfo Archive info
 * @param f File to write to
 * @return Zero on success or error code
 */
static int tzx_save_archive_info(tblock_archive_info_t *ainfo, FILE *f)
{
	tzx_block_archive_info_t block;
	tape_text_t *text;
	size_t nwr;
	unsigned long ntexts;
	size_t bsize;
	size_t slen;
	int rc;

	ntexts = list_count(&ainfo->texts);
	if (ntexts > 0xff)
		return EINVAL;

	/* Compute total block size */
	bsize = sizeof(tzx_block_archive_info_t) - sizeof(uint16_t);
	text = tblock_archive_info_first(ainfo);
	while (text != NULL) {
		slen = strlen(text->text);
		/* Cannot encode string longer than 255 bytes */
		if (slen > 0xff)
			return EINVAL;
		bsize += sizeof(tzx_text_t) + slen;
		text = tblock_archive_info_next(text);
	}

	/* Cannot encode block larger than 64K-1 */
	if (bsize > 0xffff)
		return EINVAL;

	block.block_len = host2uint16_t_le(bsize);
	block.nstrings = ntexts;

	nwr = fwrite(&block, 1, sizeof(tzx_block_archive_info_t), f);
	if (nwr != sizeof(tzx_block_archive_info_t))
		return EIO;

	text = tblock_archive_info_first(ainfo);
	while (text != NULL) {
		rc = tzx_save_text(text, f);
		if (rc != 0)
			return rc;

		text = tblock_archive_info_next(text);
	}

	return 0;
}

/** Load hardware info.
 *
 * This is part of hardware type.
 *
 * @param f File to read from
 * @param rhwinfo Place to store pointer to new hardware info
 * @return Zero on success or an error code
 */
static int tzx_load_hwinfo(FILE *f, tape_hwinfo_t **rhwinfo)
{
	tape_hwinfo_t *hwinfo = NULL;
	tzx_hwinfo_t tzxhwinfo;
	size_t nread;
	int rc;

	printf("Load hardware info\n");
	nread = fread(&tzxhwinfo, 1, sizeof(tzx_hwinfo_t), f);
	if (nread != sizeof(tzx_hwinfo_t)) {
		rc = EIO;
		goto error;
	}

	rc = tape_hwinfo_create(&hwinfo);
	if (rc != 0)
		goto error;

	hwinfo->hwtype = tzxhwinfo.hw_type;
	hwinfo->hwid = tzxhwinfo.hw_id;
	hwinfo->hwinfo = tzxhwinfo.hw_info;

	*rhwinfo = hwinfo;
	return 0;
error:
	if (hwinfo != NULL)
		tape_hwinfo_destroy(hwinfo);
	return rc;
}

/** Save hardware info.
 *
 * This is part of hardware type.
 *
 * @param hwinfo Hardware info
 * @param f File to write to
 * @return Zero on success, EIO on I/O error, EINVAL if text is not valid
 */
static int tzx_save_hwinfo(tape_hwinfo_t *hwinfo, FILE *f)
{
	tzx_hwinfo_t tzxhwinfo;
	size_t nwr;

	tzxhwinfo.hw_type = hwinfo->hwtype;
	tzxhwinfo.hw_id = hwinfo->hwid;
	tzxhwinfo.hw_info = hwinfo->hwinfo;

	nwr = fwrite(&tzxhwinfo, 1, sizeof(tzx_hwinfo_t), f);
	if (nwr != sizeof(tzx_hwinfo_t))
		return EIO;

	return 0;
}

/** Load hardware type.
 *
 * @param f File to read from
 * @param tape to add block to
 * @return Zero on success or error code
 */
static int tzx_load_hw_type(FILE *f, tape_t *tape)
{
	tzx_block_hw_type_t block;
	tblock_hw_type_t *hwtype;
	tape_hwinfo_t *hwinfo = NULL;
	size_t nread;
	uint8_t i;
	int rc;

	nread = fread(&block, 1, sizeof(tzx_block_hw_type_t), f);
	if (nread != sizeof(tzx_block_hw_type_t))
		return EIO;

	rc = tblock_hw_type_create(&hwtype);
	if (rc != 0)
		goto error;

	for (i = 0; i < block.ninfos; i++) {
		rc = tzx_load_hwinfo(f, &hwinfo);
		if (rc != 0)
			goto error;

		hwinfo->hw_type = hwtype;
		list_append(&hwinfo->lhw_type, &hwtype->hwinfos);
	}

	tape_append(tape, hwtype->block);

	return 0;
error:
	if (hwtype != NULL)
		tblock_hw_type_destroy(hwtype);
	return rc;
}

/** Save hardware type.
 *
 * @param hwtype Hardware type
 * @param f File to write to
 * @return Zero on success or error code
 */
static int tzx_save_hw_type(tblock_hw_type_t *hwtype, FILE *f)
{
	tzx_block_hw_type_t block;
	tape_hwinfo_t *hwinfo;
	size_t nwr;
	unsigned long ninfos;
	int rc;

	ninfos = list_count(&hwtype->hwinfos);
	if (ninfos > 0xff)
		return EINVAL;

	block.ninfos = ninfos;

	nwr = fwrite(&block, 1, sizeof(tzx_block_hw_type_t), f);
	if (nwr != sizeof(tzx_block_hw_type_t))
		return EIO;

	hwinfo = tblock_hw_type_first(hwtype);
	while (hwinfo != NULL) {
		rc = tzx_save_hwinfo(hwinfo, f);
		if (rc != 0)
			return rc;

		hwinfo = tblock_hw_type_next(hwinfo);
	}

	return 0;
}

/** Load unknown block conforming to the extension rule.
 *
 * @param f File to read from
 * @param btype Unkown block type
 * @param tape to add block to
 * @return Zero on success or error code
 */
static int tzx_load_unknown(FILE *f, uint8_t btype, tape_t *tape)
{
	tzx_block_unknown_t block;
	tblock_unknown_t *unknown;
	size_t nread;
	uint32_t blen;
	void *data;
	int rc;

	printf("load unknown block (%02x)\n", btype);
	nread = fread(&block, 1, sizeof(tzx_block_unknown_t), f);
	if (nread != sizeof(tzx_block_unknown_t))
		return EIO;

	blen = uint32_t_le2host(block.block_len);
	printf("block size:%u\n", (unsigned) blen);
	data = malloc(blen);
	if (data == NULL)
		return ENOMEM;

	nread = fread(data, 1, blen, f);
	printf("bytes read: %u\n", (unsigned) nread);
	if (nread != blen) {
		rc = EIO;
		goto error;
	}

	rc = tblock_unknown_create(&unknown);
	if (rc != 0)
		goto error;

	unknown->block_type = btype;
	unknown->udata = data;
	unknown->block_len = blen;

	tape_append(tape, unknown->block);

	return 0;
error:
	free(data);
	return rc;
}

/** Save unknown block.
 *
 * @param unknown Unknown block
 * @param f File to write to
 * @return Zero on success or error code
 */
static int tzx_save_unknown(tblock_unknown_t *unknown, FILE *f)
{
	tzx_block_unknown_t block;
	size_t nwr;

	block.block_len = host2uint16_t_le(unknown->block_len);

	nwr = fwrite(&block, 1, sizeof(tzx_block_unknown_t), f);
	if (nwr != sizeof(tzx_block_unknown_t))
		return EIO;

	nwr = fwrite(unknown->udata, 1, unknown->block_len, f);
	if (nwr != unknown->block_len)
		return EIO;

	return 0;
}

/** Load tape from TZX file.
 *
 * @param fname File name
 * @param rtape Place to store pointer to loaded tape
 * @return Zero on success or error code
 */
int tzx_tape_load(const char *fname, tape_t **rtape)
{
	FILE *f;
	tzx_header_t header;
	size_t nread;
	uint8_t btype;
	tape_t *tape = NULL;
	int rc;

	f = fopen(fname, "rb");
	if (f == NULL)
		return ENOENT;

	rc = tape_create(&tape);
	if (rc != 0)
		goto error;

	/* Read TZX header */
	nread = fread(&header, 1, sizeof(tzx_header_t), f);
	if (nread != sizeof(tzx_header_t)) {
		rc = EIO;
		goto error;
	}

	rc = tzx_header_validate(&header);
	if (rc != 0) {
		rc = EIO;
		goto error;
	}

	tape->version.major = header.major;
	tape->version.minor = header.minor;

	while (true) {
		/* Read block type */
		nread = fread(&btype, 1, sizeof(uint8_t), f);
		if (nread != sizeof(uint8_t)) {
			if (feof(f))
				break;

			rc = EIO;
			goto error;
		}

		switch (btype) {
		case tzxb_data:
			rc = tzx_load_data(f, tape);
			break;
		case tzxb_turbo_data:
			rc = tzx_load_turbo_data(f, tape);
			break;
		case tzxb_pause_stop:
			rc = tzx_load_pause_stop(f, tape);
			break;
		case tzxb_group_start:
			rc = tzx_load_group_start(f, tape);
			break;
		case tzxb_group_end:
			rc = tzx_load_group_end(f, tape);
			break;
		case tzxb_text_desc:
			rc = tzx_load_text_desc(f, tape);
			break;
		case tzxb_archive_info:
			rc = tzx_load_archive_info(f, tape);
			break;
		case tzxb_hw_type:
			rc = tzx_load_hw_type(f, tape);
			break;
		default:
			rc = tzx_load_unknown(f, btype, tape);
			break;
		}

		if (rc != 0)
			goto error;
	}

	fclose(f);
	*rtape = tape;
	return 0;
error:
	if (tape != NULL)
		tape_destroy(tape);
	fclose(f);
	return rc;
}


/** Save tape to TZX file.
 *
 * @param tape Tape
 * @param fname File name
 * @return Zero on success or error code
 */
int tzx_tape_save(tape_t *tape, const char *fname)
{
	FILE *f;
	tzx_header_t header;
	size_t nwr;
	tape_block_t *block;
	uint8_t btype;
	int rc;
	int rv;

	f = fopen(fname, "wb");
	if (f == NULL)
		return ENOENT;

	/* Write TZX header */

	printf("write header\n");

	tzx_header_init(tape, &header);

	nwr = fwrite(&header, 1, sizeof(tzx_header_t), f);
	if (nwr != sizeof(tzx_header_t)) {
		rc = EIO;
		goto error;
	}

	printf("write blocks\n");
	block = tape_first(tape);
	while (block != NULL) {
		printf("write block type\n");

		/* Write block type */
		btype = tzx_block_type(block);
		nwr = fwrite(&btype, 1, sizeof(uint8_t), f);
		if (nwr != sizeof(uint8_t)) {
			rc = EIO;
			goto error;
		}

		printf("write block\n");
		switch (block->btype) {
		case tb_data:
			rc = tzx_save_data((tblock_data_t *) block->ext, f);
			break;
		case tb_turbo_data:
			rc = tzx_save_turbo_data(
			    (tblock_turbo_data_t *) block->ext, f);
			break;
		case tb_pause:
			rc = tzx_save_pause((tblock_pause_t *) block->ext, f);
			break;
		case tb_stop:
			rc = tzx_save_stop((tblock_stop_t *) block->ext, f);
			break;
		case tb_group_start:
			rc = tzx_save_group_start((tblock_group_start_t *)
			    block->ext, f);
			break;
		case tb_group_end:
			rc = tzx_save_group_end((tblock_group_end_t *)
			    block->ext, f);
			break;
		case tb_text_desc:
			rc = tzx_save_text_desc((tblock_text_desc_t *)
			    block->ext, f);
			break;
		case tb_archive_info:
			rc = tzx_save_archive_info((tblock_archive_info_t *)
			    block->ext, f);
			break;
		case tb_hw_type:
			rc = tzx_save_hw_type((tblock_hw_type_t *)
			    block->ext, f);
			break;
		default:
			rc = tzx_save_unknown((tblock_unknown_t *)
			    block->ext, f);
			break;
		}

		if (rc != 0)
			goto error;

		block = tape_next(block);
	}

	rv = fclose(f);
	if (rv < 0) {
		rc = EIO;
		goto error;
	}

	return 0;
error:
	fclose(f);
	return rc;
}
