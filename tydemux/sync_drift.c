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


int check_av_drift_in_seq(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


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

	/* Results from funcs */
	int result;

	/* Number of frame rate errors */
	int error;

	/* Number of frames */
	uint16_t highest_tmp_ref;

	/* Fields */
	int nr_of_fields;
	int reminder;

	/* Times */
	ticks_t present_time;
	ticks_t next_time;
	ticks_t diff;
	ticks_t time_to_meet;
	ticks_t calculated_time;
	ticks_t two_fields;
	ticks_t three_fields;

	ticks_t drift;




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


	/* Get the last frames tmp ref number */
	highest_tmp_ref = get_highest_tmp_ref(tmp_pes_holder);

	/* FIXME we don't messure drift if the next seq is a repair */

	/* Okay lest see if a two frame hasn't the repeate flag set and that the three frames has it set */

	present_time = get_time_of_tmp_ref(tmp_pes_holder, 0);

	if(!present_time) {
		LOG_DEVDIAG("Time error time is 0\n");
		return(-1);
	}

	for(counter = 0; counter < highest_tmp_ref + 1; counter++) {

		next_time = get_time_of_tmp_ref(tmp_pes_holder, counter + 1);

		if(!next_time) {
			LOG_DEVDIAG("Time error time is 0\n");
			/* Since this may hit a repair we simply return here and don't deal
			with drift in this seq */

			return(-1);
		}

		diff = next_time - present_time;

		result =  get_picture_info_tmp_ref(tystream, seq_pes_holder,  counter, &picture_info);

		if(!result) {
			LOG_DEVDIAG("Error fetching pic info\n");
			return(-1);
		}

		two_fields = tystream->frame_tick;
		three_fields = tystream->frame_tick + tystream->frame_tick/2;


		if(picture_info.repeat_first_field) {
			/* FIXME hard coded */
			if(diff > (three_fields - 10) && diff < (three_fields + 10)) {
				//printf("Spec: " I64FORMAT " \t Real " I64FORMAT " \n", three_fields, diff);
				result = 1;
			} else {
				error++;
				LOG_DEVDIAG2("Sync error is " I64FORMAT " should be " I64FORMAT "\n", diff, three_fields);
			}
		} else {
			/* FIXME hard coded */
			if(diff > (two_fields - 10) && diff < (two_fields + 10)) {
				//printf("Spec: " I64FORMAT " \t Real " I64FORMAT " \n", two_fields, diff);
				result = 1;
			} else {
				error++;
				LOG_DEVDIAG1("Sync error is " I64FORMAT " should be 3003\n", diff);
			}
		}

		present_time = next_time;

	}


	/* Get the time of the first frame in the next SEQ */
	time_to_meet = get_time_of_tmp_ref(tmp_pes_holder, highest_tmp_ref + 1);

	present_time = get_time_of_tmp_ref(tmp_pes_holder, 0);


	/* Count fields */
	nr_of_fields = 0;
	for(counter = 0; counter < highest_tmp_ref + 1; counter++) {


		result =  get_picture_info_tmp_ref(tystream, seq_pes_holder,  counter, &picture_info);

		if(!result) {
			LOG_DEVDIAG("Error fetching pic info\n");
			return(-1);
		}

		if(picture_info.repeat_first_field) {
			nr_of_fields = nr_of_fields + 3;
		} else {
			nr_of_fields = nr_of_fields + 2;
		}
	}


	reminder = nr_of_fields % 2;

	calculated_time = present_time + ((nr_of_fields / 2) * tystream->frame_tick) + (reminder * tystream->reminder_field);

	/* FIXME hard coded */
	if(tystream->frame_tick == 3003 && reminder) {
		if(tystream->reminder_field == 1502) {
			tystream->reminder_field = 1501;
		} else {
			tystream->reminder_field = 1502;
		}
	} else if (reminder) {
		tystream->reminder_field = tystream->frame_tick/2;
	}

	if(calculated_time >= time_to_meet) {
		drift = calculated_time - time_to_meet;
	} else {
		drift = -(time_to_meet - calculated_time);
	}

	tystream->drift = tystream->drift + drift;

	LOG_DEVDIAG1("Time field error was %i \n", error);

	if(error) {
		LOG_DEVDIAG3("SEQ " I64FORMAT " - drift " I64FORMAT " - total drift " I64FORMAT "\n", seq_pes_holder->seq_number, drift, tystream->drift);
	}

	return(error);
}


int fix_drift(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {

	/* Iteration */
	int i;
	int nr_added_removed;
	int nr_seeked;
	int added;
	int still;

	/* Frames */
	uint16_t highest_tmp_ref;
	int nr_of_frames;
	int freq_to_add_remove;
	int frames_to_add_remove;
	int frames_to_add_remove_2;


	/* Reminder */
	unsigned int reminder;


	if(tystream->drift <= 0) {
		/* Adding frames to fix drift, negative drift means
		that we are behind with number of frames */

		frames_to_add_remove = (int)( llabs(tystream->drift)/(int)tystream->frame_tick);
		reminder = (int)( llabs(tystream->drift)%(unsigned int)tystream->frame_tick );

		if(reminder >= tystream->drift_threshold) {
			frames_to_add_remove++;
		}
		frames_to_add_remove_2 = frames_to_add_remove;

		LOG_DEVDIAG1("Frames to add %i\n", frames_to_add_remove);

		highest_tmp_ref = get_highest_tmp_ref(seq_pes_holder);
		nr_of_frames =  highest_tmp_ref  + 1;

		freq_to_add_remove = nr_of_frames / (frames_to_add_remove + 1);

		//printf("Freq to add %i\n", freq_to_add_remove);

		still = 0;
		if(frames_to_add_remove > 4) {
			LOG_WARNING("WARNING: An extensive amount of video frames is missing\n");
			LOG_WARNING("WARNING: Compensating by showing a still picture - This may break DVD/SVCD compability\n");
			still = 1;
		}


		added = 0;

		if(!still) {

			/* This one is very broken FIXME FIXME */
			for(i=0, nr_added_removed=0, nr_seeked = 0; i < highest_tmp_ref + 1 && nr_added_removed < frames_to_add_remove; i++) {

					if(get_frame_type_of_tmp_ref(seq_pes_holder, i) == B_FRAME) {
						add_frame(tystream, seq_pes_holder, (uint16_t)i);
						fix_tmp_ref_adding_frame(seq_pes_holder, 1, (uint16_t)(i + 1) );

						nr_added_removed++;

						i = i + (freq_to_add_remove - nr_seeked);
						added++;
						LOG_DEVDIAG("Added one frame\n");
					} else {
						nr_seeked++;
					}
			}
		} else {

			for(i=0; i < highest_tmp_ref; i++) {
				if(get_frame_type_of_tmp_ref(seq_pes_holder, i) == B_FRAME) {
					while(frames_to_add_remove_2 > 0) {
						add_frame(tystream, seq_pes_holder, (uint16_t)i);
						fix_tmp_ref_adding_frame(seq_pes_holder, 1, (uint16_t)(i + 1) );
						frames_to_add_remove_2--;
					}
					break;
				}
			}
		}


		//printf("Added is %i\n", added);
		//printf("Drift is was " I64FORMAT "\n", tystream->drift);
		tystream->drift = tystream->drift + (frames_to_add_remove * tystream->frame_tick);
		//printf("Drift is now " I64FORMAT "\n", tystream->drift);
		return(1);


	} else {
		/* Removing frames to fix drift */

		frames_to_add_remove = (int)(llabs(tystream->drift)/(int)tystream->frame_tick);
		reminder = (int)(llabs(tystream->drift)%(unsigned int)tystream->frame_tick);

		if(reminder >= tystream->drift_threshold) {
			frames_to_add_remove++;
		}

		LOG_DEVDIAG1("Frames to remove %i\n", frames_to_add_remove);

		highest_tmp_ref = get_highest_tmp_ref(seq_pes_holder);
		nr_of_frames =  highest_tmp_ref  + 1;

		freq_to_add_remove = nr_of_frames / (frames_to_add_remove + 1);

		for(i=0, nr_added_removed=0, nr_seeked = 0; i < highest_tmp_ref + 1 && nr_added_removed < frames_to_add_remove; i++) {

				if(get_frame_type_of_tmp_ref(seq_pes_holder, i) == B_FRAME) {
					remove_frame(seq_pes_holder, (uint16_t)i);
					fix_tmp_ref_removing_frame(seq_pes_holder, 1, (uint16_t)i);
					nr_added_removed++;
					i = i + (freq_to_add_remove - nr_seeked);
					LOG_DEVDIAG("Removed one frame\n");
				} else {
					nr_seeked++;
				}
		}

		tystream->drift = tystream->drift - (frames_to_add_remove * tystream->frame_tick);
		return(1);
	}
}


