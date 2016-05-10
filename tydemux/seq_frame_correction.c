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

/* Internal */

//static int control_i_frame_present(const pes_holder_t * seq_pes_holder);
static int repair_frame_time(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, tmp_ref_frame_t * frame_array);
static int remove_dup_frame(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref);
static uint16_t tmp_ref_present(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder,
	uint16_t tmp_ref, tmp_ref_frame_t * frame_array);
static int16_t is_duplicate(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref);
static pes_holder_t * find_first_frame_missing_seq(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref);
static pes_holder_t * fetch_next_avaliable_frame(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref, payload_type_t frame_type);
static pes_holder_t * find_first_frame_missing_seq_2(const pes_holder_t * seq_pes_holder,
			pes_holder_t * missing_seq_pes_holder, uint16_t dup_tmp_ref,
			ticks_t dup_frame_time, payload_type_t dup_frame_type, ticks_t first_time);

/********************************************************************/

/* This is a very hard coded way of fixing the problem !! need to make a better one FIXME */

int correct_underflow_in_frames(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder, int nr_missing_frames, int last_p_frame_missing) {

	/* Tmp ref and tmp ref counting */
	uint16_t counter;
	uint16_t highest_tmp_ref;
	uint16_t second_highest_tmp_ref;
	int result_remaining_seq;
	int result;

	/* Markers */
	int need_to_fix_tmp_ref;
	int gotit;
	int not_repaired;

	/* Array to hold all frames - tmp_ref*/
	tmp_ref_frame_t * frame_array;

	/* Pes holders to insert */
	pes_holder_t * pes_holder_insert;

	/* Tmp pes holder for stray check */
	pes_holder_t * tmp_pes_holder;

	/* Payload of pes we insert */
	payload_t * tmp_payload;



	payload_type_t payload_type_higest_tmp_ref;
	payload_type_t payload_type_second_higest_tmp_ref;

	highest_tmp_ref = get_highest_tmp_ref(seq_pes_holder);
	second_highest_tmp_ref = get_second_highest_tmp_ref(seq_pes_holder, highest_tmp_ref);

	//printf("Second is %i, First is %i\n", second_highest_tmp_ref ,highest_tmp_ref);

	if(highest_tmp_ref - second_highest_tmp_ref > 50) {
		/* Good guess that this is actually a stray tmp_ref numbering and not a real error
		FIXME this one is experimental */
		tmp_pes_holder = (pes_holder_t *)seq_pes_holder;
		remove_frame(tmp_pes_holder,highest_tmp_ref);
		//print_seq(tmp_pes_holder);
		result = check_nr_frames_tmp_ref(tmp_pes_holder);
		if(!result) {
			/* success we return */
			return(0);
		}
		if(result > 0) {
			/* Fail we have a overflow and we don't fix it FIXME */
			return(-1);
		} else {
			return(correct_underflow_in_frames(tystream, tmp_pes_holder, result, 0));
		}
	}


	/* Okay so we think we are okay here but we aint - we will need to check if the highest
	tmp_ref is a B or a P frame if it's a B -frame we need to add frames beyond the highest
	tmp_ref */

	payload_type_higest_tmp_ref = get_frame_type_of_tmp_ref(seq_pes_holder, highest_tmp_ref);

	if(payload_type_higest_tmp_ref == B_FRAME) {
		/* Hmm so it was a B-Frame hence a P-frame is missing */
		payload_type_second_higest_tmp_ref = get_frame_type_of_tmp_ref(seq_pes_holder, highest_tmp_ref - 1);
		if(payload_type_second_higest_tmp_ref == P_FRAME) {
			highest_tmp_ref = highest_tmp_ref + 2;
		}
#if 0
		else if (payload_type_second_higest_tmp_ref == UNKNOWN_PAYLOAD) {
			/* This means that we really can't make a good check
			We habe a a lot of frames missing - even to that extent that the frame before this one
			is missing */

		}
#endif
		else {
			highest_tmp_ref++;
		}
	}

	/* WARNING MEM LEEK */
	frame_array = (tmp_ref_frame_t *)malloc(sizeof(tmp_ref_frame_t) * (highest_tmp_ref + 1));
	memset(frame_array, 0,sizeof(tmp_ref_frame_t) * (highest_tmp_ref + 1));

	for(counter = 0; counter < highest_tmp_ref + 1; counter++) {
		frame_array[counter].picture_info = (picture_info_t *)malloc(sizeof(picture_info_t));
		memset(frame_array[counter].picture_info, 0,sizeof(picture_info_t));
		frame_array[counter].present = 0;
	}

	for(counter = 0; counter < highest_tmp_ref + 1; counter++) {
		if((tmp_ref_present(tystream, seq_pes_holder, counter, frame_array)) > 1) {
			/* Okay so we had a under run but a dup anyway happens in some really funky stream it's actually a whole gop
			that is missing we had a huge frame drop - hence we need so see if we have any duplicated frames
			hence we will remove this frame */
			remove_dup_frame(seq_pes_holder, counter);
			tmp_ref_present(tystream, seq_pes_holder, counter, frame_array);
		}
	}

	LOG_DEVDIAG1("Part 1 - fix underflow -- %i\n", seq_pes_holder->next->chunk_nr);
	//print_seq(seq_pes_holder);


	/* Fix missing frames */
	need_to_fix_tmp_ref = 0;
	for(counter = 0; counter < highest_tmp_ref + 1; counter++) {
		if(!frame_array[counter].present) {


			pes_holder_insert = fetch_next_avaliable_frame(seq_pes_holder, counter, frame_array[counter].frame_type);

			if(!pes_holder_insert) {
				LOG_DEVDIAG("Unable to fix missing frame - 1\n");
				return(-1);
			}

			pes_holder_insert = duplicate_pes_holder(pes_holder_insert);

			tmp_payload = pes_holder_insert->payload;
			gotit = 0;
			while(tmp_payload) {
				if(tmp_payload->payload_type == frame_array[counter].frame_type) {
					set_temporal_reference(tmp_payload->payload_data, counter);
					tmp_payload->tmp_ref = counter;
					gotit = 1;
					break;
				}
				tmp_payload = tmp_payload->next;
			}


			if(!gotit) {
				LOG_DEVDIAG("Unable to fix missing frame - 2\n");
				return(-1);
			}

			insert_pes_holder(seq_pes_holder, pes_holder_insert, counter);

			need_to_fix_tmp_ref = 1;
		}
	}


	if(!check_nr_frames_tmp_ref(seq_pes_holder)) {
		/* We actually can fail one time when checking under flow this is due
		to the fact that we can have "guessed" the higest_tmp_ref wrong - 
		there is now way (well it's hard) to make a good guess hence
		checking it twise is easier */

		for(counter = 0; counter < highest_tmp_ref + 1; counter++) {
			free(frame_array[counter].picture_info);
		}

		free(frame_array);


		highest_tmp_ref = get_highest_tmp_ref(seq_pes_holder);
		second_highest_tmp_ref = get_second_highest_tmp_ref(seq_pes_holder, highest_tmp_ref);


		/* Okay so we think we are okay here but we aint - we will need to check if the highest
		tmp_ref is a B or a P frame if it's a B -frame we need to add frames beyond the highest
		tmp_ref */

		payload_type_higest_tmp_ref = get_frame_type_of_tmp_ref(seq_pes_holder, highest_tmp_ref);

		if(payload_type_higest_tmp_ref == B_FRAME) {
			/* Hmm so it was a B-Frame hence a P-frame is missing */
			payload_type_second_higest_tmp_ref = get_frame_type_of_tmp_ref(seq_pes_holder, highest_tmp_ref - 1);
			if(payload_type_second_higest_tmp_ref == P_FRAME) {
				highest_tmp_ref = highest_tmp_ref + 2;
			}
#if 0
			else if (payload_type_second_higest_tmp_ref == UNKNOWN_PAYLOAD) {
				/* This means that we really can't make a good check
				We habe a a lot of frames missing - even to that extent that the frame before this one
				is missing */

			}
#endif
			else {
				highest_tmp_ref++;
			}
		}

		frame_array = (tmp_ref_frame_t *)malloc(sizeof(tmp_ref_frame_t) * (highest_tmp_ref + 1));
		memset(frame_array, 0,sizeof(tmp_ref_frame_t) * (highest_tmp_ref + 1));

		for(counter = 0; counter < highest_tmp_ref + 1; counter++) {
			frame_array[counter].picture_info = (picture_info_t *)malloc(sizeof(picture_info_t));
			memset(frame_array[counter].picture_info, 0,sizeof(picture_info_t));
			frame_array[counter].present = 0;
		}

		for(counter = 0; counter < highest_tmp_ref + 1; counter++) {
			if((tmp_ref_present(tystream, seq_pes_holder, counter, frame_array)) > 1) {
				/* Okay so we had a under run but a dup anyway happens in some really funky stream it's actually a whole gop
				that is missing we had a huge frame drop - hence we need so see if we have any duplicated frames
				hence we will remove this frame */
				remove_dup_frame(seq_pes_holder, counter);
				tmp_ref_present(tystream, seq_pes_holder, counter, frame_array);
			}
		}

		LOG_DEVDIAG1("Part 1 - fix underflow -- %i\n", seq_pes_holder->next->chunk_nr);
		//print_seq(seq_pes_holder);


		/* Fix missing frames */
		for(counter = 0; counter < highest_tmp_ref + 1; counter++) {
			if(!frame_array[counter].present) {


				pes_holder_insert = fetch_next_avaliable_frame(seq_pes_holder, counter, frame_array[counter].frame_type);

				if(!pes_holder_insert) {
					LOG_DEVDIAG("Unable to fix missing frame - 1\n");
					return(-1);
				}

				pes_holder_insert = duplicate_pes_holder(pes_holder_insert);

				tmp_payload = pes_holder_insert->payload;
				gotit = 0;
				while(tmp_payload) {
					if(tmp_payload->payload_type == frame_array[counter].frame_type) {
						set_temporal_reference(tmp_payload->payload_data, counter);
						tmp_payload->tmp_ref = counter;
						gotit = 1;
						break;
					}
					tmp_payload = tmp_payload->next;
				}


				if(!gotit) {
					LOG_DEVDIAG("Unable to fix missing frame - 2\n");
					return(-1);
				}

				insert_pes_holder(seq_pes_holder, pes_holder_insert, counter);

				need_to_fix_tmp_ref = 1;
			}
		}
	}

	if(need_to_fix_tmp_ref) {
		if(ty_debug >= 7) {
			//print_seq(seq_pes_holder);
		}
		fix_tmp_ref(tystream, seq_pes_holder);
		if(ty_debug >= 7) {
			//print_seq(seq_pes_holder);
		}
	}

	/* Fix time and field count */

	if(seq_pes_holder->seq_added) {
		/* HARD CODED FIXME */
		frame_array[2].present = 0;
	}


	not_repaired = repair_frame_time(tystream, seq_pes_holder, frame_array);

	while(not_repaired) {
		not_repaired = repair_frame_time(tystream, seq_pes_holder, frame_array);
	}
	if(ty_debug >= 7) {
		//print_seq(seq_pes_holder);
	}

	/* Now check the present SEQ so it's tmp ref is correct */

	result_remaining_seq = check_temporal_reference(seq_pes_holder);
	LOG_DEVDIAG1("Remaining SEQ error underflow is %i\n", result_remaining_seq);


	for(counter = 0; counter < highest_tmp_ref + 1; counter++) {
		free(frame_array[counter].picture_info);
	}

	free(frame_array);


	return(result_remaining_seq);

}

/*********************************************************************/

/* Okay this is most likely a SEQ header missing - if not we are currently toast FIXME */

int correct_overflow_in_frames(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, int nr_of_overflow_frames) {

	/* Marker */
	int gotit;
	int free_gop;
	
	/* Payload */
	payload_t * tmp_payload;
	payload_t * payload_gop;
	payload_t * payload_seq;
	payload_t * payload_i_frame;
//	payload_t * free_payload;

	/* Tmp ref and tmp ref counting */
	uint16_t counter;
	uint16_t highest_tmp_ref;
	int16_t dup_return;
	int16_t tmp_dup_return;
	int result_remaining_seq;

	/* For extra check of first missing seq frame */
	payload_type_t dup_frame_type;
	ticks_t dup_frame_time;
	ticks_t first_frame_time;
	uint16_t dup_tmp_ref;

	/* Pes holder of frame after missing seq */
	pes_holder_t * tmp_first_frame_missing_seq_pes_holder = NULL;
	pes_holder_t * first_frame_missing_seq_pes_holder = NULL;
	pes_holder_t * missing_seq_pes_holder = NULL;



	/* Init */
	gotit = 0;
	dup_return = 0;

	highest_tmp_ref = get_highest_tmp_ref(seq_pes_holder);


	for(counter=0; counter < highest_tmp_ref + 1; counter++) {

		if((tmp_dup_return = is_duplicate(seq_pes_holder, counter))) {
			gotit = 1;
			if(dup_return == 0) {
				dup_return = tmp_dup_return;
			}
			if(dup_return >= tmp_dup_return) {
				dup_tmp_ref = counter;
			}
		}
	}

	if(!gotit) {
		LOG_DEVDIAG("Internal 1 - Unable to fix over flow in frames\n");
		return(0);
	}

	/* Now we will need to find the first frame present in next seq
	(the seq that is missing the seq header) and create a seq just before
	that frame */


	tmp_first_frame_missing_seq_pes_holder = find_first_frame_missing_seq(seq_pes_holder, dup_tmp_ref);

	if(!tmp_first_frame_missing_seq_pes_holder) {
		LOG_DEVDIAG("Internal 2.1 - Unable to fix over flow in frames\n");
		return(0);
	}


	dup_frame_type = get_frame_type_of_tmp_ref(seq_pes_holder,dup_tmp_ref);

	dup_frame_time = tmp_first_frame_missing_seq_pes_holder->time;
	first_frame_time = get_time_of_tmp_ref(seq_pes_holder, dup_tmp_ref );

	if(dup_frame_time == 0 || dup_frame_type == UNKNOWN_PAYLOAD || first_frame_time == 0) {
		/* Okay not much to here we can't do a better search */
		first_frame_missing_seq_pes_holder = tmp_first_frame_missing_seq_pes_holder;
	} else {
		first_frame_missing_seq_pes_holder = find_first_frame_missing_seq_2(seq_pes_holder,
			tmp_first_frame_missing_seq_pes_holder, dup_tmp_ref, dup_frame_time, dup_frame_type, first_frame_time);
	}




	if(!first_frame_missing_seq_pes_holder) {
		LOG_DEVDIAG("Internal 2 - Unable to fix over flow in frames\n");
		return(0);
	}


	if(first_frame_missing_seq_pes_holder->i_frame_present) {
		missing_seq_pes_holder = duplicate_pes_holder(seq_pes_holder);

		gotit = 0;
		free_gop = 0;
		tmp_payload = first_frame_missing_seq_pes_holder->payload;

		while(tmp_payload) {
			if(tmp_payload->payload_type == I_FRAME) {
				payload_i_frame = tmp_payload;
				break;
			}
			tmp_payload = tmp_payload->next;
		}

		if(first_frame_missing_seq_pes_holder->gop_present) {
			tmp_payload = first_frame_missing_seq_pes_holder->payload;
			while(tmp_payload) {
				if(tmp_payload->payload_type == GOP_HEADER) {
					payload_gop = tmp_payload;
					free_gop = 1;
					break;
				}
				tmp_payload = tmp_payload->next;
			}
		} else {
			tmp_payload = missing_seq_pes_holder->payload;
			while(tmp_payload) {
				if(tmp_payload->payload_type == GOP_HEADER) {
					payload_gop = tmp_payload;
					break;
				}
				tmp_payload = tmp_payload->next;
			}

		}

		tmp_payload = missing_seq_pes_holder->payload;
		while(tmp_payload) {
			if(tmp_payload->payload_type == SEQ_HEADER) {
				payload_seq = tmp_payload;
				break;
			}
			tmp_payload = tmp_payload->next;
		}

		tmp_payload = first_frame_missing_seq_pes_holder->payload;

		while(tmp_payload) {
			if(tmp_payload->payload_type == PES_VIDEO) {
				gotit = 1;
				break;
			}
			tmp_payload = tmp_payload->next;
		}

		if(gotit) {
			tmp_payload->next = payload_seq;
			payload_seq->next = payload_gop;
			payload_seq->previous = tmp_payload;
			payload_gop->next = payload_i_frame;
			payload_gop->previous = payload_seq;
			payload_i_frame->next = NULL;
			payload_i_frame->previous = payload_gop;
		} else {
			first_frame_missing_seq_pes_holder->payload = payload_seq;
			payload_seq->previous = NULL;
			payload_gop->next = payload_i_frame;
			payload_gop->previous = payload_seq;
			payload_i_frame->next = NULL;
			payload_i_frame->previous = payload_gop;
		}

		first_frame_missing_seq_pes_holder->seq_present = 1;
		first_frame_missing_seq_pes_holder->gop_present = 1;

		
		/* FIXME WHY DO WE FAIL WHEN WE FREE THIS ??? */
		/*tmp_payload = missing_seq_pes_holder->payload;
		free(missing_seq_pes_holder);

		while(tmp_payload) {

			free_payload = tmp_payload;
			tmp_payload = tmp_payload->next;

			if((free_payload->payload_type != SEQ_HEADER && free_payload->payload_type != GOP_HEADER) ||
				(free_payload->payload_type == GOP_HEADER && free_gop)){
				if(free_payload->payload_data != NULL) {
					free(free_payload->payload_data);
				}
				free(free_payload);
			}

		}*/
		//print_seq(seq_pes_holder);
		//print_seq(first_frame_missing_seq_pes_holder);



	} else {


		missing_seq_pes_holder = duplicate_pes_holder(seq_pes_holder);


		/* Insert the missing seq holder */
		missing_seq_pes_holder->previous = first_frame_missing_seq_pes_holder->previous;
		missing_seq_pes_holder->next = first_frame_missing_seq_pes_holder;

		first_frame_missing_seq_pes_holder->previous->next = missing_seq_pes_holder;
		first_frame_missing_seq_pes_holder->previous = missing_seq_pes_holder;

		/* Mark the SEQ as added so we can fix it later on */

		missing_seq_pes_holder->seq_added = 1;
		//print_seq(seq_pes_holder);
		//print_seq(missing_seq_pes_holder);

	}

	/* Now check the present SEQ so it's tmp ref is correct */

	result_remaining_seq = check_temporal_reference(seq_pes_holder);

	//printf("Remaining SEQ error overflow is %i\n", result_remaining_seq);

	return(result_remaining_seq);

}
#if 0
/*********************************************************************/

static int control_i_frame_present(const pes_holder_t * seq_pes_holder) {


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

			if(tmp_payload->payload_type == I_FRAME) {
				return(1);
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
#endif

/*********************************************************************/

static int repair_frame_time(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, tmp_ref_frame_t * frame_array) {

	/* BIG FIXME we don't take any notice if the frame we had inserted
	before had the progesive_frame set or not!!! We need to insert the
	right type of frame!! */

	/* Markers */
	int gotit;

	/* Tmp refs for repair */
	int16_t tmp_ref_first_broken;
	int16_t tmp_ref_first_original;

	/* Frames and field numbers  */
	int nr_frames_missing;
	int nr_fields_missing;
	int nr_of_fields;
	int nr_of_fields_original;
	int nr_2_field;
	int nr_3_field;

	/* Field types */
	field_type_t first_field;
	field_type_t last_field;

	field_type_t first_field_original;
	field_type_t last_field_original;


	/* Times */
	ticks_t time;
	ticks_t time_original;
	ticks_t time_of_break;

	uint16_t counter;
	uint16_t highest_tmp_ref;



	/* Init */
	tmp_ref_first_broken  = -1;
	tmp_ref_first_original = -1;



	highest_tmp_ref = get_highest_tmp_ref(seq_pes_holder);

	for(counter=0; counter < highest_tmp_ref + 1; counter++) {
		if(tmp_ref_first_broken == -1 && !frame_array[counter].present) {
			tmp_ref_first_broken = counter;
		}

		if(tmp_ref_first_broken != -1 && frame_array[counter].present) {
			tmp_ref_first_original = counter;
			break;
		}
	}

	//printf("Part 1 fix time frame\n");

	if(tmp_ref_first_original == -1) {
		tmp_ref_first_original = highest_tmp_ref + 1;
	}

	nr_frames_missing = tmp_ref_first_original - tmp_ref_first_broken;


	get_time_field_info(tystream, seq_pes_holder, tmp_ref_first_broken - 1, &time, &last_field,
		&first_field, &nr_of_fields, frame_array);
	//printf("Part 1.5 fix time frame\n");
	get_time_field_info(tystream, seq_pes_holder, tmp_ref_first_original, &time_original, &last_field_original,
		&first_field_original, &nr_of_fields_original, frame_array);
	//printf("Part 2 fix time frame\n");
	time_of_break = time + tystream->frame_tick + (nr_of_fields%2 * (tystream->frame_tick/2));

	//printf("Tmp ref first broken %i \n", tmp_ref_first_broken);

	//printf("Before break: time " I64FORMAT ", last_field %i, first_field %i, nr_of_fields %i\n",
	//	time, last_field, first_field, nr_of_fields);

	//printf("After break: time " I64FORMAT ", last_field %i, first_field %i, nr_of_fields %i\n",
	//	time_original, last_field_original, first_field_original, nr_of_fields_original);


	nr_fields_missing = (int)( (time_original - time_of_break) / (tystream->frame_tick/2) );

	/* Now find out how many 3 field and 2 field frames we have
	x == 2 field frames, y == 3 field frames

	2x + 3y = nr_fields_missing
	x + y  = nr_frames_missing

	x = 3 * nr_frames_missing - nr_fields_missing
	y = nr_fields_missing - 2 * nr_frames_missing
	*/

	nr_2_field = 3 * nr_frames_missing - nr_fields_missing;
	nr_3_field = nr_fields_missing - 2 * nr_frames_missing;

	//printf("Nr 2 %i Nr 3 %i\n",nr_2_field,nr_3_field);


	for(counter = tmp_ref_first_broken; counter < tmp_ref_first_original ; counter++) {
		if(nr_2_field > nr_3_field) {
			set_repeat_first_field_of_tmp_ref(tystream, seq_pes_holder, counter, 0);
			set_time_of_tmp_ref(seq_pes_holder, counter, time_of_break);
			time_of_break = time_of_break + tystream->frame_tick;
			nr_2_field--;
		} else {
			set_repeat_first_field_of_tmp_ref(tystream, seq_pes_holder, counter, 1);
			set_time_of_tmp_ref(seq_pes_holder, counter, time_of_break);
			time_of_break = time_of_break + tystream->frame_tick + tystream->frame_tick/2;
			nr_3_field--;
		}
	}
	//printf("Part 3 fix time frame\n");

	/* Now we will need to fix first_field_first */
	/* last_field == 0 -> bottom field
	   last_field == 1 -> top_field
	*/
	//printf("Last field is %i\n", last_field);
	for(counter = tmp_ref_first_broken; counter < tmp_ref_first_original ; counter++) {
			last_field = set_top_field_first_of_tmp_ref(tystream, seq_pes_holder, counter, last_field);
			frame_array[counter].present = 1;
	}

	//printf("Part 4 fix time frame\n");

	gotit = 0;

	for(counter=0; counter < highest_tmp_ref + 1; counter++) {
		if(!frame_array[counter].present) {
			gotit = 1;
			break;
		}
	}

	//printf("Part 5 fix time frame - gotit %i \n", gotit);
	return(gotit);

}
/**************************************************************************/

static int remove_dup_frame(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref) {

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
				if(tmp_payload->tmp_ref == tmp_ref) {
					/* Remove the frame*/
					tmp_pes_holder->previous->next = tmp_pes_holder->next;
					tmp_pes_holder->next->previous = tmp_pes_holder->previous;

					tmp_pes_holder->next = NULL;
					tmp_pes_holder->previous = NULL;
					free_pes_holder(tmp_pes_holder, 1);
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

/**************************************************************************/

/* This is a very hard coded way of fixing the problem !! need to make a better one FIXME */

static uint16_t tmp_ref_present(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, uint16_t tmp_ref, tmp_ref_frame_t * frame_array) {

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
				if(tmp_payload->tmp_ref == tmp_ref) {
					frame_array[tmp_ref].present++;
					frame_array[tmp_ref].frame_type = tmp_payload->payload_type;
					get_picture_info(tystream, tmp_payload, frame_array[tmp_ref].picture_info);
				}
			}
			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(frame_array[tmp_ref].present) {
		return(frame_array[tmp_ref].present);
	}

	frame_array[tmp_ref].present = 0;

	if(tmp_ref == 2) {
		frame_array[tmp_ref].frame_type = I_FRAME;
	} else if (tmp_ref == 0) {
		frame_array[tmp_ref].frame_type = B_FRAME;
	} else {
		if((tmp_ref + 1)%3) {
			frame_array[tmp_ref].frame_type = B_FRAME;
		} else {
			frame_array[tmp_ref].frame_type = P_FRAME;
		}
	}

	frame_array[tmp_ref].picture_info->top_field_first = 0;
	frame_array[tmp_ref].picture_info->repeat_first_field = 0;
	frame_array[tmp_ref].picture_info->progressive_frame = 0;

	return(0);
}

/**************************************************************************/

static int16_t is_duplicate(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref) {

	/* Markers */
	int seq_header;
	int next_seq_header;
	int duplicate;
	int16_t counter;

	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;
	duplicate = 0;
	counter = 0;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	while(tmp_pes_holder) {

		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {

			/* Run into the next SEQ header */
			if(tmp_payload->payload_type == SEQ_HEADER &&
				tmp_pes_holder != seq_pes_holder) {

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
				counter++;
				if(tmp_payload->tmp_ref == tmp_ref) {
					duplicate++;
				}

				if(duplicate >= 2) {
					return(counter);
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

/**************************************************************************/

static pes_holder_t * find_first_frame_missing_seq(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref) {

	/* Markers */
	int seq_header;
	int next_seq_header;
	int duplicate;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;
	pes_holder_t * p_frame_pes_holder;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;
	duplicate = 0;

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
					duplicate++;
					if(duplicate == 2) {
						break;
					}
				}
			}

			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header || duplicate == 2) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(duplicate != 2) {
		return(0);
	}

	/* The current posision is now the first tmp_ref that is
	duplicated, however since tmp_ref for a P/I frame is a head
	of a tmp_ref for B frames we may very well have a P frame
	before this frame that is beloing to this seq */

	p_frame_pes_holder = tmp_pes_holder->previous;
	if(p_frame_pes_holder) {
		tmp_payload = p_frame_pes_holder->payload;
		while(tmp_payload) {

			if(tmp_payload->payload_type == P_FRAME) {
				if(tmp_payload->tmp_ref == tmp_ref + 1 ||
					tmp_payload->tmp_ref == tmp_ref + 2 ||
					tmp_payload->tmp_ref == tmp_ref + 3) {
					/* We had a P Frame before the B Frame that did
					indeed belong to this seq */
					tmp_pes_holder = p_frame_pes_holder;
				}
			}

			tmp_payload = tmp_payload->next;
		}

	}
	return(tmp_pes_holder);

}


static pes_holder_t * find_first_frame_missing_seq_2(const pes_holder_t * seq_pes_holder,
			pes_holder_t * missing_seq_pes_holder, uint16_t dup_tmp_ref,
			ticks_t dup_frame_time, payload_type_t dup_frame_type, ticks_t first_time) {


	/* Markers */
	int seq_header;
	int next_seq_header;
	int duplicate;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;


	/* Init */
	seq_header = 0;
	next_seq_header = 0;
	duplicate = 0;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	while(tmp_pes_holder) {

		if(tmp_pes_holder == missing_seq_pes_holder) {
			/* It was actually the start of the missing seq */
			return(missing_seq_pes_holder);
		}

		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {

			/* Run into the next SEQ header */
			if(tmp_payload->payload_type == SEQ_HEADER
				&& tmp_pes_holder != seq_pes_holder) {

				next_seq_header = 1;
				return(missing_seq_pes_holder);
				break;
			}

			if(tmp_payload->payload_type == SEQ_HEADER) {
				seq_header = 1;
			}

			if((tmp_payload->payload_type == I_FRAME ||
				tmp_payload->payload_type == P_FRAME ||
				tmp_payload->payload_type == B_FRAME) &&
				seq_header) {

				if(duplicate) {
					if(dup_frame_type == tmp_payload->payload_type &&
						tmp_pes_holder->time > first_time &&
						tmp_payload->tmp_ref < dup_tmp_ref) {
						/* This one must be the real one -
						We got the fist frame of this tmp ref
						(dupicate == 1) they are of the same
						payload type yet tmp_ref is smaller
						that the dup_tmp_ref and at the same time
						at a higher time */
						//printf("Catch - 1\n");
						return(tmp_pes_holder);
					}

					if((dup_frame_type == I_FRAME || dup_frame_type == P_FRAME) &&
						dup_frame_time < tmp_pes_holder->time) {
						/* Here we have a I/P frame as dump_frame type
						the time of the tmp_pes_holder is higher then the
						one of the dup hence it's a P frame before e.g a duped
						B-Frame*/
						//printf("Catch - 2\n");
						return(tmp_pes_holder);
					}

					if(dup_frame_type == B_FRAME &&
						tmp_pes_holder->time < dup_frame_time &&
						tmp_payload->tmp_ref < dup_tmp_ref) {
						/* Here we have a B-Frame - it has a lower
						tmp_ref than the dup and a lower time and we
						have duplicate == 1 - hence it must be a real one */
						LOG_WARNING("This catch in tmp ref fix is a bit fuzzy \n if you have problems with the stream - please\n report it - SAVE THIS STREAM!\n");
						//printf("Catch - 3\n");
						return(tmp_pes_holder);
					}

					/* Okay if we have a I-Frame we must have a GOP at least if the tmp ref of this frame is less than 3 */
					if(tmp_payload->payload_type == I_FRAME && tmp_payload->tmp_ref < 3) {
						return(tmp_pes_holder);

					}
				}

				if(tmp_payload->tmp_ref == dup_tmp_ref) {
					duplicate++;
					if(duplicate == 2) {
						/* Didn't do a better job this time around*/
						return(missing_seq_pes_holder);
					}
				}


			}

			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header || duplicate == 2) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	return(missing_seq_pes_holder);


}
/**************************************************************************/

static pes_holder_t * fetch_next_avaliable_frame(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref, payload_type_t frame_type) {


	/* Markers */
	int seq_header;
	int next_seq_header;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;
	pes_holder_t * return_pes_holder;




	/* Init */
	seq_header = 0;
	next_seq_header = 0;
	return_pes_holder = NULL;

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

			if(tmp_payload->payload_type == frame_type && seq_header && tmp_payload->tmp_ref > tmp_ref) {
				return_pes_holder = tmp_pes_holder;
				next_seq_header = 1;
				break;
			}

			tmp_payload = tmp_payload->next;
		}

		if(next_seq_header) {
			break;
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}


	if(!return_pes_holder) {
		/* Okay we will need to take a frame
		 previous to the one missing */

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

				if(tmp_payload->tmp_ref > tmp_ref) {
					next_seq_header = 1;
					break;
				}

				tmp_payload = tmp_payload->next;
			}

			if(next_seq_header) {
				break;
			}

			tmp_pes_holder = tmp_pes_holder->next;
		}

		next_seq_header = 0;

		while(tmp_pes_holder) {

			tmp_payload = tmp_pes_holder->payload;
			while(tmp_payload) {

				if(tmp_payload->payload_type == frame_type) {
					return_pes_holder = tmp_pes_holder;
					next_seq_header = 1;
					break;
				}

				tmp_payload = tmp_payload->next;
			}

			if(next_seq_header) {
				break;
			}

			tmp_pes_holder = tmp_pes_holder->previous;
		}



	}

	return(return_pes_holder);

}

