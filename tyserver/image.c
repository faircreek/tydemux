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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <ctype.h>

#define LOG_MODULE "tyserver"
#include "tylogging.h"
#include "../tydemux/tydemux.h"
#include "image.h"
#include "readfile.h"

extern int totalchunks;
extern int infile;
extern tystream_holder_t * tystream;


int get_first_seq_tivo(gop_index_t * gop_index, module_t * module) {

	/* Iteration */
	int i;

	/* vstream */
	vstream_t * vstream;

	/* The chunk we read */
	chunk_t * chunk = NULL;

	/* The chunk linked list */
	chunk_t * chunks = NULL;

	int chunks_to_read;
	int result;


	vstream = new_vstream();
	/* Lets fake it and do a read buffer */
	vstream->start_stream = (uint8_t *)malloc(sizeof(uint8_t) * CHUNK_SIZE);
	vstream->size = sizeof(uint8_t) * CHUNK_SIZE ;
	vstream->current_pos = vstream->start_stream;
	vstream->end_stream = vstream->start_stream + vstream->size;
	tystream->vstream = vstream;



	chunks_to_read = 2;


	for(i=0; i < chunks_to_read; i++) {

		vstream->current_pos = vstream->start_stream;
		vstream->eof=0;
		read_a_chunk(infile, gop_index->chunk_number_seq + i + tystream->start_chunk, vstream->start_stream);
		chunk = read_chunk(tystream, gop_index->chunk_number_seq + i + tystream->start_chunk, 1);

		if(chunk) {
			chunks = add_chunk(tystream, chunk, chunks);
		} else {
			chunks_to_read++;
		}
	}

	free_vstream(vstream);
	tystream->vstream=NULL;


	LOG_DEVDIAG("getting seq\n");
	result = get_video(gop_index->seq_rec_nr, module, chunks, MPEG_SEQ, tystream);


	if(!result) {
		LOG_ERROR2("parse_chunk_video: ERROR - seq-frame - chunk %lld , record %i\n", \
			chunks->chunk_number, gop_index->seq_rec_nr);
		free_junk_chunks(chunks);
		return(get_first_seq(tystream,gop_index->next,module));
		return(0);
	}
	LOG_DEVDIAG("setting seq\n");
	set_seq_low_delay(tystream, module);
	free_junk_chunks(chunks);
	LOG_DEVDIAG("getting seq done\n");
	return(1);


}

module_t * init_image_tivo() {


	module_t * module;


	if(!tystream->index) {
		return(0);
	}

	module = (module_t *)malloc(sizeof(module_t));
	module->data_buffer = NULL;
	module->buffer_size = 0;

	if(!get_first_seq_tivo(tystream->index->gop_index, module)) {
		free_module(module);
		return(0);
	}

	return(module);

}


int get_image_chunk_tivo(gop_index_t * gop_index,module_t * module)
{

	/* Iteration */
	int i;


	/* The chunk we read */
	chunk_t * chunk = NULL;

	/* The chunk linked list */
	chunk_t * chunks = NULL;

	/* vstream */
	vstream_t * vstream;

	int chunks_to_read;
	int result;


	chunks_to_read = 2;

	/* Find out how many chunks we need to read
	if pes and i frame is in the same chunk then two chunks if not three chunks
	*/


	vstream = new_vstream();
	/* Lets fake it and do a read buffer */
	vstream->start_stream = (uint8_t *)malloc(sizeof(uint8_t) * CHUNK_SIZE);
	vstream->size = sizeof(uint8_t) * CHUNK_SIZE ;
	vstream->current_pos = vstream->start_stream;
	vstream->end_stream = vstream->start_stream + vstream->size;
	tystream->vstream = vstream;

	for(i=0; i < chunks_to_read; i++) {

		vstream->current_pos = vstream->start_stream;
		vstream->eof=0;
		read_a_chunk(infile, gop_index->chunk_number_i_frame + i + tystream->start_chunk, vstream->start_stream);

		chunk = read_chunk(tystream, gop_index->chunk_number_i_frame_pes + i, 1);

		if(chunk) {
			chunks = add_chunk(tystream, chunk, chunks);
		} else {
			chunks_to_read++;
		}
	}

	free_vstream(vstream);
	tystream->vstream=NULL;



	result = get_video(gop_index->i_frame_rec_nr, module, chunks, MPEG_I, tystream);

	if(!result) {
		LOG_ERROR2("parse_chunk_video: ERROR - i-frame - chunk %lld , record %i\n", \
			chunks->chunk_number,gop_index->i_frame_rec_nr );
		free_junk_chunks(chunks);
		return(0);
	}

	//write(2,data_collector_module.data_buffer,data_collector_module.buffer_size);

	free_junk_chunks(chunks);

	return(1);

}


module_t * get_image_tivo(gop_index_t * gop_index) {



	/* Tydemux */

	module_t * module;
	module_t * return_module;


		return_module = NULL;

	if(!tystream->index) {
		return(0);
	}


	module = (module_t *)malloc(sizeof(module_t));
	module->data_buffer = NULL;
	module->buffer_size = 0;


	if(!get_image_chunk_tivo(gop_index, module)) {
		free(module);
		return(0);
	}

	return(module);

}




