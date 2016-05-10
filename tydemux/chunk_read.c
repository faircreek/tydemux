
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

/* Internal functions */
//static record_header_t * read_record(chunk_t * chunk, int in_file);

int get_chunk(tystream_holder_t * tystream, chunknumber_t chunk_nr, int *progress_monitor) {

	chunk_t * chunk;
#if !defined(TIVO)
	vstream_t * vstream;
	uint8_t * buffer;
	int using_vstream;
#endif
	int cutpoint_result;
#if DEBUGTHREAD
	printf("Reading chunk\n");
#endif
	if( progress_monitor ) {
		*progress_monitor = (int) chunk_nr%99;
	}

	/* Check if we are in the start or the end of chunk reading */
	cutpoint_result = cutpoint_incut_chunk(tystream, chunk_nr);
	if( cutpoint_result == 4) {
		/* Okay even if we can return here without
		reading anything it works porly for a stream
		we simply need to read the data and just dump it*/
#if !defined(TIVO)
		if(tystream->input_pipe && !tystream->remote_holder) {
			buffer = (uint8_t *)malloc(sizeof(uint8_t) * CHUNK_SIZE);
			memset(buffer, 0, sizeof(uint8_t) * CHUNK_SIZE);
			if(thread_pipe_read(tystream->input_pipe, CHUNK_SIZE, buffer) != CHUNK_SIZE) {
				free(buffer);
#if DEBUGTHREAD
				printf("End of stream returning -1 \n");
#endif
				return(-1);
			}
			free(buffer);
		}
#endif
		return(0);
	} else if (cutpoint_result == 5) {
		/* End of stream return -1 */
		return(-1);
	}



#if !defined(TIVO)
	using_vstream = 0;
	if((tystream->input_pipe ||tystream->remote_holder) && tystream->vstream == NULL) {
		using_vstream = 1;
		if(!tystream->remote_holder) {
			buffer = (uint8_t *)malloc(sizeof(uint8_t) * CHUNK_SIZE);
			memset(buffer, 0, sizeof(uint8_t) * CHUNK_SIZE);
			vstream = new_vstream();
			if(thread_pipe_read(tystream->input_pipe, CHUNK_SIZE, buffer) != CHUNK_SIZE) {
				/* We must be at the end of file - return for ever */
				free(buffer);
#if DEBUGTHREAD
				printf("End of stream returning -1 \n");
#endif
				return(-1);
			}
			vstream->size = CHUNK_SIZE;
			vstream->start_stream = vstream->current_pos = buffer;
			vstream->end_stream = vstream->start_stream + CHUNK_SIZE;
			tystream->vstream = vstream;

		} else {
			/* We are reading remotly */
			if(chunk_nr < tystream->nr_of_remote_chunks - (int64_t)1) {
				//printf("Reading remote chunk\n");
				vstream = tydemux_get_remote_chunk(tystream->remote_holder, tystream, chunk_nr);
				if(!vstream) {
					/* Faild to get the remote chunk */
					return(-1);
				}
				tystream->vstream = vstream;
			} else {
				/* Reading out of bounderies */
				return(-1);
			}
		}
	}
#endif

	chunk = read_chunk(tystream, chunk_nr, 1);
#if DEBUGTHREAD
	printf("Done reading chunk\n");
#endif
	if(chunk){

		chunk->next     = NULL;
		chunk->previous = NULL;

		chunk = check_chunk(tystream, chunk);
		if(chunk) {

			LOG_DEVDIAG("Adding chunk\n");

			tystream->chunks = add_chunk(tystream, chunk, tystream->chunks);

			LOG_DEVDIAG1("Number of chunks is %i\n", nr_chunks_f(tystream->chunks));
#if !defined(TIVO)
			if(using_vstream) {
				//free(buffer);
				free_vstream(vstream);
				tystream->vstream = NULL;
			}
#endif
			return(1);
		}
	}

#if !defined(TIVO)
	if(using_vstream) {
		//free(buffer);
		free_vstream(vstream);
		tystream->vstream = NULL;
	}
#endif

	return(0);

}


int adj_byte_offset(tystream_holder_t * tystream) {

#ifdef TIVO
	return(0);
#else

	uint8_t * junk_buffer;



	if(tystream->std_alone) {
		return(0);
	}

	if(!tystream->byte_offset) {
		return(1);
	}

	junk_buffer = (uint8_t *)malloc( (size_t)(sizeof(uint8_t) * tystream->byte_offset) );
	memset(junk_buffer, 0 , (size_t)(sizeof(uint8_t) * tystream->byte_offset));

	if( tystream->byte_offset != (unsigned long)thread_pipe_read(tystream->input_pipe,(unsigned long)tystream->byte_offset,junk_buffer)){
		return(0);
	}
	tystream->byte_offset = 0;
	free(junk_buffer);

	return(1);
#endif

}



chunk_t * read_chunk(tystream_holder_t * tystream, chunknumber_t chunk_nr, int max_probe) {

	/* Interation */
	int i;

	/* The data part of the chunk */
	uint8_t * in_records;

	/* Chunk header 2.x and up*/
	uint8_t records_nr_1;
	uint8_t records_nr_2;
	uint8_t seq_start_1;
	uint8_t seq_start_2;
	uint16_t records_nr;
	uint16_t seq_start;
	uint16_t test_start;


	/* Chunk related */
	record_header_t * record_header;
	chunk_t * chunk;
	record_t * records;


	/* Vstream */
	vstream_t * vstream=NULL;

	/* Variables for seeking
	the start of the chunk */
	int max_seek;
	off64_t present_location;
	off64_t seek_pos;
	off64_t byte_offset;

	int total_record_size;
	int bad_chunk;

	if(tystream->tivo_version == UNDEFINED_VERSION) {
		return(0);
	} else if (tystream->tivo_version == V_13) {
		return(0);
	}

	vstream = tystream->vstream;


	/* Init chunk */

	byte_offset = tystream->byte_offset;

#if !defined(TIVO)
	if(!tystream->input_pipe && !tystream->remote_holder) {
#endif
		seek_pos = chunk_nr + tystream->start_chunk;
		seek_pos *= CHUNK_SIZE;
		seek_pos = seek_pos + byte_offset;
#if !defined(TIVO)
	} else {
		seek_pos = 0;
	}
#endif

#ifdef TIVOSERVER
	seek_pos = 0;
#endif

	//printf("Seek pos is " I64FORMAT "\n", seek_pos);

	if(!vstream) {
#ifdef WIN32
		if( lseek64(tystream->in_file, seek_pos, SEEK_SET) == -1L ) {
			LOG_ERROR("Premature end of file detected\n");
			return(0);  // Seek failed
		}
#else
		if( lseek64(tystream->in_file, seek_pos, SEEK_SET) <= -1 ) {
			LOG_ERROR("File seek failed\n");
			return(0);  // Seek failed
		}
#endif
	} else {
		if(tydemux_seek_vstream(vstream, (int) seek_pos, SEEK_SET) == -1) {
			LOG_ERROR("File seek failed\n");
			return(0);  // Seek failed
		}
	}



	if(!vstream) {
		present_location = lseek64(tystream->in_file, (off64_t)0 , SEEK_CUR);
	} else {
		present_location = tydemux_tell_vstream(vstream);
	}


	max_seek = 0;

	while(max_seek < max_probe) {

		chunk = (chunk_t *)malloc(sizeof(chunk_t));
		memset(chunk, 0,sizeof(chunk_t));
		if(!vstream) {
			lseek64(tystream->in_file, present_location + byte_offset, SEEK_SET);
		} else {
			tydemux_seek_vstream(vstream, (int)(present_location + byte_offset), SEEK_SET);
		}

		byte_offset++;
		max_seek++;


		/* Tivo version 2.x and higher */
		if(!vstream) {
			read(tystream->in_file, &records_nr_1, sizeof(uint8_t));
			read(tystream->in_file, &records_nr_2, sizeof(uint8_t));
			read(tystream->in_file, &seq_start_1, sizeof(uint8_t));
			read(tystream->in_file, &seq_start_2, sizeof(uint8_t));
		} else {
			tydemux_read_vstream(&records_nr_1, sizeof(uint8_t), 1, vstream);
			tydemux_read_vstream(&records_nr_2, sizeof(uint8_t), 1, vstream);
			tydemux_read_vstream(&seq_start_1, sizeof(uint8_t), 1, vstream);
			tydemux_read_vstream(&seq_start_2, sizeof(uint8_t), 1, vstream);
		}


		records_nr = ((uint16_t)records_nr_2 << 8) + records_nr_1;
		seq_start = ((uint16_t)seq_start_2 << 8) + seq_start_1;
		test_start = 0x8000;
		test_start = seq_start & 0x8000;
		if(test_start != 0x8000) {
			if(max_probe == 1) {
				LOG_WARNING1("Read Chunk: chunk " I64FORMAT " - bad chunk header - skipping\n", chunk_nr);
			}
			free(chunk);
			chunk=NULL;
			continue;
		}

		seq_start = seq_start & 0x7fff;

		if (seq_start != 0x7fff) {
			chunk->seq_present = 1;
			chunk->seq_start = seq_start;
		} else {
			chunk->seq_present = 0;
			chunk->seq_start = 0;
		}
		LOG_DEVDIAG2("Seq present %i , seq_start %i\n", chunk->seq_present, chunk->seq_start);

		chunk->nr_records = records_nr;
		chunk->chunk_number = chunk_nr;

		if(chunk->nr_records > MAX_RECORDS) {
			if(max_probe == 1) {
				LOG_WARNING1("Read Chunk: chunk " I64FORMAT " - over flow in records - skipping\n", chunk_nr);
			}
			free(chunk);
			chunk=NULL;
			continue;
		}


		chunk->junk = 0;
		chunk->gap = 0;
		chunk->small_gap=0;
		chunk->med_ticks = 0;
		chunk->last_tick = 0;
		chunk->first_tick = 0;
		chunk->indicator = POSITIVE;

		chunk->next = NULL;
		chunk->previous=NULL;


		LOG_DEVDIAG2("Read Chunk: chunk " I64FORMAT " nr_records: %i\n",chunk->chunk_number, chunk->nr_records );

		if(chunk->nr_records == 0) {
			if(max_probe == 1) {
				LOG_WARNING1("Read Chunk: chunk " I64FORMAT " - chunk has no records - skipping\n", chunk_nr);
			}
			free(chunk);
			record_header = NULL;
			chunk=NULL;
			continue;
		}

		record_header = read_record(chunk, tystream->in_file, vstream);
		chunk->record_header = record_header;


		total_record_size = 0;

		for(i=0; i < chunk->nr_records; i++) {
			total_record_size = total_record_size + record_header[i].size;
		}

		/* Check is the chunk is too big */
		if((total_record_size + (chunk->nr_records * 16) + 4) > CHUNK_SIZE) {
			if(max_probe == 1) {
				LOG_WARNING1("Read Chunk: chunk " I64FORMAT " - chunk is to big - skipping\n", chunk_nr);
			}
			for(i=0; i < chunk->nr_records; i++) {
				if(record_header[i].extended_data != NULL) {
					free(record_header[i].extended_data);
				}
			}
			free(record_header);
			free(chunk);
			record_header = NULL;
			chunk=NULL;
			continue;
		}

		/* Check if chunk is to small */
		if((total_record_size + (chunk->nr_records * 16) + 4) < CHUNK_SIZE/10) {
			if(max_probe == 1) {
				LOG_WARNING1("Read Chunk: chunk " I64FORMAT " - chunk is to small - skipping\n", chunk_nr);
			}
			for(i=0; i < chunk->nr_records; i++) {
				if(record_header[i].extended_data != NULL) {
					free(record_header[i].extended_data);
				}
			}
			free(record_header);
			free(chunk);
			record_header = NULL;
			chunk=NULL;
			continue;
		}


		bad_chunk = 0;

		/**
		* Junk record type check
		*
		* I have found that we sometimes have junk records types
		* in the chunk if we have a junk chunk.
		*/



		for(i=0; i < chunk->nr_records; i++){
			switch(chunk->record_header[i].type) {
				case 0x000: /* Good knows what this record is*/
				case 0x2c0: /* Audio Data */
				case 0x3c0: /* Audio PES - MPEG */
				case 0x4c0: /* Audio Data */
				case 0x9c0: /* Audio PES - AC3 */
				case 0x2e0: /* Video Data */
				case 0x6e0: /* Video PES */
				case 0x7e0: /* Video SEQ */
				case 0x8e0: /* Video I Frame/Field */
				case 0xae0: /* Video P Frame/Field */
				case 0xbe0: /* Video B Frame/Field */
				case 0xce0: /* Video GOP */
				case 0xe01: /* CC  (ClosedCaptions) */
				case 0xe02: /* XDS (Extended Data Service)*/
				case 0xe03: /* Ipreview */
				case 0xe05: /* TT (TeleText) ??*/
				case 0xe04:  /* ?? UK record ?? */
				case 0xe06:
					LOG_DEVDIAG3("Read Chunk: chunk " I64FORMAT " record: %i - junk record type: %03x\n", \
						chunk->chunk_number, i, chunk->record_header[i].type);
					break;
				default:
					if(max_probe == 1) {
						LOG_WARNING3("Read Chunk: chunk " I64FORMAT " record: %i - junk record type: %03x  - skipping\n", \
							chunk->chunk_number, i, chunk->record_header[i].type);
					}
					bad_chunk = 1;
					break;

			}
		}


		if(bad_chunk) {
			for(i=0; i < chunk->nr_records; i++) {
				if(record_header[i].extended_data != NULL) {
					free(record_header[i].extended_data);
				}
			}
			free(record_header);
			free(chunk);
			record_header = NULL;
			chunk=NULL;
			continue;
		}

		bad_chunk = 0;

		/**
		* Audio check
		*
		* If we have a TyStream that has e.g. AC3 audio then
		* a chunk with MPEG-II audio is definitly a junk chunk.
		* This can happen in the start of a recording but
		* also in the middle of a recording when a OOS is
		* encountered. OOS == Out Of Sync i.e. a chunk
		* which is not in sync with the rest of the TyStream
		* usually from an other PID in the DirectTV PID - or
		* just junk in general
		*
		*
		* FIXME May have bad impact on shows how alter ac3/mpeg-II
		* We may want to set some form of indicator for this.
		*
		* NOTE: We don't do this check during the probe i.e. when max_probe
		* is bigger than 1.
		*/

		if(max_probe == 1 && tystream->tivo_probe == 0) {

			for(i=0; i < chunk->nr_records; i++){
				if(chunk->record_header[i].type == tystream->wrong_audio) {
						LOG_WARNING2("Read Chunk: " I64FORMAT " Record %i - wrong type of audio - skipping\n", chunk->chunk_number, i);
					bad_chunk++;
					break;
				}
			}


			if(bad_chunk) {
				for(i=0; i < chunk->nr_records; i++) {
					if(record_header[i].extended_data != NULL) {
						free(record_header[i].extended_data);
					}
				}
				free(record_header);
				free(chunk);
				record_header = NULL;
				chunk=NULL;
				continue;
			}
		}


		/* If we came this far everything is okay */

		records = (record_t *)malloc(sizeof(record_t) * chunk->nr_records);
		memset(records, 0,sizeof(record_t) * chunk->nr_records);
		chunk->records = records;


		for(i=0; i < chunk->nr_records; i++) {
			if (record_header[i].size == 0 ) {
				chunk->records[i].record = NULL;
			} else {
				in_records =(uint8_t *)malloc(sizeof(uint8_t) * record_header[i].size);
				memset(in_records, 0,sizeof(uint8_t) * record_header[i].size);
				if(!vstream) {
					read(tystream->in_file, in_records,sizeof(uint8_t) * record_header[i].size);
				} else {
					tydemux_read_vstream(in_records,sizeof(uint8_t) * record_header[i].size, 1, vstream);
				}

				chunk->records[i].record = in_records;
			}
		}

		if(max_seek > 1) {
			/* Don't ask me why I have to add 3 in bassmans version ? */
			LOG_WARNING2("WARNING: Chunk " I64FORMAT " is out of alignment with %i bytes\n", chunk_nr, max_seek - 1);
			tystream->miss_alinged = 1;
			tystream->byte_offset =  max_seek - 1;
		}

		/* Use this one when you want to debug a specific chunk */
		//if(chunk_nr > 170 && chunk_nr < 177) {
		//	print_chunk(chunk);
		//}
		return(chunk);

	}

	return(0);

}


/**************************************************************************************/

record_header_t * read_record(chunk_t * chunk, int in_file, vstream_t * vstream) {

	/* interation */
	int i, j;

	/* Data buffers for the header */
	record_header_t * record_header=NULL;
	uint8_t * tmp_record_array=NULL;

	/* Extended Data */
	uint8_t * extended_data;
	uint8_t ext0;
	uint8_t ext1;


	/* Create the tmp_buffer and read it in */
	tmp_record_array = (uint8_t *)malloc(sizeof(uint8_t) * chunk->nr_records * TYRECORD_HEADER_SIZE);
	memset(tmp_record_array, 0,sizeof(uint8_t) * chunk->nr_records * TYRECORD_HEADER_SIZE);

	if(!vstream) {
		read(in_file, tmp_record_array,sizeof(uint8_t) * chunk->nr_records * TYRECORD_HEADER_SIZE);
	} else {
		tydemux_read_vstream(tmp_record_array, sizeof(uint8_t) * chunk->nr_records * TYRECORD_HEADER_SIZE, 1, vstream);
	}

	record_header = (record_header_t *)malloc(sizeof(record_header_t) * chunk->nr_records);
	memset(record_header, 0,sizeof(record_header_t) * chunk->nr_records);

	LOG_DEVDIAG2("Record Header Read: chunk " I64FORMAT ", records %i\n", chunk->chunk_number, chunk->nr_records);

	for(i=0; i < chunk->nr_records * TYRECORD_HEADER_SIZE; i = i + TYRECORD_HEADER_SIZE) {
		for(j=0; j < TYRECORD_HEADER_SIZE ; j++ ) {
			LOG_DEVDIAG1(" %02x", tmp_record_array[i + j]);
		}
		LOG_DEVDIAG("\n");
	}

	/* Translate the tmp_buffer into a record_header array */

	for(i=0, j=0; i < chunk->nr_records * TYRECORD_HEADER_SIZE; j++, i = i + TYRECORD_HEADER_SIZE) {

			if((tmp_record_array[i] >> 4) == 0x08) {
				record_header[j].size = 0;

				/* Read the extened data */
				ext0 = ((tmp_record_array[i] & 0x0f) << 4) + ((tmp_record_array[i + 1] & 0xf0) >> 4);
				ext1 = ((tmp_record_array[i+1] & 0x0f) << 4) + ((tmp_record_array[i + 2] & 0xf0) >> 4);
				extended_data = (uint8_t *)malloc(sizeof(uint8_t) * 2);
				memset(extended_data, 0,sizeof(uint8_t) * 2);
				extended_data[0] = ext0;
				extended_data[1] = ext1;
				record_header[j].extended_data = extended_data;

			} else {
				/* Get record size */
				record_header[j].size = (tmp_record_array[i] << 12)
										+(tmp_record_array[i+1] << 4)
										+(tmp_record_array[i+2] >> 4);
				record_header[j].extended_data = NULL;
			}

			/* Get record type */
			record_header[j].type = ((tmp_record_array[i+2] & 0x0f) << 8)
									+(tmp_record_array[i+3]);
#if 0
			/* The rest of the header 12 bit stean id + 20bit offset in a buffer
			Used by Tivo + a 64 bit timestamp - Hmm we might use the steam id?? Nope doesn't look useful*/
			record_header[j].junk1 = (tmp_record_array[i+4] << 24)
									+(tmp_record_array[i+5] << 16)
									+(tmp_record_array[i+6] << 8)
									+(tmp_record_array[i+7]);

			time_stamp = 0;
			time_stamp = (tmp_record_array[i+8] << 24)
									+(tmp_record_array[i+9]  << 16)
									+(tmp_record_array[i+10] << 8)
									+(tmp_record_array[i+11]);
			time_stamp_2 = 0;
			time_stamp_2 = (tmp_record_array[i+12] << 24)
									+(tmp_record_array[i+13] << 16)
									+(tmp_record_array[i+14] << 8)
									+(tmp_record_array[i+15]);

			/* Why ?? Need to make usre that timestamp  upper part is zero */
			time_stamp_2 = time_stamp_2 & 0x00000000ffffffff;

			record_header[j].junk2 = (time_stamp << 32) + time_stamp_2;
#endif

			LOG_DEVDIAG1("\nRecord: %i\n", j+1);
			LOG_DEVDIAG1("Record size %u\n", record_header[j].size);
			LOG_DEVDIAG1("Record type %03x\n", record_header[j].type);
#if 0
			LOG_DEVDIAG1("Record junk1 %08x\n", record_header[j].junk1);
			LOG_DEVDIAG1("Record junk2 %016llx\n", record_header[j].junk2);

#endif
	}
	free(tmp_record_array);
	return(record_header);
}

