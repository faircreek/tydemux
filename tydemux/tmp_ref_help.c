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

uint16_t get_highest_tmp_ref(const pes_holder_t * seq_pes_holder) {

	/* Markers */
	int seq_header;
	int next_seq_header;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;

	/* Highest tmp ref */
	uint16_t highest_tmp_ref;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;
	highest_tmp_ref = 0;


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
				if(tmp_payload->tmp_ref > highest_tmp_ref) {
					highest_tmp_ref = tmp_payload->tmp_ref;
				}
			}
			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	return(highest_tmp_ref);
}

/****************************************************************************/

uint16_t get_highest_tmp_ref_special(const pes_holder_t * seq_pes_holder) {

	/* Markers */
	int seq_header;
	int next_seq_header;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;

	/* Highest tmp ref */
	uint16_t highest_tmp_ref;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;
	highest_tmp_ref = 0;


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


			if(tmp_payload->payload_type == I_FRAME ||
				tmp_payload->payload_type == P_FRAME ||
				tmp_payload->payload_type == B_FRAME) {
				if(tmp_payload->tmp_ref > highest_tmp_ref) {
					highest_tmp_ref = tmp_payload->tmp_ref;
				}
			}
			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	return(highest_tmp_ref);
}


/*************************************************************************/



uint16_t get_second_highest_tmp_ref(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref) {

	/* Markers */
	int seq_header;
	int next_seq_header;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;

	/* Highest tmp ref */
	uint16_t highest_tmp_ref;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;
	highest_tmp_ref = 0;


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
				if(tmp_payload->tmp_ref > highest_tmp_ref && tmp_payload->tmp_ref < tmp_ref) {
					highest_tmp_ref = tmp_payload->tmp_ref;
				}
			}
			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	return(highest_tmp_ref);
}




/*************************************************************************/

ticks_t get_time_of_tmp_ref(const pes_holder_t * seq_pes_holder, int tmp_ref ) {

	/* Markers */
	int seq_header;
	int next_seq_header;
	int gotit;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;

	/* Time */
	ticks_t time;


	/* Counter for going back wards */
	int counter;

	/* High tmp ref */
	uint16_t highest_tmp_ref;

	/* Init */
	seq_header = 0;
	next_seq_header = 0;
	time = 0;
	counter = 0;
	gotit = 0;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	highest_tmp_ref = get_highest_tmp_ref_special(tmp_pes_holder);

	if(tmp_ref > highest_tmp_ref) {
		tmp_ref = tmp_ref - highest_tmp_ref - 1;
		seq_pes_holder = next_seq_holder(seq_pes_holder);

		/* Check if we have a repair if so don't return the time */
		if(seq_pes_holder->repaired || seq_pes_holder->next->repaired) {
			return(0);
		}

		tmp_pes_holder = (pes_holder_t *)seq_pes_holder;
	}


	if(tmp_ref < 0) {
		//printf("Negative tmp ref\n");

		if(seq_pes_holder->repaired || seq_pes_holder->next->repaired) {
			/* Don't return the previous time if we had a repair */
			return(0);
		}

		while(tmp_pes_holder->previous) {
			//printf("SEQ %i I %i\n",tmp_pes_holder->seq_present,tmp_pes_holder->i_frame_present);
			if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
				gotit = 1;
				seq_pes_holder=tmp_pes_holder;
				break;
			}
			counter++;
			tmp_pes_holder = tmp_pes_holder->previous;
		}
		if(tmp_pes_holder->seq_present) {
			gotit = 1;
			seq_pes_holder=tmp_pes_holder;
		}

		if(gotit) {
			highest_tmp_ref = get_highest_tmp_ref(tmp_pes_holder);
		} else if(counter > 5) {
			//printf("Only %i  \n", counter);
			highest_tmp_ref = get_highest_tmp_ref_special(tmp_pes_holder);
			seq_header = 1;
		} else {
			//printf("No pes holder\n");
			return(0);
		}
		//printf("Higest tmpref, TMP ref is %i \n", highest_tmp_ref,  tmp_ref);
		/* Remember tmp_ref is negative */
		tmp_ref = highest_tmp_ref + tmp_ref + 1;
	}



	//printf("Tmp Ref is %i\n", tmp_ref);

	while(tmp_pes_holder) {
		if(tmp_pes_holder->gap) {
			break;
		}

		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {

			/* Run into the next SEQ header */
			/* FIXME will not work when we have made a backward
			jump */
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
				//printf("tmp payload tmp_ref %i\n",tmp_payload->tmp_ref);
				if(tmp_payload->tmp_ref == tmp_ref) {
					time = tmp_pes_holder->time;
					return(time);
				}
			}
			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			//printf("No time 1\n");
			return(0);
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}
	//printf("No time\n");
	return(0);
}

/*************************************************************************/

int set_time_of_tmp_ref(const pes_holder_t * seq_pes_holder, int tmp_ref, ticks_t time) {

	/* Markers */
	int seq_header;
	int next_seq_header;
	int gotit;

	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;

	/* Counter for going back wards */
	int counter;

	/* High tmp ref */
	uint16_t highest_tmp_ref;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;
	counter = 0;
	gotit = 0;


	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	if(tmp_ref < 0) {
		//printf("Negative tmp ref\n");

		while(tmp_pes_holder->previous) {
			//printf("SEQ %i I %i\n",tmp_pes_holder->seq_present,tmp_pes_holder->i_frame_present);
			if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
				gotit = 1;
				seq_pes_holder=tmp_pes_holder;
				break;
			}
			counter++;
			tmp_pes_holder = tmp_pes_holder->previous;
		}
		if(tmp_pes_holder->seq_present) {
			gotit = 1;
			seq_pes_holder=tmp_pes_holder;
		}

		if(gotit) {
			highest_tmp_ref = get_highest_tmp_ref(tmp_pes_holder);
		} else if(counter > 5) {
			//printf("Only %i  \n", counter);
			highest_tmp_ref = get_highest_tmp_ref_special(tmp_pes_holder);
			seq_header = 1;
		} else {
			//printf("No pes holder\n");
			return(0);
		}
		//printf("Higest tmpref, TMP ref is %i \n", highest_tmp_ref,  tmp_ref);
		/* Remember tmp_ref is negative */
		tmp_ref = highest_tmp_ref + tmp_ref + 1;
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
				if(tmp_payload->tmp_ref == tmp_ref) {
					tmp_pes_holder->time = time;
					return(1);
				}
			}
			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			return(0);
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	return(0);
}

/*************************************************************************/

payload_type_t get_frame_type_of_tmp_ref(const pes_holder_t * seq_pes_holder, int tmp_ref) {

	/* Markers */
	int seq_header;
	int next_seq_header;
	int gotit;

	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;

	/* Counter for going back wards */
	int counter;

	/* High tmp ref */
	uint16_t highest_tmp_ref;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;
	counter = 0;
	gotit = 0;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	if(tmp_ref < 0) {

		while(tmp_pes_holder->previous) {
			if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
				gotit = 1;
				break;
			}
			counter++;
			tmp_pes_holder = tmp_pes_holder->previous;
		}
		if(gotit) {
			highest_tmp_ref = get_highest_tmp_ref(tmp_pes_holder);
		} else if(counter > 5) {
			highest_tmp_ref = get_highest_tmp_ref_special(tmp_pes_holder);
			seq_header = 1;
		} else {
			return(UNKNOWN_PAYLOAD);
		}
		/* Remember tmp_ref is negative */
		tmp_ref = highest_tmp_ref + tmp_ref + 1;
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
				if(tmp_payload->tmp_ref == tmp_ref) {
					return(tmp_payload->payload_type);
				}
			}
			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			return(UNKNOWN_PAYLOAD);
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	return(UNKNOWN_PAYLOAD);
}

/*************************************************************************/

int remove_frame(pes_holder_t * seq_pes_holder, uint16_t tmp_ref) {

	/* Markers */
	int seq_header;
	int next_seq_header;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;

	/* Time */
	ticks_t time;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;
	time = 0;


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
				if(tmp_payload->tmp_ref == tmp_ref) {
					/* Remove this frame */
					if(tmp_pes_holder->previous && tmp_pes_holder->next) {
						tmp_pes_holder->previous->next = tmp_pes_holder->next;
						tmp_pes_holder->next->previous = tmp_pes_holder->previous;
						free_pes_holder(tmp_pes_holder, 1);
						return(1);
					} else {
						return(0);
					}
				}
			}
			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			return(0);
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	return(0);

}

/****************************************/


int add_frame(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder, uint16_t tmp_ref) {

	/* Markers */
	int seq_header;
	int next_seq_header;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	payload_t * tmp_payload_2;
	pes_holder_t * tmp_pes_holder;
	pes_holder_t * new_pes_holder;


	/* Picture info */
	picture_info_t picture_info_before;


	/* Values to set */
	int top_field_first;
	int repeat_first_field;

	field_type_t last_field;

	/* Time */
	ticks_t time;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;
	time = 0;


	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;
	LOG_DEVDIAG1("Adding frame %i \n", tmp_ref);
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
				if(tmp_payload->tmp_ref == tmp_ref) {
					/* Duplicate this frame */
					new_pes_holder = duplicate_pes_holder(tmp_pes_holder);
					tmp_payload_2 = new_pes_holder->payload;
					get_picture_info_tmp_ref(tystream, seq_pes_holder, tmp_ref, &picture_info_before);
					//get_picture_info_tmp_ref(tystream, seq_pes_holder, tmp_ref + 1, &picture_info_after);

					/* Figure out what field should be the top field of our added frame */

					if(picture_info_before.repeat_first_field && picture_info_before.top_field_first) {
						last_field = TOP_FIELD;
					} else if(picture_info_before.repeat_first_field && !picture_info_before.top_field_first) {
						last_field = BOTTOM_FIELD;
					} else if(!picture_info_before.repeat_first_field && picture_info_before.top_field_first) {
						last_field = BOTTOM_FIELD;
					} else { //(!picture_info_before.repeat_first_field && !picture_info_before.top_field_first) {
						last_field = TOP_FIELD;
					}

					if(last_field == BOTTOM_FIELD) {
						top_field_first = 1;
					} else {
						top_field_first = 0;
					}

					repeat_first_field = 0;

					while(tmp_payload_2) {
						if(tmp_payload_2->tmp_ref == tmp_ref && (tmp_payload_2->payload_type == B_FRAME ||
							tmp_payload_2->payload_type == P_FRAME || tmp_payload_2->payload_type == I_FRAME)) {
							LOG_DEVDIAG1("Setting tmp ref of frame %i\n", tmp_ref);
							tmp_payload_2->tmp_ref++;
							set_temporal_reference(tmp_payload_2->payload_data,tmp_payload_2->tmp_ref);
							LOG_DEVDIAG1("Tmp ref set to %i\n", get_temporal_reference(tmp_payload_2->payload_data));
							tmp_payload_2->tmp_ref_added = 1;

							/* Set the top field and repeat first field to the correct values */
							set_top_field_first(tystream, tmp_payload_2, top_field_first);
							set_repeat_first_field(tystream, tmp_payload_2, repeat_first_field);
					
							if(repeat_first_field) {
								if(top_field_first) {
									tmp_payload_2->fields = TBT;
								} else {
									tmp_payload_2->fields = BTB;
								}
							} else {
								if(top_field_first) {
									tmp_payload_2->fields = TB;
								} else {
									tmp_payload_2->fields = BT;
								}
							}

							if(tmp_pes_holder->next) {
								new_pes_holder->next = tmp_pes_holder->next;
								new_pes_holder->next->previous = new_pes_holder;
								tmp_pes_holder->next = new_pes_holder;
								new_pes_holder->previous = tmp_pes_holder;
							} else {
								tmp_pes_holder->next = new_pes_holder;
								new_pes_holder->previous = tmp_pes_holder;
							}
							return(1);
						}
						tmp_payload_2 = tmp_payload_2->next;
					}
					free_pes_holder(new_pes_holder, 1);
					return(0);

				}
			}
			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			return(0);
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	return(0);
}

/*************************************************************************/


uint16_t tmp_ref_of_payload(const payload_t * payload) {


	payload_t * tmp_payload;

	tmp_payload =(payload_t *)payload;

	while(tmp_payload) {

		if(tmp_payload->payload_type == I_FRAME ||
			tmp_payload->payload_type == P_FRAME ||
			tmp_payload->payload_type == B_FRAME) {
			return(tmp_payload->tmp_ref);
		}

		tmp_payload = tmp_payload->next;
	}

	return(0);
}

