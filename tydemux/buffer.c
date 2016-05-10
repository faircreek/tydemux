/*
 * Copyright (C) 2002, 2003, 2015  Olof <jinxolina@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "common.h"
#include "global.h"


int find_start_code_offset_in_buffer(const start_codes_t start_code_type, int size, uint8_t * data_buffer,
		 tystream_holder_t * tystream) {

	/* Iteration */
	int k;

	/* Makrers */
	int got_offset;

	/* Sizes and offsets */
	int offset;

	/* Pointer to magic with */
	uint8_t * pt1;

	/* FIXME The start code */
	uint8_t * start_code;

	/* Init */
	start_code = tystream->start_code_array[start_code_type].start_code;
	got_offset = 0;
	offset = -1;

	pt1 = data_buffer;

	/* FIXME need to check if we should have start code size or
	(start code size + 1) - size is the size of the data_buffer*/
	for(k=0; k < size - (START_CODE_SIZE + 1); k++, pt1++) {
		if (memcmp(pt1, start_code, START_CODE_SIZE) == 0) {
			offset = pt1 - data_buffer;
			got_offset = 1;
		}
		if(got_offset) {
			break;
		}
	}

	if(!got_offset) {
		return(-1);
	}

	return(offset);
}

int find_extension_in_payload(tystream_holder_t * tystream, payload_t * payload, uint8_t extension) {

	/* Sizes and offsets */
	int buffer_size;
	int offset;

	/* Buffer */
	uint8_t * buffer;

	/* Pointers to do magic with */
	uint8_t * pt1;

	/* Extensions */
	uint8_t found_extension;

	/* Init */
	buffer = payload->payload_data;
	buffer_size = payload->size;
	offset = 0;

	pt1 = buffer;

	while(offset != -1) {

		offset = find_start_code_offset_in_buffer(MPEG_EXT, buffer_size, pt1, tystream);
		LOG_DEVDIAG1("Offset was %i \n", offset);
		if(offset != -1) {
			/* Okay lets find out if this is the right extension */
			found_extension = pt1[offset + 4] >> 4;
			LOG_DEVDIAG1("Found extension %02x \n", found_extension);
			if(found_extension == extension) {
				/* It was return the offset */
				return(offset);
			} else {
				pt1 = pt1 + offset + 1;
				buffer_size = buffer_size - ((pt1 - buffer) -1);
				LOG_DEVDIAG1("Not this one %i\n", found_extension);
			}
		} else {
			LOG_DEVDIAG("Offset was -1\n");
			break;
		}
	}
	LOG_DEVDIAG("Offset not found\n");
	return(-1);
}


