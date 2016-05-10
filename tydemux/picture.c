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

int get_picture_info(tystream_holder_t * tystream, payload_t * payload, picture_info_t * picture_info) {


	/* Sizes and offsets */
	int offset;

	/* Pointer to magic with */
	uint8_t * pt1;


	/* Find the picture coding extension in the picture buffer */
	offset = find_extension_in_payload(tystream, payload, (uint8_t)0x08);

	if(offset == -1) {
		//printf("Frame off is -1\n");
		return(0);
	}

	pt1 = payload->payload_data;
	pt1 = pt1 + offset + 4 + 3;

	/* Top field first */
	picture_info->top_field_first = getbit(pt1, 0);

	/* Repeat first field */
	picture_info->repeat_first_field = getbit(pt1, 6);

	/* Progressive frame */
	picture_info->progressive_frame = getbit(pt1, 8);

	return(1);

}


int check_fix_progressive_frame(tystream_holder_t * tystream, payload_t * payload) {


	/* Sizes and offsets */
	int offset;

	/* Pointer to magic with */
	uint8_t * pt1;



	offset = find_extension_in_payload(tystream, payload, (uint8_t)0x08);

	if(offset == -1) {
		//printf("Frame off is -1\n");
		return(0);
	}

	pt1 = payload->payload_data;
	pt1 = pt1 + offset + 4 + 3;

	/* Top field first */
	setbit(pt1, 0, 0);

	/* Repeat first field */
	setbit(pt1, 6, 0);

	/* Progressive frame */
	setbit(pt1, 8, 1);

	return(1);

}


int set_repeat_first_field(tystream_holder_t * tystream, payload_t * payload, int value) {


	/* Sizes and offsets */
	int offset;

	/* Pointer to magic with */
	uint8_t * pt1;

	offset = find_extension_in_payload(tystream, payload, (uint8_t)0x08);

	if(offset == -1) {
		//printf("Frame off is -1\n");
		return(0);
	}

	pt1 = payload->payload_data;
	pt1 = pt1 + offset + 4 + 3;


	/* Repeat first field */
	setbit(pt1, 6, value);

	if(value) {
		/* Progressive frame needs to be one */
		/* FIXME This is not the correct way */
		setbit(pt1, 8, 1);
	}

	return(1);

}


int set_top_field_first(tystream_holder_t * tystream, payload_t * payload, int value) {


	/* Sizes and offsets */
	int offset;

	/* Pointer to magic with */
	uint8_t * pt1;

	offset = find_extension_in_payload(tystream, payload, (uint8_t)0x08);

	if(offset == -1) {
		//printf("Frame off is -1\n");
		return(0);
	}

	pt1 = payload->payload_data;
	pt1 = pt1 + offset + 4 + 3;



	/* Top field first */
	setbit(pt1, 0, value);


	return(1);

}



int get_repeat_first_field(tystream_holder_t * tystream, payload_t * payload) {


	/* Sizes and offsets */
	int offset;

	/* Pointer to magic with */
	uint8_t * pt1;


	/* Find the picture coding extension in the picture buffer */
	offset = find_extension_in_payload(tystream, payload, (uint8_t)0x08);

	if(offset == -1) {
		//printf("Frame off is -1\n");
		return(0);
	}

	pt1 = payload->payload_data;
	pt1 = pt1 + offset + 4 + 3;

	/* Repeat first field */
	return(getbit(pt1, 6));

}

int check_fields_in_seq(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* Maker */
	int gotit;


	/* Iteration */
	uint16_t counter;

	/* Pes holder */
	pes_holder_t * tmp_pes_holder;

	/* Payload holder*/
	payload_t * tmp_payload;

	/* Pic info */
	picture_info_t picture_info;

	/* Number of frame rate errors */
	int error;

	/* Number of frames */
	uint16_t highest_tmp_ref;

	/* Fields */
	field_type_t last_field;
	int result;

	/* Init */
	error = 0;
	gotit = 0;


	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	gotit = 0;
	tmp_payload = tmp_pes_holder->payload;
	while(tmp_payload) {
		/* Check SEQ header */
		if(tmp_payload->payload_type == SEQ_HEADER) {
			gotit = 1;
			break;
		}
		tmp_payload = tmp_payload->next;
	}

	if(!gotit) {
		LOG_DEVDIAG1("Didn't find SEQ, %i\n", tmp_pes_holder->seq_present);
		return(-1);
	}

	//printf("In check fields\n");

	/* Get the last frames tmp ref number */
	highest_tmp_ref = get_highest_tmp_ref(tmp_pes_holder);

	/* Get the time of the first frame in the next SEQ */
	//time_to_meet = get_time_of_tmp_ref(tmp_pes_holder, highest_tmp_ref + 1);

	//if(!time_to_meet) {
		/* FIXME */
	//	printf("Repair of stream - func not implemented!\n")
	//	return(-1);
	//}

	if(seq_pes_holder->repaired || tystream->last_field == UNKNOW_FIELD) {

		result = get_picture_info_tmp_ref(tystream, seq_pes_holder, 0, &picture_info);

		if(!result) {
			return(-1);
		}

		if(picture_info.top_field_first) {
			last_field = BOTTOM_FIELD;
		} else {
			last_field = TOP_FIELD;
		}
	} else {
		last_field = tystream->last_field;
	}

	/* Okay so lets check the integrety of our fields in the SEQ */


	for(counter = 0 ; counter < highest_tmp_ref + 1; counter++) {

		result =  get_picture_info_tmp_ref(tystream, seq_pes_holder,  counter, &picture_info);

		if(!result) {
			LOG_DEVDIAG("Error fetching pic info\n");
			return(-1);
		}


		if(last_field == BOTTOM_FIELD && !picture_info.top_field_first) {
			LOG_DEVDIAG("Error in integrity fields not matching bot to bot\n");
			error++;
		}

		if(last_field == TOP_FIELD && picture_info.top_field_first) {
			LOG_DEVDIAG("Error in integrity fields not matching top to top\n");
			error++;
		}

		if(picture_info.repeat_first_field && !picture_info.progressive_frame) {
			LOG_DEVDIAG("Error in progressive frame\n");
			error++;
		}

		if(picture_info.top_field_first && picture_info.repeat_first_field) {
			last_field = TOP_FIELD;
		} else if (picture_info.top_field_first && !picture_info.repeat_first_field) {
			last_field = BOTTOM_FIELD;
		} else if (!picture_info.top_field_first && picture_info.repeat_first_field) {
			last_field = BOTTOM_FIELD;
		} else { /* !picture_info.top_field_first && !picture_info.repeat_first_field */
			last_field = TOP_FIELD;
		}
	}

	tystream->last_field = last_field;

	return(error);
}




int get_picture_info_tmp_ref(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, int tmp_ref, picture_info_t * picture_info ) {

	/* Markers */
	int seq_header;
	int next_seq_header;
	int gotit;
	int return_value;


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
		if(seq_pes_holder->repaired) {
			return(0);
		}

		tmp_pes_holder = (pes_holder_t *)seq_pes_holder;
	}


	if(tmp_ref < 0) {
		//printf("Negative tmp ref\n");

		if(seq_pes_holder->repaired) {
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
		} else {
			//printf("No pes holder\n");
			return(0);
		}
		//printf("Higest tmpref, TMP ref is %i \n", highest_tmp_ref,  tmp_ref);
		/* Remember tmp_ref is negative */
		tmp_ref = highest_tmp_ref + tmp_ref + 1;
	}

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;


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
					return_value = get_picture_info(tystream, tmp_payload, picture_info);

					if(picture_info->repeat_first_field) {
						if(picture_info->top_field_first) {
							tmp_payload->fields = TBT;
						} else {
							tmp_payload->fields = BTB;
						}
					} else {
						if(picture_info->top_field_first) {
							tmp_payload->fields = TB;
						} else {
							tmp_payload->fields = BT;
						}
					}

					return(return_value);
					//return(get_picture_info(tystream, tmp_payload, picture_info));
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

int set_repeat_first_field_of_tmp_ref(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, uint16_t tmp_ref, int value) {



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

		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {

			/* Run into the next SEQ header */
			if(tmp_payload->payload_type == SEQ_HEADER
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
					set_repeat_first_field(tystream, tmp_payload, value);
					return(1);
				}
			}

			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	return(0);

}


field_type_t set_top_field_first_of_tmp_ref(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, uint16_t tmp_ref, field_type_t  last_field) {


	/* Markers */
	int seq_header;
	int next_seq_header;

	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;

	/* Values to set and get */
	int value;
	int repeat;

	/* Init */
	seq_header = 0;
	next_seq_header = 0;

	//printf("Last field is %i\n", last_field);

	if(last_field == BOTTOM_FIELD) {
		/* Top field first == 1 */
		value = 1;
	} else {
		/* Top field first == 0 */
		value = 0;
	}

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	while(tmp_pes_holder) {

		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {

			/* Run into the next SEQ header */
			if(tmp_payload->payload_type == SEQ_HEADER
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

					set_top_field_first(tystream, tmp_payload, value);
					repeat = get_repeat_first_field(tystream, tmp_payload);

					if(value && repeat) {
						last_field = TOP_FIELD;
					} else if (!value && repeat) {
						last_field = BOTTOM_FIELD;
					} else if (value && !repeat) {
						last_field = BOTTOM_FIELD;
					} else if (!value && !repeat) {
						last_field = TOP_FIELD;
					}

					return(last_field);
				}
			}

			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	return(UNKNOW_FIELD);

}

