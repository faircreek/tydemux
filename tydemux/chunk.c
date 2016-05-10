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

void free_chunk(chunk_t * chunk) {

	/* Iteration */
	int i;

	for (i=0; i < chunk->nr_records; i++) {
		if(chunk->record_header[i].size != 0) {
			free(chunk->records[i].record);
		}
		if(chunk->record_header[i].extended_data != NULL) {
			free(chunk->record_header[i].extended_data);
		}
	}
	free(chunk->records);
	free(chunk->record_header);

	/* Fix any references to this chunk */
	if (chunk->previous && chunk->previous->next == chunk) {
		chunk->previous->next = NULL;
	}
	if (chunk->next && chunk->next->previous == chunk) {
		chunk->next->previous = NULL;
	}

	free(chunk);
}

void free_junk_chunks(chunk_t * chunks) {


	chunk_t * old_chunk;

	/* Go back to the first chunk */
	while(chunks->previous) {
		chunks = chunks->previous;
	}

	while(chunks) {
		old_chunk = chunks;
		chunks = chunks->next;
		free_chunk(old_chunk);
	}
}


chunk_t * add_chunk(tystream_holder_t * tystream, chunk_t * chunk, chunk_t * chunks){

	/* Iteration */
	int i;

	/* The first chunk */
	chunk_t * save;

	/* If we want to save new
	duplicated chunk
	chunk_t * old_chunk;*/

	/* Uplication indicator */
	int duplicate;

	duplicate = 0;

	/* If it's the first chunk */
	if(chunks == NULL) {
		LOG_DEVDIAG("Chunk was NULL returning\n");
		return(chunk);
	}

	save = chunks;
	LOG_DEVDIAG("Chunk was not NULL \n");

	while (chunks->next) {
		chunks = chunks->next;
	}


	/* Check for possible duplication of chunk - HMM is this DTIVO ONLY FIXME */
	if(chunk->nr_records == chunks->nr_records && chunk->seq_start == chunks->seq_start) {
		/* We might have a duplicate :( */
		for(i=0; i < chunks->nr_records; i++) {
#if 0
			if(chunks->record_header[i].size  == chunk->record_header[i].size &&
			   chunks->record_header[i].type  == chunk->record_header[i].type &&
			   chunks->record_header[i].junk1 == chunk->record_header[i].junk1 &&
			   chunks->record_header[i].junk2 == chunk->record_header[i].junk2) {
#endif
			if(chunks->record_header[i].size  == chunk->record_header[i].size &&
			   chunks->record_header[i].type  == chunk->record_header[i].type) {

				duplicate = 1;
			} else {
				duplicate = 0;
				break;
			}
		}
	}

	/* FIXME We need to take care of duplication check in the junk_chunks too */

	if (duplicate) {
		/* Well either we keep the old chunk
		 or we replace it with the new one -
		 good guees is that the new one is
		 better than the old one "error"
		 made them rerecord it??*
		 Anyways still sticking with the old chunk.
		 */
		LOG_WARNING1("Duplicated chunk " I64FORMAT " - skipping\n", chunk->chunk_number);

		printf("Duplicated chunk " I64FORMAT " - skipping\n", chunk->chunk_number);


		/* When we add the junk buffer and we have a remaining
		junkchunk this will set it to zero FIXME very unlikely although*/
		if(chunk->junk) {
			tystream->cons_junk_chunk--;
		}


		/* If this is the junk chunk buffer we will leak !! and go south FIXME */
		if(!chunk->next) {
			free_chunk(chunk);
		} else {
			LOG_WARNING("Duplication error\n");
		}

		/*Keep new chunk code
		LOG_WARNING1("Dupliacted chunk %l - skipping\n", chunk_nr);
		old_chunk = chunks;
		chunk->previous = chunks->previous;
		chunk->next = null;
		chunk->previous->next = chunk;
		free_chunk(old_chunk);
		*/
	} else {
		chunks->next = chunk;
		chunk->previous = chunks;
	}
	return(save);
}
