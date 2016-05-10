
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

/* Prototypes for internal functions */

static int private_find_start_code_offset(int i, int first, const chunk_t * chunks,
				const start_codes_t start_code_type,
			   	int size, uint8_t **pdata_buffer, tystream_holder_t * tystream);

static int video_size_to_get(int i, last_record_info_t * info, const chunk_t * chunks,
				 tystream_holder_t * tystream);


static int audio_size_to_get(int i, last_record_info_t * info, const chunk_t * chunks,
				start_codes_t  module_type, tystream_holder_t * tystream);

static int get_pes_header_size(const uint8_t * pes_header);


/*************************************************************************************************/

int find_start_code_offset(int i, const chunk_t * chunks, const start_codes_t start_code_type,
			   int size, tystream_holder_t * tystream) {
	int retcode;

	/* Temp buffer */
	uint8_t * tmp_buffer = NULL;

	if(i <= chunks->nr_records) {
		/* Recurse until we find the offset */
		retcode = private_find_start_code_offset(i, 1, chunks, start_code_type, size, &tmp_buffer, tystream);
	} else {
		LOG_ERROR2("WE TRY TO GET A RECORD NOT PRESENT IN THE CHUNK %i - %i\n", i, chunks->nr_records);
		return(-1);
	}

	if( tmp_buffer ) {
		free(tmp_buffer);
	}

	return(retcode);
}




/*************************************************************************************************/



int get_video(int i, module_t * module, const chunk_t * chunks, start_codes_t  module_type, tystream_holder_t * tystream) {

	/* Iteration */
	//int j;

	/* Offset */
	int offset;

	/* Sizes */
	int size;
	int size_to_copy;
	int toget;
	int emb_pes_header_size;

	/* Info about the next record */
	int return_value;
	last_record_info_t last_record_info;


	/* Buffer for the pes header */
	uint8_t * data_buffer;
	uint8_t * tmp_data_buffer;

	/* Pointers to do magic with */
	uint8_t * pt1;

	tmp_data_buffer = NULL;

	size = chunks->record_header[i].size;

	offset = find_start_code_offset(i, chunks, module_type, 0, tystream);


	//printf("First offset %d, size of record %d - type %i\n", offset, size, module_type);

	if(offset == -1 || offset > size) {
		/* Error we didn't find the start */
		LOG_ERROR2("get_video_pes: Error - can't find start - chunk " I64FORMAT " - record %i\n", \
			chunks->chunk_number, i);
		return(0);
	}

	if(tystream->tivo_series == S2 && module_type == MPEG_PES_VIDEO) {
		/* Tivo S2 has the pes header embeded in SEQ, P/B-Frame records
		 * Hence if we call get_video to get a pes header then we should not extract
		 * the whole thing
		 * We know the offset to the start of the pes header we can extract the
		 * size of the pes header which is in byte 8 (nine bytes from the start).
		 */
		if(size - offset - 8 >= 0) {
			/* okay so we can get the size of the pes header */
			emb_pes_header_size = chunks->records[i].record[offset + 8] + 9;
		} else {
			LOG_ERROR2("get_video: Error - S2 embeded pes error - 1 - chunk " I64FORMAT ", record %i\n", chunks->chunk_number, i);
			return(0);
		}
		if(size - offset >= emb_pes_header_size) {
			/* Okay so we can now copy the actual data - i.e. the embeded pes header*/
			data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * emb_pes_header_size);
			memset(data_buffer, 0,sizeof(uint8_t) * emb_pes_header_size);
			pt1 = chunks->records[i].record + offset;
			memcpy(data_buffer,pt1, emb_pes_header_size);
			module->data_buffer = data_buffer;
			module->buffer_size = emb_pes_header_size;
			return(1);
		} else {
			LOG_ERROR2("get_video: Error - S2 embeded pes error - 2 - chunk " I64FORMAT ", record %i\n", chunks->chunk_number, i);
			return(0);
		}
	}


	/* Get the total size of the next records plus present record - offset we must have
	in order to extract this MPEG module */

	last_record_info.size = size - offset;

	/*printf("Size to get is %d\n",last_record_info.size);

	if(last_record_info.size < 0) {
		printf("This record in chunk " I64FORMAT "\n", chunks->chunk_number);
		for(j=0; j < chunks->record_header[i].size && j < 16; j++) {
			printf(" %02x", chunks->records[i].record[j]);
		}
		printf("\n");

		if(chunks->nr_records > i + 1) {
			printf("Next record type of record is %02x\n", chunks->record_header[i + 1].type);
			for(j=0; j < chunks->record_header[i + 1].size && j < 16; j++) {
				printf(" %02x", chunks->records[i + 1].record[j]);
			}
		}
		printf("\n");

		if(chunks->nr_records > i + 2) {
			printf("Next record type of record is %02x\n", chunks->record_header[i + 1].type);
			for(j=0; j < chunks->record_header[i + 2].size && j < 16; j++) {
				printf(" %02x", chunks->records[i + 2].record[j]);
			}
		}
		printf("\n");

		if(chunks->nr_records > i + 3) {
			printf("Next record type of record is %02x\n", chunks->record_header[i + 3].type);
			for(j=0; j < chunks->record_header[i + 3].size && j < 16; j++) {
				printf(" %02x", chunks->records[i + 3].record[j]);
			}
		}
		printf("\n");



	}*/

	/*Hmm this might also be buggy see offset check for offset bigger than size FIXME */

	return_value = video_size_to_get(i + 1, &last_record_info, chunks, tystream);

	if (return_value == -1) {
		/* Error no more packages */
		LOG_ERROR2("get_video_pes: Error - can't find get_size to get - chunk " I64FORMAT " - record %i\n", \
				chunks->chunk_number, i);
		return(0);
	}

	/* Copy this record into a buffer - starting at the start (offset) of the mpeg module */

	pt1 = chunks->records[i].record + offset;

	//printf("Chunk " I64FORMAT " Size to malloc %d - size %d - offset %d \n",chunks->chunk_number, size - offset, size, offset );

	tmp_data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * (size - offset));
	memset(tmp_data_buffer, 0,sizeof(uint8_t) * (size - offset));
	memcpy(tmp_data_buffer,pt1, size - offset);


	/* Now append get_size bytes from the next records into that buffer */
	toget = last_record_info.size;

	tmp_data_buffer = get_next_record(i+1, chunks, VIDEO, size - offset, tmp_data_buffer, toget - 1);

	if(!tmp_data_buffer) {
		LOG_ERROR2("get_video_pes: Error - can't get_next_record - chunk " I64FORMAT " - record %i\n", \
			chunks->chunk_number, i);
		return(0);
	}

	/* Now we need to know the size of the last record
	we copied to the tmp_buffer plus the offset of the
	start code in that buffer. When we know that we
	can copy "get_size + (size - offset) - (size_last_record - last_record_offset)"
	to the data_buffer which is what we will return */

	/* Okay let see how much we should copy */
	size_to_copy = last_record_info.size - last_record_info.last_record_size
			+ last_record_info.last_record_offset;

	/* Now malloc thats size and copy the data over */

	data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * size_to_copy);
	memset(data_buffer, 0,sizeof(uint8_t) * size_to_copy);
	memcpy(data_buffer, tmp_data_buffer, sizeof(uint8_t) * size_to_copy);

	/* Free the tmp_data_buffer and return the data_buffer which
	is the data of mpeg module we wanted */
	if(tmp_data_buffer) {
		free(tmp_data_buffer);
	}
	module->data_buffer = data_buffer;
	module->buffer_size = size_to_copy;
	return(1);
}


/*************************************************************************************************/

/* The main problem with audio is that there is really no good start codes as in video - well there is
but they are not that fun to play with. Hence we will only implement a get audio of type MPEG_PES_AUDIO and
AC3_PES_AUDIO.
We will "return" both the pes and the payload one as pes_data and the other one as just data */

int get_audio(int i, audio_module_t * module, const chunk_t * chunks,
	start_codes_t  module_type, tystream_holder_t * tystream) {



	/* Offset */
	int offset;

	/* Sizes */
	int size;
	int size_to_copy;
	int data_size_to_copy;
	int toget;
	uint16_t pes_say_size;
	int pes_header_size;


	/* Info about the next record */
	int return_value;
	last_record_info_t last_record_info;


	/* Buffer for the pes header and audio data*/
	uint8_t * data_buffer;
	uint8_t * pes_data_buffer;
	uint8_t * tmp_data_buffer;

	/* Pointers to do magic with */
	uint8_t * pt1;
	uint8_t * pt2;


	/* PES header to add to a DTIVO
	invalid DTivo MPEG pes header */
	uint8_t dtivo_pes[]= {0x84, 0x80, 0x05};

	size = chunks->record_header[i].size;

	offset = find_start_code_offset(i, chunks, module_type, 0, tystream);

	if(offset == -1 || offset > size ) {
		/* Error we didn't find the start */
		LOG_ERROR2("get_audio: Error - can't find start - chunk " I64FORMAT " - record %i\n", \
			chunks->chunk_number, i);
		return(0);
	}

	/* Get the total size of the next records plus present record - offset we must have
	in order to extract this MPEG module */
	last_record_info.size = size - offset;
	return_value = audio_size_to_get(i + 1, &last_record_info, chunks, module_type, tystream);

	if (return_value == -1) {
		/* Error no more packages */
		LOG_ERROR2("get_audio: Error - can't find get_size to get - chunk " I64FORMAT " - record %i\n", \
			chunks->chunk_number, i);
		return(0);
	}

	/* Copy this record into a buffer - starting at the start (offset) of the mpeg module */

	pt1 = chunks->records[i].record + offset;

	tmp_data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * (size - offset));
	memset(tmp_data_buffer, 0,sizeof(uint8_t) * (size - offset));
	memcpy(tmp_data_buffer,pt1, size - offset);

	/* Now append get_size bytes from the next records into that buffer */
	toget = last_record_info.size;
	tmp_data_buffer = get_next_record(i+1, chunks, AUDIO, size - offset, tmp_data_buffer, toget - 1);

	if(!tmp_data_buffer) {
		LOG_ERROR2("get_audio: Error - can't get_next_record - chunk " I64FORMAT " - record %i\n", \
			chunks->chunk_number, i);
		return(0);
	}

	/* Now we need to know the size of the last record
	we copied to the tmp_buffer plus the offset of the
	start code in that buffer. When we know that we
	can copy the "total size - the last records size +
	start offset to the last record
	to the data_buffer*/

	/* Okay let see how much we should copy */
	size_to_copy = last_record_info.size - last_record_info.last_record_size
			+ last_record_info.last_record_offset;
	LOG_DEVDIAG1("Size to copy is %i \n", size_to_copy);

	/* The problem is that we need to do this in two steps for audio
	one copy for the pes and one for the payload :(. To make things even
	worse the DTivo MPEG audio pes header is not a valid - correction
	it's a MPEG1 PES header - jack how ugly to mix mpeg one audio PES streams
	with MPEG2 video PES streams. It's only
	the start code size and the PTS present in it :( - hence 3 bytes is missing
	in PES header  -> no way to now the pes header size -> need
	to copy X a fixed value. However it's probably safe to bet that they
	will not change this behaviour since most DirectTV recivers will stop
	working*/

	if(tystream->DTIVO_MPEG == 3) {
		/* Okay DTivo invalid PES header :( */
		pes_data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * PES_MIN_PTS_SIZE);
		memset(pes_data_buffer, 0,sizeof(uint8_t) * PES_MIN_PTS_SIZE);
		/* Now copy the first 6 bytes -> fill in the missing part -> copy
		the remaining 5 bytes (PTS) -> voala we have a valid pes header :) */
		memcpy(pes_data_buffer,tmp_data_buffer, 6);
		pt1 = pes_data_buffer + 6;
		memcpy(pt1,dtivo_pes, 3);
		pt1 = pt1 + 3;
		pt2 = tmp_data_buffer + 6;
		memcpy(pt1,pt2, 5);

		/* We now have a slight problem we need to adjust the size field in
		the pes header it should be 3 bytes bigger */
		pes_say_size = (pes_data_buffer[4] << 8) + pes_data_buffer[5];
		pes_say_size = pes_say_size + 3;
		pes_data_buffer[4] = ((pes_say_size >> 8) & 0x00ff);
		pes_data_buffer[5] = pes_say_size & 0x00ff;

		/* The pes header is now 14 byes i,e, the min
		size for including a PTS time */
		pes_header_size = PES_MIN_PTS_SIZE;


	} else {

		pes_header_size = get_pes_header_size(tmp_data_buffer);

		pes_data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * pes_header_size);
		memset(pes_data_buffer, 0,sizeof(uint8_t) * pes_header_size);
		memcpy(pes_data_buffer,tmp_data_buffer, pes_header_size);
	}

	/* We need to include the tystream->DTIVO_MPEG even if we repaired the header */
	data_size_to_copy = size_to_copy - pes_header_size + tystream->DTIVO_MPEG;

	/* Now lets have a check function here to see if the size field in
	the PES header matches the size we are about to extract */

	pes_say_size = (pes_data_buffer[4] << 8) + pes_data_buffer[5];

	/* The pes size is all bytes in the pes after the size field
	this includes 3 manditory bytes plus what is in byte 8 in the pes header
	hence we need to deduct that size we only copy the data */

	pes_say_size = pes_say_size - 3 - pes_data_buffer[8];

	if(data_size_to_copy != pes_say_size) {
		LOG_WARNING2("get_audio: Warning! Audio size %i while pes says size %i \n", \
			data_size_to_copy, pes_say_size);
		/* FIXME we will need to add null bytes if it's too small
		or adjust size field if it's too big */

	}



	/* Now malloc data_size and copy the data over
	if pes size is smaller only copy that data */
	if(pes_say_size < data_size_to_copy) {
		data_size_to_copy = pes_say_size;
	}
	data_buffer = (uint8_t *)malloc(sizeof(uint8_t) * data_size_to_copy);
	memset(data_buffer, 0,sizeof(uint8_t) * data_size_to_copy);
	pt1 = tmp_data_buffer + pes_header_size - tystream->DTIVO_MPEG;
	memcpy(data_buffer, pt1, sizeof(uint8_t) * data_size_to_copy);


	/* Free the tmp_data_buffer and "return" the data_buffer and pes_buffer which
	is the data of mpeg module we wanted */
	if(tmp_data_buffer) {
		free(tmp_data_buffer);
	}
	module->data_buffer = data_buffer;
	module->data_buffer_size = data_size_to_copy;
	module->pes_data_buffer = pes_data_buffer;
	module->pes_buffer_size = pes_header_size;

	return(1);
}


/*************************************************************************************************/

uint8_t * get_next_record(int i, const chunk_t * chunks, media_type_t media,
	int size, uint8_t * data_buffer, int min_size) {

	/* Sizes */
	int tsize;

	/* Markers */
	int gotit;

	/* Pointer to do magic with */
	uint8_t * pt1;

	/* Set the default minimum size
	of the record*/
	if(min_size == 0) {
		min_size = 40;
	}

	gotit = 0;

	if(media == VIDEO) {

		/* Get next video record */

		for(; i < chunks->nr_records; i++) {
			switch(chunks->record_header[i].type){
				case 0x7e0:
				case 0x2e0:
				case 0x8e0:
				case 0xae0:
				case 0xbe0:
				case 0xce0:
				case 0x6e0:
					tsize = size;
					size = size + chunks->record_header[i].size;
					data_buffer = (uint8_t *)realloc(data_buffer, size * sizeof(uint8_t));
					pt1 = data_buffer + tsize;
					memcpy(pt1, chunks->records[i].record,
						sizeof(uint8_t) * chunks->record_header[i].size);
					if(size >= min_size) {
						gotit = 1;
					}
			}
			if(gotit == 1){
				break;
			}

		}

	} else if (media == AUDIO) {

		/* Get next audio record */
		for(; i < chunks->nr_records; i++) {
			switch(chunks->record_header[i].type){
				case 0x2c0:
				case 0x4c0:
				case 0x3c0:
				case 0x9c0:
					tsize = size;
					size = size + chunks->record_header[i].size;
					data_buffer = (uint8_t *)realloc(data_buffer, size * sizeof(uint8_t));
					pt1 = data_buffer + tsize;
					memcpy(pt1, chunks->records[i].record,
						sizeof(uint8_t) * chunks->record_header[i].size);
					if(size >= min_size) {
						gotit = 1;
					}
			}
			if(gotit == 1){
				break;
			}

		}
	} else {

		LOG_ERROR1("get_next_record: Not a video or audio type in chunk " I64FORMAT " - exit\n", \
			chunks->chunk_number);
		if(data_buffer) {
			free(data_buffer);
		}
		return(0);
	}

	if(gotit) {
		return(data_buffer);
	}

	if (chunks->next != NULL && !chunks->next->gap) {
		return(get_next_record(0, chunks->next, media, size, data_buffer, min_size));
	} /* else */

	if(data_buffer) {
		free(data_buffer);
	}
	return 0;

}

/***********************************************************************************/

static int private_find_start_code_offset(int i, int first, const chunk_t * chunks, const start_codes_t start_code_type,
			   int size, uint8_t **pdata_buffer, tystream_holder_t * tystream) {

	/* Interation */
	int k;

	/* Makrers */
	int gotit;
	int got_offset;
	int first_done;

	/* Sizes and offsets */
	int tsize;
	int offset;

	/* Pointer to magic with */
	uint8_t * pt1;

	/* FIXME The start code */
	uint8_t * start_code;

	start_code = tystream->start_code_array[start_code_type].start_code;

	gotit = 0;
	first_done = 0;
	got_offset = 0;

	if(i > chunks->nr_records) {
		LOG_ERROR2("WE TRY TO GET A RECORD NOT PRESENT IN THE CHUNK %i - %i\n", i, chunks->nr_records);
		return(-1);
	}


	/* Get the first record */
	if(first) {
		size = chunks->record_header[i].size;
		/* Do some sanity tests - OLAF I had a crash here when i > entries in record_header */
		if( size <= 0 || size > CHUNK_SIZE ) {
			LOG_ERROR1("WE TRY TO GET MORE DATA THAN PRESENT IN THE CHUNK %i \n", size);
			return( -1 );
		}
		*pdata_buffer = (uint8_t *)realloc(*pdata_buffer, size * sizeof(uint8_t));
		if( !pdata_buffer ) {
			return( -1 );
		}
		pt1 = *pdata_buffer;
		memcpy(pt1, chunks->records[i].record,
			sizeof(uint8_t) * chunks->record_header[i].size);
		first = 0;
		first_done = 1;
	}

	if(size < 40) {

		if(start_code_type < MPEG_PES_AUDIO) {

			/* Get next video record */

			for(i=i + first_done; i < chunks->nr_records; i++) {
				switch(chunks->record_header[i].type){
					case 0x7e0:
					case 0x2e0:
					case 0x8e0:
					case 0xae0:
					case 0xbe0:
					case 0xce0:
					case 0x6e0:
						tsize = size;
						size = size + chunks->record_header[i].size;
						*pdata_buffer = (uint8_t *)realloc(*pdata_buffer, size * sizeof(uint8_t));
						pt1 = (*pdata_buffer) + tsize;
						memcpy(pt1, chunks->records[i].record,
							sizeof(uint8_t) * chunks->record_header[i].size);
						if(size > 40) {
							gotit = 1;
						}
				}
				if(gotit == 1){
					break;
				}

			}

		} else if (start_code_type > MPEG_B) {

			/* Get next audio record */
			for(i=i + first_done; i < chunks->nr_records; i++) {
				switch(chunks->record_header[i].type){
					case 0x2c0:
					case 0x4c0:
					case 0x3c0:
					case 0x9c0:
						tsize = size;
						size = size + chunks->record_header[i].size;
						*pdata_buffer = (uint8_t *)realloc(*pdata_buffer, size * sizeof(uint8_t));
						pt1 = (*pdata_buffer) + tsize;
						memcpy(pt1, chunks->records[i].record,
							sizeof(uint8_t) * chunks->record_header[i].size);
						if(size > 40) {
							gotit = 1;
						}
				}
				if(gotit == 1){
					break;
				}
			}
		} else {

			LOG_ERROR1("find_start_code_offset: Not a video or audio type in chunk " I64FORMAT " - exit\n", \
				chunks->chunk_number);
			return(-1);
		}
	} else {
		gotit = 1;
	}


	if(gotit) {
		pt1 = *pdata_buffer;
		/* FIXME need to check if we should have start code size or
		(start code size + 1) - size is the size of the data_buffer*/
		for(k=0; k < size - (START_CODE_SIZE + 1); k++, pt1++) {
			if (memcmp(pt1, start_code, START_CODE_SIZE) == 0) {
				offset = pt1 - (*pdata_buffer);
				got_offset = 1;
			}
			if(got_offset) {
				break;
			}
		}
	}


	if(got_offset) {
		return(offset);
	} else if (chunks->next != NULL && gotit != 1) {
		/* If we have a gap/hole in the stream
		don't even try to find the offset
		since we don't know what will happen */
		if(chunks->next->gap) {
			return(-1);
		} else {
			/* Recurse until we find the offset */
			return(private_find_start_code_offset(0, first, chunks->next, start_code_type, size, pdata_buffer, tystream));
		}
	} else {
		LOG_ERROR2("find_start_code_offset: Can't find start_code %i in chunk " I64FORMAT " - exit\n", \
			start_code_type, chunks->chunk_number);

		return(-1);
	}
}

/**************************************************************************************************/

static int video_size_to_get(int i, last_record_info_t * info, const chunk_t * chunks, tystream_holder_t * tystream) {

	/* Marker */
	int gotit;
	int offset;

	gotit = 0;

	for(; i < chunks->nr_records; i++) {
		switch(chunks->record_header[i].type){
			/* Lots of code duplication but I don't want a goto statement */
			case 0x2e0:
				info->size = info->size + chunks->record_header[i].size;
				break;

			case 0x6e0:
				offset = find_start_code_offset(i, chunks, MPEG_PES_VIDEO, 0, tystream);
				gotit = 1;
				break;

			case 0x7e0:
				if(tystream->tivo_series == S1) {
					offset = find_start_code_offset(i, chunks, MPEG_SEQ, 0, tystream);
				} else {
					/* Tivo series 2 - pes header embeded in SEQ records */
					offset = find_start_code_offset(i, chunks, MPEG_PES_VIDEO, 0, tystream);
				}
				gotit = 1;
				break;

			case 0x8e0:
				offset = find_start_code_offset(i, chunks, MPEG_I, 0, tystream);
				gotit = 1;
				break;

			case 0xae0:
				if(tystream->tivo_series == S1) {
					offset = find_start_code_offset(i, chunks, MPEG_P, 0, tystream);
				} else {
					/* Tivo series 2 - pes header embeded in P-Frame records */
					offset = find_start_code_offset(i, chunks, MPEG_PES_VIDEO, 0, tystream);
				}
				gotit = 1;
				break;

			case 0xbe0:
				if(tystream->tivo_series == S1) {
					offset = find_start_code_offset(i, chunks, MPEG_B, 0, tystream);
				} else {
					/* Tivo series 2 - pes header embeded in B-Frame records */
					offset = find_start_code_offset(i, chunks, MPEG_PES_VIDEO, 0, tystream);
				}
				gotit = 1;
				break;

			case 0xce0:
				offset = find_start_code_offset(i, chunks, MPEG_GOP, 0, tystream);
				gotit = 1;
				break;
		}
		if(gotit) {
			info->size = info->size + chunks->record_header[i].size;
			if(offset == -1 || offset > (int)chunks->record_header[i].size ) {
				//printf("Offset %i, size %i\n", offset,(int)chunks->record_header[i].size );
				LOG_ERROR2("size_to_get: No offset last record - chunk " I64FORMAT " - record %i\n", \
					chunks->chunk_number, i);
				return(-1);
			} else {
				info->last_record_offset = offset;
			}
			info->last_record_size = chunks->record_header[i].size;
			break;
		}
	}

	if(gotit) {
		return(1);
	}

	if (chunks->next != NULL) {

		if(!chunks->next->gap) {
			return(video_size_to_get(0, info, chunks->next, tystream));
		}

		/* No use trying to get the next record size since
		we have a gap */
	} else {
		LOG_DEVDIAG1("The next chunk is null - chunk " I64FORMAT " ??\n", chunks->chunk_number);
		/* No more chunks - error */
	} /* i.e. return -1 if NULL or gap */
	return(-1);

}

/*******************************************************************************************/

static int get_pes_header_size(const uint8_t * pes_header) {

		/* All PES headers are 9 bytes plus
		what is in byte 8 */
		return(pes_header[8] + 9);
}

/*********************************************************************************************/

static int audio_size_to_get(int i, last_record_info_t * info, const chunk_t * chunks, start_codes_t  module_type, tystream_holder_t * tystream) {

	/* Marker */
	int gotit;
	int offset;

	gotit = 0;

	for(; i < chunks->nr_records; i++) {
		switch(chunks->record_header[i].type){
			/* Lots of code duplication but I don't want a goto statement */
			case 0x2c0:
			case 0x4c0:
				info->size = info->size + chunks->record_header[i].size;
				break;
			case 0x3c0:
			case 0x9c0:
				info->size = info->size + chunks->record_header[i].size;
				offset = find_start_code_offset(i, chunks, module_type, 0, tystream);
				if(offset == -1 || offset > (int)chunks->record_header[i].size ) {
					LOG_WARNING2("size_to_get: No offset last record - chunk " I64FORMAT " - record %i\n", \
						chunks->chunk_number, i);
					return(-1);
				} else {
					info->last_record_offset = offset;
				}
				info->last_record_size = chunks->record_header[i].size;
				gotit = 1;

		}
		if(gotit) {
			break;
		}
	}

	if(gotit) {
		return(1);
	}

	if (chunks->next != NULL && !chunks->next->gap) {
		return(audio_size_to_get(0, info, chunks->next, module_type, tystream));
	} /* else */
	return(-1);

}
