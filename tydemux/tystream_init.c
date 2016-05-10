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
static pes_holder_t *  init_find_seq_holder(pes_holder_t * seq_pes_holder);
static pes_holder_t * init_find_audio(tystream_holder_t * tystream, pes_holder_t * audio_pes_holder, data_times_t * video_times);
static pes_holder_t * init_find_audio_stream(tystream_holder_t * tystream, pes_holder_t * audio_pes_holder, data_times_t * video_times);
static int tystream_real_init(tystream_holder_t * tystream);
static int tystream_stream_real_init(tystream_holder_t * tystream);
/* FIXME we need a counter how may times we shoud fail before aborting */

int tystream_init(tystream_holder_t * tystream) {

	if(tystream->find_seq) {
		/*Hmm the stream is allready inited */
		return(1);
	}

	if(!tystream->pes_holder_video) {
		/* No pes holders at all yet */
		return(0);
	}

	/* FIXME need to think about cut here!! */
	if(tystream->tivo_series == S2) {
		if(pes_holders_nr_of_okay(tystream->pes_holder_video, 70)) {
			return(tystream_real_init(tystream));
		}
	} else {
		/* We have S1 */
		if(pes_holders_nr_of_okay(tystream->pes_holder_video, 400)) {
			return(tystream_real_init(tystream));
		}
	}
	return(0);
}



int tystream_stream_init(tystream_holder_t * tystream) {


	if(tystream->find_seq) {
		/*Hmm the stream is allready inited */
		return(1);
	}

	if(!tystream->pes_holder_video && !tystream->pes_holder_audio) {
		/* No pes holders at all yet */
		return(0);
	}

	/* FIXME need to think about cut here!! */
	if(tystream->tivo_series == S2) {
		if(pes_holders_nr_of_okay(tystream->pes_holder_video, 70)) {
			return(tystream_stream_real_init(tystream));
		}
	} else {
		/* We have S1 */
		if(pes_holders_nr_of_okay(tystream->pes_holder_video, 400)) {
			return(tystream_stream_real_init(tystream));
		}
	}
	return(0);
}




static int tystream_real_init(tystream_holder_t * tystream) {


	data_times_t video_times;

	pes_holder_t * seq_pes_holder;
	pes_holder_t * next_seq_pes_holder;
	pes_holder_t * audio_pes_holder;
	pes_holder_t * junk_pes_holder;
	pes_holder_t * tmp_pes_holder;

	seq_pes_holder = tystream->pes_holder_video;
	audio_pes_holder = tystream->pes_holder_audio;


	if(!(seq_pes_holder = init_find_seq_holder(seq_pes_holder))) {
		return(0);
	}


	if(!(seq_pes_holder = gop_make_closed(tystream, seq_pes_holder, &video_times))) {

		seq_pes_holder = init_find_seq_holder(tystream->pes_holder_video);
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

	LOG_DEVDIAG2("GOP %i SEQ %i\n", seq_pes_holder->gop_present, seq_pes_holder->seq_present);
	if(!(audio_pes_holder = init_find_audio(tystream, audio_pes_holder, &video_times))) {
		return(0);
	}

	if(audio_pes_holder != tystream->pes_holder_audio) {
		junk_pes_holder = tystream->pes_holder_audio;

		audio_pes_holder->previous->next = NULL;
		audio_pes_holder->previous = NULL;

		tystream->pes_holder_audio = audio_pes_holder;

		free_all_pes_holders(junk_pes_holder);
	}

	if(seq_pes_holder != tystream->pes_holder_video) {
		junk_pes_holder = tystream->pes_holder_video;

		seq_pes_holder->previous->next = NULL;
		seq_pes_holder->previous = NULL;

		tystream->pes_holder_video = seq_pes_holder;

		free_all_pes_holders(junk_pes_holder);
	}

	LOG_DEVDIAG2("Audio diff is " I64FORMAT " - indicator is %i\n", tystream->time_diff, tystream->indicator);

	/* Fixing SEQ header if we have a S1 V_2X */
	if(tystream->tivo_series == S1 && tystream->tivo_version == V_2X) {
		tmp_pes_holder = move_seq_to_i_frame(tystream->pes_holder_video);
		if(tmp_pes_holder) {
			tmp_pes_holder->seq_fixed = 1;
			tystream->lf_seq_pes_holder =tmp_pes_holder;
			tystream->pes_holder_video = tmp_pes_holder;
		}
	}


	tystream->find_seq = 1;

	return(1);

}


static int tystream_stream_real_init(tystream_holder_t * tystream) {


	data_times_t video_times;

	pes_holder_t * seq_pes_holder;
	pes_holder_t * next_seq_pes_holder;
	pes_holder_t * audio_pes_holder;
	pes_holder_t * junk_pes_holder;
	pes_holder_t * tmp_pes_holder;

	seq_pes_holder = tystream->pes_holder_video;
	audio_pes_holder = tystream->pes_holder_audio;


	if(!(seq_pes_holder = init_find_seq_holder(seq_pes_holder))) {
		return(0);
	}


	if(!(seq_pes_holder = gop_make_closed(tystream, seq_pes_holder, &video_times))) {

		seq_pes_holder = init_find_seq_holder(tystream->pes_holder_video);
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

	LOG_DEVDIAG2("GOP %i SEQ %i\n", seq_pes_holder->gop_present, seq_pes_holder->seq_present);
	if(!(audio_pes_holder = init_find_audio_stream(tystream, audio_pes_holder, &video_times))) {
		return(0);
	}

	if(audio_pes_holder != tystream->pes_holder_audio) {
		junk_pes_holder = tystream->pes_holder_audio;

		audio_pes_holder->previous->next = NULL;
		audio_pes_holder->previous = NULL;

		tystream->pes_holder_audio = audio_pes_holder;

		free_all_pes_holders(junk_pes_holder);
	}

	if(seq_pes_holder != tystream->pes_holder_video) {
		junk_pes_holder = tystream->pes_holder_video;

		seq_pes_holder->previous->next = NULL;
		seq_pes_holder->previous = NULL;

		tystream->pes_holder_video = seq_pes_holder;

		free_all_pes_holders(junk_pes_holder);
	}

	LOG_DEVDIAG2("Audio diff is " I64FORMAT " - indicator is %i\n", tystream->time_diff, tystream->indicator);

	/* Fixing SEQ header if we have a S1 V_2X */
	if(tystream->tivo_series == S1 && tystream->tivo_version == V_2X) {
		tmp_pes_holder = move_seq_to_i_frame(tystream->pes_holder_video);
		if(tmp_pes_holder) {
			tmp_pes_holder->seq_fixed = 1;
			tystream->lf_seq_pes_holder =tmp_pes_holder;
			tystream->pes_holder_video = tmp_pes_holder;
		}
	}


	tystream->find_seq = 1;

	return(1);

}


/**************************************************************************/
/* FIXME DUP CODE */
/* This one is really a dup from tystream_repair.c FIXME */

static pes_holder_t *  init_find_seq_holder(pes_holder_t * seq_pes_holder) {


	while(seq_pes_holder) {
		if(seq_pes_holder->seq_present && seq_pes_holder->gop_present) {
			if((seq_pes_holder->next && seq_pes_holder->next->i_frame_present) || seq_pes_holder->i_frame_present) {
				LOG_DEVDIAG("Returning SEQ\n");
				LOG_DEVDIAG2("GOP %i SEQ %i\n", seq_pes_holder->gop_present, seq_pes_holder->seq_present);
				return(seq_pes_holder);
			}
		} else if(seq_pes_holder->gap) {
			LOG_WARNING("GAP NOT Returning SEQ\n");
			return(0);
		}
		seq_pes_holder = seq_pes_holder->next;
	}
	LOG_WARNING("GAP NOT Returning SEQ\n");
	return(0);

}

/**************************************************************************/
static pes_holder_t * init_find_audio_stream(tystream_holder_t * tystream, pes_holder_t * audio_pes_holder, data_times_t * video_times) {

	/* Audio should start 960 - 330 ms before video (when it comes to a closed gop */
	/* This is 56700 ticks or roughtly 18 video frames at 29.97 f/s */

	while(audio_pes_holder) {
		if(audio_pes_holder->time >= video_times->data_start - (ticks_t)56700) {
			return(audio_pes_holder);
		}
			audio_pes_holder = audio_pes_holder->next;
	}
	/* Okay we didn't find any good once lets at least take the first one */
	return(tystream->pes_holder_audio);
}

/**************************************************************************/

static pes_holder_t * init_find_audio(tystream_holder_t * tystream, pes_holder_t * audio_pes_holder, data_times_t * video_times) {

	while(audio_pes_holder) {
		if(audio_pes_holder->time >= video_times->data_start) {
			if(audio_pes_holder->previous) {
				if(video_times->data_start -  audio_pes_holder->previous->time <=
					audio_pes_holder->time - video_times->data_start) {

					tystream->indicator = POSITIVE;
					tystream->time_diff = video_times->data_start -  audio_pes_holder->previous->time;
					return(audio_pes_holder->previous);
				}
			}
			//printf("AUDIO " I64FORMAT " - VIDEO " I64FORMAT "\n", audio_pes_holder->time, video_times->data_start);
			tystream->indicator = NEGATIVE;
			tystream->time_diff = video_times->data_start - audio_pes_holder->time;
			return(audio_pes_holder);

		} else if(audio_pes_holder->gap) {
			return(0);
		} else {
			audio_pes_holder = audio_pes_holder->next;
		}
	}
	return(0);
}
