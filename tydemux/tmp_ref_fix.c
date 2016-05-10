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

/* Internal */
static pes_holder_t * get_least_sign_frame(const pes_holder_t * pes_holder, payload_type_t frame_type, int init);

/**************************************************************************/

void fix_tmp_ref_after_close_gop(const pes_holder_t * seq_pes_holder, uint16_t nr_b_frames) {


	/* Markers */
	int seq_header;
	int next_seq_header;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;


	while(tmp_pes_holder) {
		if(tmp_pes_holder->gap) {
			break;
		}

		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {

			/* Run into the next SEQ header */
			if((tmp_payload->payload_type == SEQ_HEADER ||
				tmp_payload->payload_type == GOP_HEADER)
				&& tmp_pes_holder != seq_pes_holder) {

				next_seq_header = 1;
				break;
			}

			if(tmp_payload->payload_type == SEQ_HEADER) {
				seq_header = 1;
			}

			if((tmp_payload->payload_type == I_FRAME ||
				tmp_payload->payload_type == P_FRAME ||
				tmp_payload->payload_type == B_FRAME) &&
				seq_header) {
				//printf("Tmp ref %i - %i \n", tmp_payload->tmp_ref, tmp_payload->tmp_ref - nr_b_frames);
				tmp_payload->tmp_ref = tmp_payload->tmp_ref - nr_b_frames;

				set_temporal_reference(tmp_payload->payload_data,tmp_payload->tmp_ref);
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

/**************************************************************************/

void fix_tmp_ref_removing_frame(const pes_holder_t * pes_holder, uint16_t nr_of_removed, uint16_t temp_ref_removed) {

	/* Markers */
	int seq_header;
	int next_seq_header;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;

	tmp_pes_holder = (pes_holder_t *)pes_holder;

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

				next_seq_header = 1;
				break;
			}

			if(tmp_payload->payload_type == SEQ_HEADER) {
				seq_header = 1;
			}

			if((tmp_payload->payload_type == I_FRAME ||
				tmp_payload->payload_type == P_FRAME ||
				tmp_payload->payload_type == B_FRAME) &&
				seq_header) {
				if(tmp_payload->tmp_ref >= temp_ref_removed) {
					//printf("Tmp ref %i - %i \n", tmp_payload->tmp_ref, tmp_payload->tmp_ref - nr_of_removed);
					tmp_payload->tmp_ref = tmp_payload->tmp_ref - nr_of_removed;
					set_temporal_reference(tmp_payload->payload_data,tmp_payload->tmp_ref);
				}
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

/**************************************************************************/

void fix_tmp_ref_adding_frame(const pes_holder_t * pes_holder, uint16_t nr_of_added, uint16_t temp_ref_added) {

	/* Markers */
	int seq_header;
	int next_seq_header;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;

	tmp_pes_holder = (pes_holder_t *)pes_holder;

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

				next_seq_header = 1;
				break;
			}

			if(tmp_payload->payload_type == SEQ_HEADER) {
				seq_header = 1;
			}

			if((tmp_payload->payload_type == I_FRAME ||
				tmp_payload->payload_type == P_FRAME ||
				tmp_payload->payload_type == B_FRAME) &&
				seq_header) {
				if(tmp_payload->tmp_ref >= temp_ref_added) {
					if(!tmp_payload->tmp_ref_added) {
						LOG_DEVDIAG2("Tmp ref %i - %i \n", tmp_payload->tmp_ref, tmp_payload->tmp_ref + nr_of_added);
						tmp_payload->tmp_ref = tmp_payload->tmp_ref + nr_of_added;
						set_temporal_reference(tmp_payload->payload_data,tmp_payload->tmp_ref);
					} else {
						/* Reset it if we need to add more frames later on */
						tmp_payload->tmp_ref_added = 0;
					}
				}
			}
			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}


	tmp_pes_holder = (pes_holder_t *)pes_holder;

	return;

}


/**************************************************************************/

int fix_tmp_ref(tystream_holder_t * tystream, pes_holder_t * pes_holder) {



	/* Markers */
	int change_tystream_pes_holder;
	int gotit;
	int fake_previous;
	int first_pi_frame;
	int closed_gop;
	int break_loop;
	int bail_loop;
	int loop_counter;

	/* Pes holders */
	pes_holder_t * previous_pes_holder;
	pes_holder_t * next_seq_pes_holder;
	pes_holder_t * tmp_pes_holder;
	pes_holder_t * start_pes_holder;
	pes_holder_t * tmp_start_pes_holder;
	pes_holder_t * permanent_start_pes_holder;

	/* Payloads */
	payload_t * tmp_payload;

	/* Temporal reference */
	uint16_t previous_tmp_ref;

	/* Init */
	change_tystream_pes_holder = 0;
	fake_previous = 0;
	gotit = 0;
	first_pi_frame = 0;
	closed_gop = 0;
	break_loop = 0;
	bail_loop = 0;

	start_pes_holder = NULL;

	/* Okay first figure out what tystream->pes_holder_video
	points to */
	LOG_DEVDIAG("In Fix tmp ref \n");
	//print_seq(pes_holder);

	if(pes_holder == tystream->pes_holder_video) {
		LOG_DEVDIAG("pes_holder_video the same as pes_holder");
		change_tystream_pes_holder = 1;
	}


	/* Now create a holder of fake or real
	previous pes holder */

	if(!pes_holder->previous) {
		LOG_DEVDIAG("We have to fake a previous\n");
		fake_previous = 1;
		previous_pes_holder = new_pes_holder(VIDEO);
		pes_holder->previous = previous_pes_holder;
		previous_pes_holder->next = pes_holder;

	} else {
		//LOG_DEVDIAG("Real previous\n");
		previous_pes_holder=pes_holder->previous;
	}

	LOG_DEVDIAG1("Pre pes chunk nr: " I64FORMAT " \n", previous_pes_holder->chunk_nr);
	/* Find the pes holder that holds the next
	seq header */

	tmp_pes_holder = pes_holder;

	while(tmp_pes_holder) {
		LOG_DEVDIAG("seeking\n");
		if(tmp_pes_holder->seq_present && tmp_pes_holder != pes_holder) {
			gotit = 1;
			break;
		} else {
			tmp_pes_holder->this_seq = 1;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(gotit) {
		next_seq_pes_holder = tmp_pes_holder;
	} else {
		//LOG_DEVDIAG("Error - 1\n");
		return(0);
	}
	gotit = 0;

	LOG_DEVDIAG1("Seq nr of this seq is " I64FORMAT "\n", pes_holder->seq_number);
	LOG_DEVDIAG1("Seq nr of next seq is " I64FORMAT "\n",next_seq_pes_holder->seq_number);




	/* Okay so lets get the first I frame :) */

	tmp_pes_holder = pes_holder;
	start_pes_holder = get_least_sign_frame(tmp_pes_holder, P_FRAME, 1);

	if(!start_pes_holder) {
		//LOG_DEVDIAG("Error - 2\n");
		return(0);
	}


	/* let tie up the stream and remove this pes holder*/

	tmp_pes_holder = pes_holder;

	while(tmp_pes_holder) {
		if(tmp_pes_holder == start_pes_holder) {
			gotit = 1;
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(!gotit) {
		//LOG_DEVDIAG("Error - 3\n");
		return(0);
	}
	gotit = 0;


	tmp_pes_holder->previous->next = tmp_pes_holder->next;
	tmp_pes_holder->next->previous = tmp_pes_holder->previous;

	tmp_pes_holder = previous_pes_holder->next;




	/* The I Frame is our new pes holder - reset it and
	set it to the start pes the start pes */
	start_pes_holder->next = NULL;
	start_pes_holder->previous = NULL;

	permanent_start_pes_holder = start_pes_holder;


	/* Now lets check if we have a closed or open gop,
	get the I-Frame payload and tmp ref */

	tmp_payload = start_pes_holder->payload;

	while(tmp_payload) {
		if(tmp_payload->payload_type == I_FRAME) {
			if(tmp_payload->tmp_ref == 0) {
				closed_gop = 1;
				gotit = 1;
			}
			gotit = 1;
			previous_tmp_ref = tmp_payload->tmp_ref;
			break;
		}
		tmp_payload = tmp_payload->next;
	}

	if(!gotit) {
		//LOG_DEVDIAG("Error - 4\n");
		return(0);
	}

	LOG_DEVDIAG1("Pre pes chunk nr: " I64FORMAT " \n", previous_pes_holder->chunk_nr);

	loop_counter = 0;

	if(closed_gop) {
		LOG_DEVDIAG("Closed GOP\n");
		first_pi_frame = 1;
		while(1) {
			loop_counter++;
			/* Get the P/I Frame */
			tmp_pes_holder = previous_pes_holder->next;
			tmp_start_pes_holder = get_least_sign_frame(tmp_pes_holder, P_FRAME, 0);


			if(tmp_start_pes_holder == NULL) {
				return(0);
			} else {
				break_loop = 0;
			}
			/* Get the P-Frame payload */
			tmp_payload = tmp_start_pes_holder->payload;
			gotit = 0;
			while(tmp_payload) {
				if(tmp_payload->payload_type == P_FRAME ||
				tmp_payload->payload_type == I_FRAME) {
					gotit=1;
					break;

				}
				tmp_payload = tmp_payload->next;
			}

			if(!gotit) {
				return(0);
			}
			gotit = 0;

			/* Check the tmp_ref of the P/I-Frame */
			if(first_pi_frame) {
				if(tmp_payload->tmp_ref > previous_tmp_ref) {

					/* We found an okay P-Frame
					lets find it in the SEQ and
					remove it */
					tmp_pes_holder = previous_pes_holder->next;

					gotit = 0;
					while(tmp_pes_holder) {
						if(tmp_pes_holder == tmp_start_pes_holder) {
							gotit = 1;
							break;
						}
						tmp_pes_holder = tmp_pes_holder->next;
					}

					if(!gotit) {
						return(0);
					}
					gotit = 0;

					/* Remove the pes holder */
					tmp_pes_holder->previous->next = tmp_pes_holder->next;
					tmp_pes_holder->next->previous = tmp_pes_holder->previous;

					/* Attache it to the start_pes_holder linked list */
					tmp_start_pes_holder->previous = start_pes_holder;
					tmp_start_pes_holder->next = NULL;
					start_pes_holder->next = tmp_start_pes_holder;

					start_pes_holder = tmp_start_pes_holder;

					/* Fix tmp ref and pi frame for the next turn around */
					previous_tmp_ref = tmp_payload->tmp_ref;
					first_pi_frame = 0;
				} else {
					break;
				}
			} else {

				if(tmp_payload->tmp_ref ==  previous_tmp_ref + 1) {

					/* We found an okay P-Frame
					lets find it in the SEQ and
					remove it */
					tmp_pes_holder = previous_pes_holder->next;

					gotit = 0;
					while(tmp_pes_holder) {
						if(tmp_pes_holder == tmp_start_pes_holder) {
							gotit = 1;
							break;
						}
						tmp_pes_holder = tmp_pes_holder->next;
					}

					if(!gotit) {
						return(0);
					}
					gotit = 0;

					/* Remove the pes holder */
					tmp_pes_holder->previous->next = tmp_pes_holder->next;
					tmp_pes_holder->next->previous = tmp_pes_holder->previous;

					/* Attache it to the start_pes_holder linked list */
					tmp_start_pes_holder->previous = start_pes_holder;
					tmp_start_pes_holder->next = NULL;
					start_pes_holder->next = tmp_start_pes_holder;
					start_pes_holder = tmp_start_pes_holder;

					/* Fix tmp ref and pi frame for the next turn around */
					previous_tmp_ref = tmp_payload->tmp_ref;
					first_pi_frame = 0;
				} else {
					break_loop=1;
					break;
				}
			}

			if(loop_counter > 1025) {
				break;
			}

			if(break_loop) {
				break;
			}
		}
	}


	/* Now loop until we have removed all pes holder between the
	previous pes holder and the next seq pes holder - when that
	is done we have sorted all pes holders */

	loop_counter = 0;
	while(previous_pes_holder->next != next_seq_pes_holder) {
		LOG_DEVDIAG("In fix tmp ref loop\n");
		loop_counter++;

		while(1 && !bail_loop) {

			/* Get the P/I Frame */
			tmp_pes_holder = previous_pes_holder->next;
			tmp_start_pes_holder = get_least_sign_frame(tmp_pes_holder, P_FRAME, 0);

			if(tmp_start_pes_holder &&  !tmp_start_pes_holder->this_seq) {
				bail_loop = 1;
				break;
			}

			if(tmp_start_pes_holder) {
				LOG_DEVDIAG2("Got a P - Frame tmp ref %i, chunk " I64FORMAT "\n", \
					tmp_ref_of_payload(tmp_start_pes_holder->payload), \
					tmp_start_pes_holder->chunk_nr);
			}

			if(tmp_start_pes_holder == NULL) {
				break_loop++;
				break;
			} else {
				break_loop = 0;
			}

			/* Get the P-Frame payload */
			tmp_payload = tmp_start_pes_holder->payload;
			gotit = 0;
			while(tmp_payload) {
				if(tmp_payload->payload_type == P_FRAME ||
				tmp_payload->payload_type == I_FRAME) {
					gotit = 1;
					break;
				}
				tmp_payload = tmp_payload->next;
			}

			if(!gotit) {
				return(0);
			}

			gotit = 0;
			LOG_DEVDIAG2("Tmp ref %i Pre ref %i\n",tmp_payload->tmp_ref,  previous_tmp_ref);
			/* Check the tmp_ref of the P/I-Frame */
			if(first_pi_frame) {
				if(tmp_payload->tmp_ref > previous_tmp_ref) {

					/* We found an okay P/I-Frame
					lets find it in the SEQ and
					remove it */
					LOG_DEVDIAG("Using it\n\n");
					tmp_pes_holder = previous_pes_holder->next;

					gotit = 0;
					while(tmp_pes_holder) {
						if(tmp_pes_holder == tmp_start_pes_holder) {
							gotit = 1;
							break;
						}
						tmp_pes_holder = tmp_pes_holder->next;
					}
					if(!gotit) {
						LOG_DEVDIAG("Error - 7\n");
						return(0);
					}
					gotit = 0;

					/* Remove the pes holder */
					tmp_pes_holder->previous->next = tmp_pes_holder->next;
					tmp_pes_holder->next->previous = tmp_pes_holder->previous;

					/* Attache it to the start_pes_holder linked list */
					tmp_start_pes_holder->previous = start_pes_holder;
					start_pes_holder->next = tmp_start_pes_holder;
					tmp_start_pes_holder->next = NULL;

					start_pes_holder = tmp_start_pes_holder;

					/* Fix tmp ref and pi frame for the next turn around */
					previous_tmp_ref = tmp_payload->tmp_ref;
					first_pi_frame = 0;
				} else {
					LOG_DEVDIAG("Ditching it\n\n");
					break;
				}
			} else {

				if(tmp_payload->tmp_ref ==  previous_tmp_ref + 1) {

					/* We found an okay P/I-Frame
					lets find it in the SEQ and
					remove it */
					LOG_DEVDIAG("Using it\n\n");
					tmp_pes_holder = previous_pes_holder->next;

					gotit = 0;
					while(tmp_pes_holder) {
						if(tmp_pes_holder == tmp_start_pes_holder) {
							gotit = 1;
							break;
						}
						tmp_pes_holder = tmp_pes_holder->next;
					}

					if(!gotit) {
						LOG_DEVDIAG("Error - 8\n");
						return(0);
					}
					gotit = 0;

					/* Remove the pes holder */
					tmp_pes_holder->previous->next = tmp_pes_holder->next;
					tmp_pes_holder->next->previous = tmp_pes_holder->previous;

					/* Attache it to the start_pes_holder linked list */
					tmp_start_pes_holder->previous = start_pes_holder;
					start_pes_holder->next = tmp_start_pes_holder;
					tmp_start_pes_holder->next = NULL;

					start_pes_holder = tmp_start_pes_holder;

					/* Fix tmp ref and pi frame for the next turn around */
					previous_tmp_ref = tmp_payload->tmp_ref;
					first_pi_frame = 0;
				} else {
					LOG_DEVDIAG("Ditching it\n\n");
					break;
				}
			}
		}





/*********************************/



		/* Get B-Frames */
		while(1 && !bail_loop)  {
			/* Get the B Frame */
			tmp_pes_holder = previous_pes_holder->next;
			tmp_start_pes_holder = get_least_sign_frame(tmp_pes_holder, B_FRAME, 0);

			if(tmp_start_pes_holder &&  !tmp_start_pes_holder->this_seq) {
				bail_loop = 1;
				break;
			}

			if(tmp_start_pes_holder) {
				LOG_DEVDIAG2("Got a B - Frame tmp ref %i, chunk " I64FORMAT "\n", \
					tmp_ref_of_payload(tmp_start_pes_holder->payload), \
					tmp_start_pes_holder->chunk_nr);
			}

			if(tmp_start_pes_holder == NULL) {
				break_loop++;
				break;
			} else {
				break_loop = 0;
			}

			/* Get the B-Frame payload */
			gotit=0;

			tmp_payload = tmp_start_pes_holder->payload;
			while(tmp_payload) {
				if(tmp_payload->payload_type == B_FRAME) {
					gotit=1;
					break;
				}
				tmp_payload = tmp_payload->next;
			}

			if(!gotit) {
				LOG_DEVDIAG("Error - 5\n");
				return(0);
			}
			gotit=0;
			LOG_DEVDIAG2("Tmp ref %i Pre ref %i\n",tmp_payload->tmp_ref,  previous_tmp_ref);
			/* Check the tmp_ref of the B-Frame */
			if(tmp_payload->tmp_ref < previous_tmp_ref) {
				LOG_DEVDIAG("Using it\n\n");
				/* We found an okay B-Frame
				lets find it in the SEQ and
				remove it */

				tmp_pes_holder = previous_pes_holder->next;

				gotit = 0;
				while(tmp_pes_holder) {
					if(tmp_pes_holder == tmp_start_pes_holder) {
						gotit = 1;
						break;
					}
					tmp_pes_holder = tmp_pes_holder->next;
				}

				if(!gotit) {
					LOG_DEVDIAG("Error - 6\n");
					return(0);
				}
				gotit = 0;

				/* Remove the pes holder */
				tmp_pes_holder->previous->next = tmp_pes_holder->next;
				tmp_pes_holder->next->previous = tmp_pes_holder->previous;

				/* Attache it to the start_pes_holder linked list */
				tmp_start_pes_holder->previous = start_pes_holder;
				start_pes_holder->next = tmp_start_pes_holder;
				tmp_start_pes_holder->next = NULL;

				start_pes_holder = tmp_start_pes_holder;

				/* Fix tmp ref and pi frame for the next turn around
				we add to to catch the next B-Frame but not a B-Frame
				that comes after a P/I - Frame */
				previous_tmp_ref = tmp_payload->tmp_ref + 2;
				first_pi_frame = 1;
			} else {
				LOG_DEVDIAG("Ditching it\n\n");
				break;
			}
		}

		/* Get P/I Frames */

		while(1 && !bail_loop) {

			/* Get the P/I Frame */
			tmp_pes_holder = previous_pes_holder->next;
			tmp_start_pes_holder = get_least_sign_frame(tmp_pes_holder, P_FRAME, 0);

			if(tmp_start_pes_holder &&  !tmp_start_pes_holder->this_seq) {
				bail_loop = 1;
				break;
			}

			if(tmp_start_pes_holder) {
				LOG_DEVDIAG2("Got a P - Frame tmp ref %i, chunk " I64FORMAT "\n", \
					tmp_ref_of_payload(tmp_start_pes_holder->payload), \
					tmp_start_pes_holder->chunk_nr);
			}

			if(tmp_start_pes_holder == NULL) {
				break_loop++;
				break;
			} else {
				break_loop = 0;
			}

			/* Get the P-Frame payload */
			tmp_payload = tmp_start_pes_holder->payload;
			gotit = 0;
			while(tmp_payload) {
				if(tmp_payload->payload_type == P_FRAME ||
				tmp_payload->payload_type == I_FRAME) {
					gotit = 1;
					break;
				}
				tmp_payload = tmp_payload->next;
			}

			if(!gotit) {
				return(0);
			}
			gotit = 0;
			LOG_DEVDIAG2("Tmp ref %i Pre ref %i\n",tmp_payload->tmp_ref,  previous_tmp_ref);
			/* Check the tmp_ref of the P/I-Frame */
			if(first_pi_frame) {
				if(tmp_payload->tmp_ref > previous_tmp_ref) {

					/* We found an okay P/I-Frame
					lets find it in the SEQ and
					remove it */
					LOG_DEVDIAG("Using it\n\n");
					tmp_pes_holder = previous_pes_holder->next;

					gotit = 0;
					while(tmp_pes_holder) {
						if(tmp_pes_holder == tmp_start_pes_holder) {
							gotit = 1;
							break;
						}
						tmp_pes_holder = tmp_pes_holder->next;
					}
					if(!gotit) {
						//LOG_DEVDIAG("Error - 7\n");
						return(0);
					}
					gotit = 0;

					/* Remove the pes holder */
					tmp_pes_holder->previous->next = tmp_pes_holder->next;
					tmp_pes_holder->next->previous = tmp_pes_holder->previous;

					/* Attache it to the start_pes_holder linked list */
					tmp_start_pes_holder->previous = start_pes_holder;
					start_pes_holder->next = tmp_start_pes_holder;
					tmp_start_pes_holder->next = NULL;

					start_pes_holder = tmp_start_pes_holder;

					/* Fix tmp ref and pi frame for the next turn around */
					previous_tmp_ref = tmp_payload->tmp_ref;
					first_pi_frame = 0;
				} else {
					LOG_DEVDIAG("Ditching it\n\n");
					break;
				}
			} else {

				if(tmp_payload->tmp_ref ==  previous_tmp_ref + 1) {

					/* We found an okay P/I-Frame
					lets find it in the SEQ and
					remove it */
					LOG_DEVDIAG("Using it\n\n");
					tmp_pes_holder = previous_pes_holder->next;

					gotit = 0;
					while(tmp_pes_holder) {
						if(tmp_pes_holder == tmp_start_pes_holder) {
							gotit = 1;
							break;
						}
						tmp_pes_holder = tmp_pes_holder->next;
					}

					if(!gotit) {
						//LOG_DEVDIAG("Error - 8\n");
						return(0);
					}
					gotit = 0;

					/* Remove the pes holder */
					tmp_pes_holder->previous->next = tmp_pes_holder->next;
					tmp_pes_holder->next->previous = tmp_pes_holder->previous;

					/* Attache it to the start_pes_holder linked list */
					tmp_start_pes_holder->previous = start_pes_holder;
					start_pes_holder->next = tmp_start_pes_holder;
					tmp_start_pes_holder->next = NULL;

					start_pes_holder = tmp_start_pes_holder;

					/* Fix tmp ref and pi frame for the next turn around */
					previous_tmp_ref = tmp_payload->tmp_ref;
					first_pi_frame = 0;
				} else {
					LOG_DEVDIAG("Ditching it\n\n");
					break;
				}
			}
		}
		if(loop_counter > 1025) {
			break;
		}

		if(break_loop > 3) {
			break;
		}
		if(bail_loop) {
			break;
		}

	}


	/* The whole seq should no be sorted in such way that the temporal
	reference is okay lets just check that we actually made it to the end */

	if(previous_pes_holder->next != next_seq_pes_holder) {
		LOG_DEVDIAG("Error pre next not next seq\n");
		return(0);
	}

	/* Remove the marker */
	tmp_pes_holder = permanent_start_pes_holder;

	while(tmp_pes_holder) {
		LOG_DEVDIAG("seeking\n");
			tmp_pes_holder->this_seq = 0;
		tmp_pes_holder = tmp_pes_holder->next;
	}

	/* Okay lets fold it together */

	permanent_start_pes_holder->previous = previous_pes_holder;
	previous_pes_holder->next = permanent_start_pes_holder;

	/* The start pes holder is the last pes in the seq */
	start_pes_holder->next = next_seq_pes_holder;
	next_seq_pes_holder->previous = start_pes_holder;


	if(fake_previous) {
		permanent_start_pes_holder->previous = NULL;
		free_pes_holder(previous_pes_holder, 0);
	}

	if(change_tystream_pes_holder) {
		tystream->pes_holder_video = permanent_start_pes_holder;
	}

	return(1);
}


/**************************************************************************/

static pes_holder_t * get_least_sign_frame(const pes_holder_t * pes_holder, payload_type_t frame_type, int init) {


	/* Holder of least sign  frame */
	pes_holder_t * least_sign_frame_holder;

	/* Least sign ip frame tmp_ref */
	uint16_t least_sign_frame;

	/* Markers */
	int seq_header;
	int next_seq_header;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;


	/* Init */
	next_seq_header = 0;
	least_sign_frame_holder = NULL;
	least_sign_frame = 0xffff;

	if(init) {
		seq_header = 0;
	} else {
		seq_header = 1;
	}

	tmp_pes_holder = (pes_holder_t *)pes_holder;

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

				next_seq_header = 1;
				break;
			}

			if(tmp_payload->payload_type == SEQ_HEADER) {
				seq_header = 1;
			}
			if(frame_type == P_FRAME) {
				if((tmp_payload->payload_type == I_FRAME ||
					tmp_payload->payload_type == P_FRAME)
					&& seq_header) {

					if(least_sign_frame > tmp_payload->tmp_ref) {
						least_sign_frame = tmp_payload->tmp_ref;
						least_sign_frame_holder = tmp_pes_holder;
					}
				}

			} else {

				if(tmp_payload->payload_type == B_FRAME && seq_header) {

					if(least_sign_frame > tmp_payload->tmp_ref) {
						least_sign_frame = tmp_payload->tmp_ref;
						least_sign_frame_holder = tmp_pes_holder;
					}

				}
			}

			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	return(least_sign_frame_holder);

}
