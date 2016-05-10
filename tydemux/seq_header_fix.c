/*
 * Copyright (C) 2002  Olof <jinxolina@gmail.com>
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

int counter=0;

/* Internal */
static payload_t * find_last_i_frame(const pes_holder_t * pes_holder);
static int  check_start_seq_pes_okay1(const pes_holder_t * seq_pes_holder);
static int check_start_seq_pes_okay2(const pes_holder_t * seq_pes_holder);

pes_holder_t *  fix_seq_header(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {

	/* pes holder */
	pes_holder_t * tmp_pes_holder;
	pes_holder_t * new_tmp_pes_holder;
	pes_holder_t * next_pes_holder;

	/* payload when we are missing I frame */
	payload_t * new_payload;

	/* Markers */
	int gotit;

	/* Init */
	gotit = 0;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;


	LOG_DEVDIAG("Fixing SEQ\n");

	if(tystream->tivo_series == S1) {

		while(tmp_pes_holder) {
			//printf("Fixing SEQ\n");
			if(tmp_pes_holder->seq_present && tmp_pes_holder->make_closed_gop) {
				data_times_t video_times;
				/* Need to check that we have enough of frames to make a closed gop - well we need a seq to be precise*/
				if(!check_start_seq_pes_okay1(tmp_pes_holder)) {
					LOG_WARNING("We can't close gop\n");
					counter++;
					if(counter > 10) {
						/* Okay we will never be able to close this gop
						abort - the user will need to make the cut in an
						other GOP */
						LOG_FATAL("We can't close the gop in this cut - choose another frame for your cut\n");
						LOG_FATAL("We are aborting now\n");
						exit(-1);
					}
					return(0);
				}
				counter = 0;
				//print_seq(tmp_pes_holder);
				LOG_WARNING1("We should be okay to close the gop - chunk "I64FORMAT"\n", tmp_pes_holder->chunk_nr);
				tmp_pes_holder = gop_make_closed(tystream, tmp_pes_holder, &video_times);
#if 0
				print_seq(previous_seq_holder(tmp_pes_holder));
				print_seq(tmp_pes_holder);
				print_seq(next_seq_holder(tmp_pes_holder));
#endif
				//gop_make_closed(tystream, tmp_pes_holder, &video_times);

				// John to OLAF - Had an error here where tmp_pes_holder == 0
				// So I added this checking code
				if( !tmp_pes_holder ) {
					LOG_FATAL("Failed to close gop -  choose another frame for your cut\n");
					LOG_FATAL("We are aborting now\n");
					exit(-1);
				}

				tmp_pes_holder->seq_fixed = 1;
#if 0
				if(!tmp_pes_holder->make_closed_gop) {
					printf("Did we get the wrong one back?\n");
				} else {
					printf("This one has a make close gop\n");
				}
#endif
				tmp_pes_holder->make_closed_gop = 0;
				tmp_pes_holder->repaired = 1;
				next_pes_holder = next_seq_holder(tmp_pes_holder);
				return(next_pes_holder);

			} else if (tmp_pes_holder->seq_present && (tmp_pes_holder->b_frame_present || tmp_pes_holder->p_frame_present)) {
				if(tmp_pes_holder->next && tmp_pes_holder->next->i_frame_present) {
					//printf("Fixing SEQ 1\n");
					if(move_seq_to_i_frame(tmp_pes_holder)) {
						tystream->lf_seq_pes_holder = tmp_pes_holder->next;
						tystream->lf_seq_pes_holder->seq_fixed = 1;
						tystream->lf_seq_pes_holder->seq_number = tystream->seq_counter;
						tystream->seq_counter++;
					}
					tmp_pes_holder = tmp_pes_holder->next;
					//printf("Fixing SEQ 2\n");
					continue;
				} else if (tmp_pes_holder->next && !tmp_pes_holder->next->i_frame_present && !tmp_pes_holder->next->gap){

					/* We have a major error the I Frame is missing :(
					We need to correct it other wise we will hang at a later stage */

					new_tmp_pes_holder = new_pes_holder(VIDEO);
					new_payload = find_last_i_frame(tmp_pes_holder);


					if(!new_payload) {
						free_pes_holder(new_tmp_pes_holder, 1);
						LOG_ERROR("ERROR Didn't find last I -Frame\n");
						return(0);
					}

					new_payload = duplicate_payload(new_payload);
					add_payload(new_tmp_pes_holder,new_payload);
					new_tmp_pes_holder->i_frame_present++;
					new_tmp_pes_holder->size = new_payload->size;
					new_tmp_pes_holder->total_size = new_payload->size;
					new_tmp_pes_holder->seq_added = 1;

					/* Add the new pes holder after tmp_pes_holder */

					tmp_pes_holder->next->previous = new_tmp_pes_holder;
					new_tmp_pes_holder->next = tmp_pes_holder->next;

					tmp_pes_holder->next = new_tmp_pes_holder;
					new_tmp_pes_holder->previous = tmp_pes_holder;

					/* Now we can move the seq to the I frame */

					if(move_seq_to_i_frame(tmp_pes_holder)) {
						tystream->lf_seq_pes_holder = tmp_pes_holder->next;
						tystream->lf_seq_pes_holder->seq_fixed = 1;
						tystream->lf_seq_pes_holder->seq_number = tystream->seq_counter;
						tystream->seq_counter++;
					}
					tmp_pes_holder = tmp_pes_holder->next;
					//printf("Fixing SEQ 2\n");
					continue;
				}

			} else if (tmp_pes_holder->seq_present && tmp_pes_holder->seq_fixed) {
				tystream->lf_seq_pes_holder = tmp_pes_holder;
				//printf("SEQ FIX: seq nr " I64FORMAT " \n", tmp_pes_holder->seq_number);
				return(next_seq_holder(tmp_pes_holder));
			}

			tmp_pes_holder = tmp_pes_holder->next;
		}

	} else {
		while(tmp_pes_holder) {
			if(tmp_pes_holder->seq_present && tmp_pes_holder->i_frame_present) {
				tystream->lf_seq_pes_holder = tmp_pes_holder;
				tystream->lf_seq_pes_holder->seq_fixed = 1;
				tystream->lf_seq_pes_holder->seq_number = tystream->seq_counter;
				tystream->seq_counter++;
			}
			tmp_pes_holder = tmp_pes_holder->next;
		}
	}
	return(0);
}



static payload_t * find_last_i_frame(const pes_holder_t * pes_holder) {



	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;



	tmp_pes_holder = (pes_holder_t *)pes_holder;


	//printf("Finding I Frame:\n");

	while(tmp_pes_holder) {

		//printf("Chunk nr: %i\n", tmp_pes_holder->chunk_nr);

		tmp_payload = tmp_pes_holder->payload;

		while(tmp_payload) {

			if(tmp_payload->payload_type == I_FRAME ) {
				/*Found the I-Frame now return */
				return(tmp_payload);
			}

			tmp_payload = tmp_payload->next;
		}

		tmp_pes_holder = tmp_pes_holder->previous;
	}


	//printf("Error didn't find a I-Frame searching backwards - trying fwd\n");

	tmp_pes_holder = (pes_holder_t *)pes_holder;

	while(tmp_pes_holder) {

		//printf("Chunk nr: %i\n", tmp_pes_holder->chunk_nr);

		tmp_payload = tmp_pes_holder->payload;

		while(tmp_payload) {

			if(tmp_payload->payload_type == I_FRAME ) {
				/*Found the I-Frame now return */
				return(tmp_payload);
			}

			tmp_payload = tmp_payload->next;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	//printf("Error didn't find a I-Frame searching backwards or forwards\n");

	return(0);

}




static int check_start_seq_pes_okay1(const pes_holder_t * seq_pes_holder) {

	int gotit;

	pes_holder_t * tmp_pes_holder;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;


	gotit = 0;

	while(tmp_pes_holder) {
		//LOG_USERDIAG("Loop to find how many frames\n");
		if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
			gotit = 1;
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(!gotit) {
		return(0);
	}
	return(1);
}


static int check_start_seq_pes_okay2(const pes_holder_t * seq_pes_holder) {

	int nr_frames;

	pes_holder_t * tmp_pes_holder;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	nr_frames = 0;

	while(tmp_pes_holder) {
		nr_frames++;
		tmp_pes_holder = tmp_pes_holder->next;
		if(nr_frames++ > 50 ) {
			return(1);
		}

	}

	return(0);

}
