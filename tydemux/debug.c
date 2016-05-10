
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

void print_chunk(const chunk_t * chunk) {

	/* Iteration */
	int i;
	uint32_t j;

	if(ty_debug >= 7) {
		printf("print_chunk\n");
	}

	printf("\nChunk: " I64FORMAT "\n", chunk->chunk_number);

	for (i=0; i < chunk->nr_records; i++) {
		printf("Record: %i size: %u type: %03x\n", i,
			chunk->record_header[i].size, chunk->record_header[i].type);
		if(chunk->record_header[i].size != 0) {
			printf("Record data - start:\n ");

			for (j=0; j < chunk->record_header[i].size && j < 64; j ++) {
				printf("%02x ", chunk->records[i].record[j]);
				if(j != 0 && j%16 == 0) {
					printf("\n");
				}
			}
			printf("\n");

			if(chunk->record_header[i].size > 64) {
				printf("Record data - end:\n ");
				for (j= chunk->record_header[i].size - 64; j < chunk->record_header[i].size; j ++) {
					printf("%02x ", chunk->records[i].record[j]);
					if(j != 0 && j%16 == 0) {
						printf("\n");
					}
				}
			} else {
				printf("Record data - end - see start:\n ");
			}

			printf("\n");
		}

		if(chunk->record_header[i].extended_data != NULL) {
			printf("Extended Data: %02x %02x\n", chunk->record_header[i].extended_data[0],
				chunk->record_header[i].extended_data[1]);
		}
	}

}


void print_seq(const pes_holder_t * pes_holder) {

	/* Markers */
	int seq_header;
	int next_seq_header;


	int nr_of_frames;

	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;

	nr_of_frames = 0;

	//return;

	tmp_pes_holder = (pes_holder_t *)pes_holder;
	if(tmp_pes_holder) {
		printf("SEQ: " I64FORMAT " - Chunk " I64FORMAT " \n", tmp_pes_holder->seq_number, tmp_pes_holder->chunk_nr);
	} else {
		printf("The seq holder is null !!!\n");
	}
	
	while(tmp_pes_holder) {
		if(tmp_pes_holder->gap) {
			break;
		}

		tmp_payload = tmp_pes_holder->payload;

		while(tmp_payload) {

			/* Run into the next SEQ header */
			if((tmp_payload->payload_type == SEQ_HEADER ||
				tmp_payload->payload_type == GOP_HEADER)
				&& tmp_pes_holder != pes_holder) {
				printf("Next SEQ header \n");
				next_seq_header = 1;
				break;
			}

			if(tmp_payload->payload_type == SEQ_HEADER) {
				printf("Found SEQ\n");
				seq_header = 1;
			}

			if((tmp_payload->payload_type == I_FRAME ||
				tmp_payload->payload_type == P_FRAME ||
				tmp_payload->payload_type == B_FRAME) &&
				seq_header) {
				printf("\tSEQ: " I64FORMAT " Chunk: " I64FORMAT " Payload type: %i Tmp ref: %i Payload ref: %i"
					" Payload time: " I64FORMAT " Frame number %i\n",
					tmp_pes_holder->seq_number, tmp_pes_holder->chunk_nr,
					tmp_payload->payload_type, get_temporal_reference(tmp_payload->payload_data),
					tmp_payload->tmp_ref, tmp_pes_holder->time, nr_of_frames);
				nr_of_frames++;
			}
			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	return;
}



void print_video_pes(tystream_holder_t * tystream) {


	pes_holder_t * tmp_pes_holder;
	payload_t * tmp_payload;

	tmp_pes_holder = tystream->pes_holder_video;

	while(tmp_pes_holder) {
		printf("\nPES Holder:\n");
		printf("\t\tSEQ %i, GOP %i, I %i, P %i, B %i\n",
			tmp_pes_holder->seq_present,
			tmp_pes_holder->gop_present,
			tmp_pes_holder->i_frame_present,
			tmp_pes_holder->p_frame_present,
			tmp_pes_holder->b_frame_present);
		printf("\t\tPayload:\n");
		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {
			printf("\t\t\tType: %i \n", tmp_payload->payload_type);
			if(tmp_payload->payload_type == 3 ||
				tmp_payload->payload_type == 4 ||
				tmp_payload->payload_type == 5) {
					printf("\t\t\tTmp ref: %i \n", get_temporal_reference(tmp_payload->payload_data));
				}
			tmp_payload = tmp_payload->next;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}

	return;
}


void print_chunks(const chunk_t * chunk) {


	chunk_t * tmp_chunk;

	tmp_chunk = (chunk_t *)chunk;


	while(tmp_chunk) {
		printf("Chunk " I64FORMAT ", Gap %i\n", tmp_chunk->chunk_number, tmp_chunk->gap);
		tmp_chunk = tmp_chunk->next;
	}

	return;
}
