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
static int repair_tystream_real(tystream_holder_t * tystream);
static int nr_pes_holders_after_gap(const pes_holder_t * pes_holder);
static pes_holder_t * find_seq_holder(pes_holder_t * seq_pes_holder, data_times_t * audio_times);
static pes_holder_t * find_pi_frame_holder(tystream_holder_t * tystream, pes_holder_t * pi_frame_pes_holder, data_times_t * audio_time,
		data_times_t * video_times);
static int check_gaps(const pes_holder_t * seq_pes_holder);
static int get_audio_times(const tystream_holder_t * tystream, data_times_t * audio_times);
static int fix_audio_pes_holder_list(tystream_holder_t * tystream, pes_holder_t * stop_pes_holder,
		 pes_holder_t * start_pes_holder);
static int repair_audio(tystream_holder_t * tystream, data_times_t * audio_times, data_times_t * video_times);




/***************************************************************************************/

int repair_tystream(tystream_holder_t * tystream) {

	int result;


	if(!tystream->pes_holder_video  || !tystream->pes_holder_audio) {
		/* FIXME No pes holders exit */
		LOG_WARNING("repair_tystream: No pes_holders \n");
		return(0);
	}

	if(pes_holders_nr_of_okay(tystream->pes_holder_video, 100) &&
		pes_holders_nr_of_okay(tystream->pes_holder_audio, 100)) {
		LOG_USERDIAG("\nRepairing TyStream\n");
		result = repair_tystream_real(tystream);
		if(!result) {
			LOG_ERROR("Failed to repair tystream\n");
			LOG_ERROR("Sorry folks, we are exting now due to this failure \n");
			return(0);
		}
		LOG_USERDIAG("Managed to repair tystream\n");
		return(1);
	}

	return(0);

}


/***************************************************************************************/


static int repair_tystream_real(tystream_holder_t * tystream) {


	/* Pes holder */
	pes_holder_t * tmp_pes_holder;

	pes_holder_t * seq_pes_holder;

	pes_holder_t * pi_frame_pes_holder;

	pes_holder_t * b_frame_pes_holder;

	pes_holder_t * junk_pes_holder;
	
	pes_holder_t * next_seq_pes_holder;


	/* Audio/video time holder */
	data_times_t audio_times;
	data_times_t video_times;

	/* Marker */
	int gotit;

	/* Init */
	gotit = 0;

	if(!get_audio_times(tystream, &audio_times)) {
		return(0);
	}

	tmp_pes_holder = tystream->pes_holder_video;


	while(tmp_pes_holder) {
		if(tmp_pes_holder->gap) {
			gotit = 1;
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;

	}

	if(!gotit) {
		LOG_DEVDIAG("Did get the gap\n");
		return(0);
	}
	gotit = 0;

	seq_pes_holder = tmp_pes_holder;

	seq_pes_holder = find_seq_holder(seq_pes_holder, &audio_times);

	if(!seq_pes_holder) {
		LOG_DEVDIAG("Did get the seq\n");
		return(0);
	}

	pi_frame_pes_holder = tmp_pes_holder->previous;

	pi_frame_pes_holder = find_pi_frame_holder(tystream, pi_frame_pes_holder, &audio_times, &video_times);

	if(!pi_frame_pes_holder) {
		LOG_DEVDIAG("Did get the pi\n");
		return(0);
	}

	/* Okay so now we have the first P/I frame that we need
	to throw away pes from - we also know that the B-Frame
	before that P/I-Frame is okay

	Further more we have the PES holder holding the first
	repairable SEQ and GOP header after the gap

	We are ready to make the repair :) */

	/* We can either make the GOP broken linked or we can
	throw away the B-Frames in it that depends on the previous
	P/I Frame before the SEQ Header. We opt to make the
	GOP closed saves space and it's easier to calculate
	PTS's later on when we remux the stream */

	if(!(seq_pes_holder = gop_make_closed(tystream, seq_pes_holder, &video_times))) {
		/* so we error lets try to take the next gop */
		
		seq_pes_holder = find_seq_holder(seq_pes_holder, &audio_times);
		next_seq_pes_holder = seq_pes_holder;
		
		while(next_seq_pes_holder) {
			if(next_seq_pes_holder->seq_present &&  next_seq_pes_holder != seq_pes_holder) {
				break;
			}
			next_seq_pes_holder = next_seq_pes_holder->next;
		}

		if(next_seq_pes_holder) {
			seq_pes_holder = gop_make_closed(tystream, next_seq_pes_holder, &video_times);
			if(!seq_pes_holder) {
				return(0);
			}
		} else {
			return(0);
		}
	}

	seq_pes_holder->repaired = 1;

	b_frame_pes_holder = pi_frame_pes_holder->previous;

	junk_pes_holder = seq_pes_holder->previous;

	/* Okay so unlink the pes holder to throw away and link
	the seq/b-frame pes holers */

	junk_pes_holder->next = NULL;

	pi_frame_pes_holder->previous = NULL;

	b_frame_pes_holder->next = seq_pes_holder;

	seq_pes_holder->previous = b_frame_pes_holder;

	/* FIXME free chunks we junk */

	LOG_DEVDIAG2("Time: Stop is " I64FORMAT " - " I64FORMAT " \n", b_frame_pes_holder->previous->previous->time, video_times.data_stop);
	LOG_DEVDIAG2("Time: Start is " I64FORMAT " - " I64FORMAT "\n", seq_pes_holder->next->time, video_times.data_start);

	if(!repair_audio(tystream,  &audio_times, &video_times)) {
		return(0);
	}


	tystream->repair = check_gaps(seq_pes_holder);

	return(1);
}


/***************************************************************************************/

static int nr_pes_holders_after_gap(const pes_holder_t * pes_holder) {

	/* Counter */
	int i;

	/* Tmp pes holder */
	pes_holder_t * tmp_pes_holder;

	/* Init */
	tmp_pes_holder = (pes_holder_t *)pes_holder;
	i = 0;

	while(tmp_pes_holder) {
		if(tmp_pes_holder->gap) {
			return(i);
		} else {
			i++;
			tmp_pes_holder = tmp_pes_holder->next;
		}
	}
	return(i);
}

/************************************************************************************/


static pes_holder_t * find_seq_holder(pes_holder_t * seq_pes_holder, data_times_t * audio_times) {

	/* Marker */
	int gotit;

	/* pes holder check */
	int nr_pes;

	/* Tmp pes holder */
	pes_holder_t * tmp_pes_holder;

	gotit = 0;
	tmp_pes_holder = seq_pes_holder;

	while(seq_pes_holder) {
		if(seq_pes_holder->seq_present && seq_pes_holder->gop_present &&
			(seq_pes_holder->time >= audio_times->data_start)) {
			gotit = 1;
			break;
		} else if (seq_pes_holder->gap && tmp_pes_holder != seq_pes_holder) {
			LOG_DEVDIAG("Bummer gap\n");
			return(0);
		} else {
			seq_pes_holder = seq_pes_holder->next;
		}
	}


	if(!gotit) {
		LOG_DEVDIAG("Bummer\n");
		return(0);
	}

	nr_pes = nr_pes_holders_after_gap(seq_pes_holder);

	if(nr_pes > 5) {
		/* It's okay to do a repair otherwise continue
		 to find the next seq holder */
		LOG_DEVDIAG("Returning SEQ\n");
		return(seq_pes_holder);
	} else {
		return(find_seq_holder(seq_pes_holder->next, audio_times));
	}
}

/*********************************************************************************/

static pes_holder_t * find_pi_frame_holder(tystream_holder_t * tystream, pes_holder_t * pi_frame_pes_holder, data_times_t * audio_time,
		data_times_t * video_times) {

	/* Times */
//	ticks_t tick_diff;
	ticks_t time;
	payload_t * tmp_payload;
	int fields;
	
	fields = 0;

	while(pi_frame_pes_holder) {
		if((pi_frame_pes_holder->p_frame_present || pi_frame_pes_holder->i_frame_present) &&
			pi_frame_pes_holder->previous && pi_frame_pes_holder->previous->time) {

			if(pi_frame_pes_holder->previous && pi_frame_pes_holder->previous->previous &&
				pi_frame_pes_holder->previous->b_frame_present &&
				pi_frame_pes_holder->previous->previous->b_frame_present) {

				//tick_diff = pi_frame_pes_holder->previous->time - pi_frame_pes_holder->previous->previous->time;
				//time = pi_frame_pes_holder->previous->time + (2 * tick_diff);

				/* we must fine out if we have two or three fields in this frame */
				tmp_payload =  pi_frame_pes_holder->previous->payload;
				while(tmp_payload) {
					if(tmp_payload->payload_type == I_FRAME ||
						tmp_payload->payload_type == P_FRAME ||
						tmp_payload->payload_type == B_FRAME) {
							fields = get_repeat_first_field(tystream, tmp_payload);
							break;
					}
					tmp_payload = tmp_payload->next;
				}

				time = pi_frame_pes_holder->previous->time + tystream->frame_tick;
				if(fields) {
					time = time + tystream->frame_tick/2;
				}


				if(time <= audio_time->data_stop) {
					video_times->data_stop = time;
					return(pi_frame_pes_holder);
				}

			}
		}
		pi_frame_pes_holder = pi_frame_pes_holder->previous;
	}

	return(0);
}

/**********************************************************************************/

static int check_gaps(const pes_holder_t * seq_pes_holder) {


	pes_holder_t * tmp_pes_holder;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	while(tmp_pes_holder) {
		if(tmp_pes_holder->gap) {
			return(1);
		} else {
			tmp_pes_holder = tmp_pes_holder->next;
		}
	}
	return(0);
}

/******************************************************************************************/

static int get_audio_times(const tystream_holder_t * tystream, data_times_t * audio_times) {

	int gotit;
	pes_holder_t * tmp_pes_holder;

	gotit = 0;
	tmp_pes_holder = (pes_holder_t *)tystream->pes_holder_audio;


	while(tmp_pes_holder) {
		if(tmp_pes_holder->gap) {
			LOG_DEVDIAG1("Time Stop Audio: " I64FORMAT "\n", tmp_pes_holder->previous->time);
			LOG_DEVDIAG1("Time Start Audio: " I64FORMAT "\n", tmp_pes_holder->time);
			if(!tmp_pes_holder->previous) {
				return(0);
			}
			audio_times->data_stop = tmp_pes_holder->previous->time + tystream->audio_median_tick_diff;
			audio_times->data_start = tmp_pes_holder->time;
			gotit = 1;
			break;
		} else {
			tmp_pes_holder = tmp_pes_holder->next;
		}
	}
	if(gotit) {
		return(1);
	} else {
		return(0);
	}
}


/******************************************************************************************/

static int fix_audio_pes_holder_list(tystream_holder_t * tystream, pes_holder_t * stop_pes_holder, pes_holder_t * start_pes_holder) {

	pes_holder_t * junk_pes_holder;


	LOG_DEVDIAG1("Time Stop Audio 2: " I64FORMAT "\n", stop_pes_holder->time);
	LOG_DEVDIAG1("Time Start Audio 2: " I64FORMAT "\n", start_pes_holder->time);


	if(stop_pes_holder->next == start_pes_holder) {
		return(1);
	}

	junk_pes_holder = stop_pes_holder->next;
	junk_pes_holder->previous = NULL;

	/* Bind stop and start */
	stop_pes_holder->next = start_pes_holder;

	/* Turncate junk pes */
	start_pes_holder->previous->next = NULL;

	/* Bind start and stop */
	start_pes_holder->previous = stop_pes_holder;

	/* Mark the start pes holder as repair */
	start_pes_holder->repaired = 1;


	free_all_pes_holders(junk_pes_holder);

	return(1);

}

/***************************************************************************************************************/

static int repair_audio(tystream_holder_t * tystream, data_times_t * audio_times, data_times_t * video_times) {

	pes_holder_t * tmp_pes_holder;
	pes_holder_t * stop_pes_holder;
	pes_holder_t * start_pes_holder;
	int gotit;
	ticks_t start_drift, stop_drift, total_drift;

	ticks_t start_a_before_v, start_a_after_v, stop_a_before_v, stop_a_after_v;

	tmp_pes_holder = (pes_holder_t *)tystream->pes_holder_audio;


	while(tmp_pes_holder) {
		if(tmp_pes_holder->gap) {
			break;
		} else {
			tmp_pes_holder = tmp_pes_holder->next;
		}
	}

	if(!tmp_pes_holder->previous) {
		return(0);
	}

	stop_pes_holder = tmp_pes_holder->previous;
	start_pes_holder = tmp_pes_holder;

	gotit = 0;

	while(stop_pes_holder) {
		if(stop_pes_holder->time + tystream->audio_median_tick_diff <= video_times->data_stop) {
			gotit=1;
			LOG_DEVDIAG1("Audio stop time - " I64FORMAT "\n", stop_pes_holder->time + tystream->audio_median_tick_diff);
			LOG_DEVDIAG1("Video stop time - " I64FORMAT "\n", video_times->data_stop);
			break;
		} else {
			stop_pes_holder = stop_pes_holder->previous;
		}
	}

	if(!gotit) {
		return(0);
	}


	gotit = 0;
	while(start_pes_holder) {
		if(start_pes_holder->time >= video_times->data_start) {
			if(start_pes_holder != tmp_pes_holder && start_pes_holder->gap) {
				/* Bummer we hit another gap  FIXME*/
				return(0);
			}
			gotit=1;
			LOG_DEVDIAG1("Audio start time - " I64FORMAT "\n", start_pes_holder->time);
			break;
		} else {
			start_pes_holder = start_pes_holder->next;
		}
	}

	if(!gotit) {
		return(0);
	}

	/* Okay we have posistioned out start/stop pes holders
	in such way that they stop before video and start after
	video - hence we have less audio than video (well it can naturally match up perfectly
	but it's unlikely */

	if(start_pes_holder->gap) {
		start_a_before_v = 0;
	} else {
		start_a_before_v = video_times->data_start - start_pes_holder->previous->time;
	}

	start_a_after_v = start_pes_holder->time - video_times->data_start;

	if(stop_pes_holder->next->gap) {
		stop_a_after_v = 0;
	} else {
		stop_a_after_v = stop_pes_holder->next->time + tystream->audio_median_tick_diff - video_times->data_stop;
	}

	stop_a_before_v = video_times->data_stop - stop_pes_holder->time + tystream->audio_median_tick_diff;

	/* NOTE - Drift is mention revered to what is the case in audio cut !!! */


	if(start_pes_holder->gap && stop_pes_holder->next->gap) {
		/* must calc the drift */
		/* We are starting after video */
		start_drift = start_pes_holder->time  - video_times->data_start;
		stop_drift = video_times->data_stop - stop_pes_holder->time;
		total_drift = start_drift + stop_drift;
		pes_holder_attache_drift_i_frame(tystream->pes_holder_video, video_times->data_stop, total_drift);

		return(fix_audio_pes_holder_list(tystream, stop_pes_holder, start_pes_holder));

	} else if (start_pes_holder->gap) {
		if (llabs(start_a_after_v - stop_a_after_v) <= (start_a_after_v + stop_a_before_v)) {
			stop_pes_holder = stop_pes_holder->next;
		}
		start_drift = start_pes_holder->time  - video_times->data_start;
		stop_drift = video_times->data_stop - stop_pes_holder->time;
		total_drift = start_drift + stop_drift;
		pes_holder_attache_drift_i_frame(tystream->pes_holder_video, video_times->data_stop, total_drift);

		return(fix_audio_pes_holder_list(tystream, stop_pes_holder, start_pes_holder));

	} else if (stop_pes_holder->next->gap) {
		/* Hmm figure out which one is the best */
		if (llabs(stop_a_before_v - start_a_before_v) <= (stop_a_before_v + start_a_after_v)) {
			start_pes_holder = start_pes_holder->previous;
		}

		start_drift = start_pes_holder->time  - video_times->data_start;
		stop_drift = video_times->data_stop - stop_pes_holder->time;
		total_drift = start_drift + stop_drift;
		pes_holder_attache_drift_i_frame(tystream->pes_holder_video, video_times->data_stop, total_drift);

		return(fix_audio_pes_holder_list(tystream, stop_pes_holder, start_pes_holder));

	}

	/* Okay so above we choosed the one with as little offset as possible */

	if(llabs(stop_a_before_v - start_a_before_v) <=  llabs(stop_a_after_v - start_a_after_v)) {
		if(llabs(stop_a_before_v - start_a_before_v) <= (stop_a_before_v + start_a_after_v)) {
			start_pes_holder = start_pes_holder->previous;
		}

	} else {
		if(llabs(stop_a_after_v - start_a_after_v) <= (stop_a_before_v + start_a_after_v)) {
			stop_pes_holder = stop_pes_holder->next;
		}
	}

	start_drift = start_pes_holder->time  - video_times->data_start;
	stop_drift = video_times->data_stop - stop_pes_holder->time;
	total_drift = start_drift + stop_drift;
	pes_holder_attache_drift_i_frame(tystream->pes_holder_video, video_times->data_start, total_drift);

	return(fix_audio_pes_holder_list(tystream, stop_pes_holder, start_pes_holder));

}


