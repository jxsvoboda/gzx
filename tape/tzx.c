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
	case tb_archive_info:
		return tzxb_archive_info;
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

	printf("load standard speed data block\n");

	nread = fread(&block, 1, sizeof(tzx_block_data_t), f);
	if (nread != sizeof(tzx_block_data_t))
		return EIO;

	rc = tblock_data_create(&data);
	if (rc != 0)
		goto error;

	data->pause_after = uint16_t_le2host(block.pause_after);
	data->data_len = uint16_t_le2host(block.data_len);

	data->data = calloc(block.data_len, 1);
	if (data->data == NULL) {
		rc = ENOMEM;
		goto error;
	}

	printf("pause after:%u\n", (unsigned) data->pause_after);
	printf("data len:%u\n", (unsigned) data->data_len);

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

	printf("save standard speed data block\n");

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

	printf("load text\n");

	if (*bremain < sizeof(tzx_text_t)) {
		rc = EIO;
		goto error;
	}

	nread = fread(&tzxtext, 1, sizeof(tzx_text_t), f);
	if (nread != sizeof(tzx_text_t)) {
		rc = EIO;
		goto error;
	}

	printf("text type:0x%x length=%u\n", tzxtext.text_type,
	    tzxtext.text_len);
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

	printf("save text\n");

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

	printf("load archive info\n");
	nread = fread(&block, 1, sizeof(tzx_block_archive_info_t), f);
	if (nread != sizeof(tzx_block_archive_info_t))
		return EIO;

	blen = uint16_t_le2host(block.block_len);
	printf("block size:%u\n", (unsigned) blen);
	printf("size of block header:%u\n", (unsigned) sizeof(tzx_block_archive_info_t));
	printf("# of strings:%u\n", (unsigned) block.nstrings);

	rc = tblock_archive_info_create(&ainfo);
	if (rc != 0)
		goto error;

	bremain = blen;

	for (i = 0; i < block.nstrings; i++) {
		printf("loading string %d\n", i);
		rc = tzx_load_text(f, &bremain, &ttext);
		if (rc != 0)
			goto error;

		printf("text is %d/'%s'\n", ttext->text_type, ttext->text);
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

	printf("save archive info\n");
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

	printf("block size: %u\n", (unsigned)bsize);
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

	printf("save unknown block\n");

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

	printf("validate header\n");
	rc = tzx_header_validate(&header);
	if (rc != 0) {
		rc = EIO;
		goto error;
	}

	tape->version.major = header.major;
	tape->version.minor = header.minor;

	printf("read blocks\n");
	while (true) {
		printf("read block type\n");
		/* Read block type */
		nread = fread(&btype, 1, sizeof(uint8_t), f);
		if (nread != sizeof(uint8_t)) {
			if (feof(f))
				break;

			rc = EIO;
			goto error;
		}

		printf("process block\n");
		switch (btype) {
		case tzxb_data:
			rc = tzx_load_data(f, tape);
			break;
		case tzxb_archive_info:
			rc = tzx_load_archive_info(f, tape);
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
		case tb_archive_info:
			rc = tzx_save_archive_info((tblock_archive_info_t *)
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
