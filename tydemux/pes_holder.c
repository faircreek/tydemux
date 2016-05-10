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

int video_pes_count = 0;
int audio_pes_count = 0;

int64_t last_audio_pes_time = 0;

int64_t last_video_pes_time = 0;
payload_type_t last_video_pes_type = I_FRAME;

int off_balance=0;



/* Internal */
static pes_holder_t * write_pes_holder_real_audio(tystream_holder_t * tystream, pes_holder_t * pes_holder, int * audio_progress_meter);
static pes_holder_t * write_pes_holder_real_video(tystream_holder_t * tystream, pes_holder_t * pes_holder, int * video_progress_meter);
static int add_pes_holder_audio(tystream_holder_t * tystream, pes_holder_t * pes_holder);
static int add_pes_holder_video(tystream_holder_t * tystream, pes_holder_t * pes_holder);
static payload_t * fetch_next_correct_audio_payload(tystream_holder_t * tystream, const pes_holder_t * pes_holder);
static int write_video_size(tystream_holder_t * tystream, pes_holder_t * pes_holder);


int get_pes_holders(tystream_holder_t * tystream) {

	if(!tystream->chunks) {
		return(0);
	}

	while(chunk_okay_to_parse(tystream->chunks)) {
		tystream->chunks = parse_chunk(tystream, tystream->chunks);
	}

	return(1);

}


int pes_holders_nr_of_okay(const pes_holder_t * pes_holder, int treshhold) {

	int counter;
	pes_holder_t * tmp_pes_holder;

	/* Init */
	tmp_pes_holder = (pes_holder_t *)pes_holder;
	counter = 0;

	while(tmp_pes_holder) {
		counter++;
		if(counter > treshhold) {
			return(1);
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}

	return(0);
}




payload_t * new_payload() {

	payload_t * payload;

	payload =(payload_t *)malloc(sizeof(payload_t));
	memset(payload, 0,sizeof(payload_t));

	payload->payload_type = UNKNOWN_PAYLOAD;
	payload->fields = UNKNOWN_FIELD;

	return(payload);
}

/****************************************************************************/

void free_payload(payload_t * payload) {

	if(payload->payload_data != NULL) {
		free(payload->payload_data);
	}

	if (payload->previous && payload->previous->next == payload) {
		payload->previous->next = NULL;
	}

	if (payload->next && payload->next->previous == payload) {
		payload->next->previous = NULL;
	}

	free(payload);
}
/****************************************************************************/

void free_all_payload(payload_t * payload) {

	payload_t * old_payload;

	while (payload->previous) {
		payload = payload->previous;
	}

	while(payload) {
		old_payload = payload;
		payload = payload->next;
		free_payload(old_payload);
	}
}

/****************************************************************************/

void free_pes_holder(pes_holder_t * pes_holder, int free_payload) {

	if(free_payload) {
		if(pes_holder->payload != NULL) {
			free_all_payload(pes_holder->payload);
		}
	}

	if (pes_holder->previous && pes_holder->previous->next == pes_holder) {
		 pes_holder->previous->next = NULL;
	}

	if (pes_holder->next && pes_holder->next->previous == pes_holder) {
		pes_holder->next->previous = NULL;
	}

	free(pes_holder);
}

/****************************************************************************/

void free_all_pes_holders(pes_holder_t * pes_holder) {

	pes_holder_t * old_pes_holder;

	while(pes_holder) {
		old_pes_holder = pes_holder;
		pes_holder = pes_holder->next;
		free_pes_holder(old_pes_holder, 1);
	}
}

/****************************************************************************/

pes_holder_t * new_pes_holder(media_type_t media) {


	pes_holder_t * pes_holder;

	pes_holder = (pes_holder_t *)malloc(sizeof(pes_holder_t));
	memset(pes_holder, 0,sizeof(pes_holder_t));

	pes_holder->media = media;

	return(pes_holder);
}
/****************************************************************************/


int write_pes_holders(tystream_holder_t * tystream, int * video_progress_meter, int * audio_progress_meter) {

	int j;

	int to_write_video;
	int to_write;
	int first;
	long int audio_space;
	long int video_space;
	int nothing_written = 0;

	int64_t new_last_time_pes_video;
	int64_t DTS;

	payload_type_t new_last_video_pes_type;

	int audio_buffer;

	if(!tystream->pes_holder_video || !tystream->pes_holder_audio) {
		return(0);
	}

	if(tystream->repair) {
		return(0);
	}

	if(!tystream->find_seq) {
		return(0);
	}




	if(tystream->std_alone) {
		while(pes_holders_nr_of_okay(tystream->pes_holder_audio, 10)) {
			tystream->pes_holder_audio = write_pes_holder(tystream, tystream->pes_holder_audio, video_progress_meter, audio_progress_meter);
		}


		to_write = is_okay(tystream->pes_holder_video, tystream->lf_gop_frame_counter_pes_holder_video);

		for(j = 0; j < to_write - 1; j++) {
			first = 0;
			while(tystream->pes_holder_video) {
				if(tystream->pes_holder_video->seq_present) {
					if(first) {
						break;
					} else {
						first = 1;
					}
				}
				tystream->pes_holder_video = write_pes_holder(tystream, tystream->pes_holder_video, video_progress_meter, audio_progress_meter);
			}
		}

	}

#if !defined(TIVO)
	else {
		/* We are writing to pipes and we need to be a bit more careful how we do that */

		/* We will need to find out how many video frames that is waiting to be written to the pipe */
		to_write_video = is_okay_frames(tystream->pes_holder_video, tystream->lf_gop_frame_counter_pes_holder_video);

		/* Leave at least 100 frames as a buffer since we often need to fecth e.g. seq/gops for repair
		if we cut this too low we will fail doing that which is bad - now this affects audio too -
		since we can fill up the audio buffer and lock it - dead lock! - hence we need to caluclate the
		amount of audio we want in the audio buffer */

		if(tystream->tivo_type == DTIVO) {
			audio_buffer = (int)(((int64_t)50 * tystream->frame_tick)/tystream->audio_median_tick_diff);
		} else {
			audio_buffer = (int)(((int64_t)60 * tystream->frame_tick)/tystream->audio_median_tick_diff);
		}

		if(to_write_video <= 60) {
			to_write_video = 0;
		} else {
			to_write_video = to_write_video - 60;
		}
#if 0
		if(!tystream->std_alone) {
			printf("Avalible in audio %ld\n", thread_pipe_freespace(tystream->audio_pipe));
			printf("Avalible in video %ld\n", thread_pipe_freespace(tystream->video_pipe));
		}
#endif
		//printf("Into write\n");
		for(j = 0 ; j < to_write_video && pes_holders_nr_of_okay(tystream->pes_holder_audio, audio_buffer);)  {
			
			if( nothing_written++ > 30 ) {
				// Nothing written for 30 seconds - mplex or transcode stalled
				LOG_FATAL( "Demux stalled, nothing written for 30 seconds. transcode or mplex may have crashed\n" );
			}

				/* The strategy is to write audio a head of video hence we will not write
				any video frames unless they are 1000 (11ms) ticks behind the audio (DTS wise )- this will ensure
				that mplex gets enough audio to multiplex the video - NOTE that audio is in case of
				mplex (And mpeg in general) always ahead of video in the stream */

				if(tystream->pes_holder_video->time == 0) {

					/* okay we have a fake pes - either a repair or a pframe in a SA/UK tivo */
					/* lest fake it's PTS */

					if(tystream->pes_holder_video->i_frame_present || tystream->pes_holder_video->p_frame_present) {

						if(last_video_pes_type == I_FRAME) {

							/* We have a p/i after p/i just add the frame tick (Well
							if have a closed gop this is not entierly true but what the heck :) */

							new_last_time_pes_video = last_video_pes_time + tystream->frame_tick;


						} else {
							/* We have a B-Frame - B-Frames are always decoded before a I frame
							 hence a I frame will have a substancially higher PTS infact the normal
							 is 4 * tystream->frame_tick */

							new_last_time_pes_video = last_video_pes_time + (tystream->frame_tick * (int64_t)4);
						}


					} else {

						if(last_video_pes_type == I_FRAME) {

							/* Okay since I/P-Frames has a higher PTS than a B frame normally
							2 to 3 times tystream->frame_tick */

							new_last_time_pes_video = last_video_pes_time + (tystream->frame_tick * (int64_t)2);

						} else {

							/* we have a bframe after a b frame just add the frame tick */

							new_last_time_pes_video = last_video_pes_time + tystream->frame_tick;
						}
					}

				} else {

					new_last_time_pes_video = tystream->pes_holder_video->time;

				}

				if(tystream->pes_holder_video->i_frame_present || tystream->pes_holder_video->p_frame_present) {


					new_last_video_pes_type = I_FRAME;

				} else {

					new_last_video_pes_type = B_FRAME;
				}


				/* now lest see if we should write this video frame - to do so we need to figure out the DTS */
				if (new_last_video_pes_type == I_FRAME && last_video_pes_type == B_FRAME) {

					DTS = new_last_time_pes_video  - (tystream->frame_tick * (int64_t)3);

				} else {

					DTS = new_last_time_pes_video;
				}

#if DEBUGTHREAD
				printf("DTS " I64FORMAT " - last_audio_pes_time " I64FORMAT " \n", DTS, last_audio_pes_time);
#endif
				audio_space = thread_pipe_freespace(tystream->audio_pipe);
				/* Okay lest see if our audio time is smaller than the DTS of the video to write if so write */
				if(tystream->pes_holder_audio->time) {

					//printf("The audi had a time\n");
					while(pes_holders_nr_of_okay(tystream->pes_holder_audio, audio_buffer) &&
						(tystream->pes_holder_audio->time < DTS + (int64_t)3000 || last_audio_pes_time == 0) &&
						(audio_space > 1552000 || audio_space == -1)) {

						tystream->pes_holder_audio = write_pes_holder(tystream, tystream->pes_holder_audio, video_progress_meter, audio_progress_meter);
						nothing_written = 0;
						last_audio_pes_time = tystream->pes_holder_audio->time;
						audio_space = thread_pipe_freespace(tystream->audio_pipe);
#if DEBUGTHREAD
						printf("Wrote audio\n");
#endif
					}


#if 0
					if(tystream->pes_holder_audio->time < DTS + (int64_t)1000 || last_audio_pes_time == 0) {
						tystream->pes_holder_audio = write_pes_holder(tystream, tystream->pes_holder_audio, video_progress_meter, audio_progress_meter);
#if DEBUGTHREAD
						printf("Wrote audio\n");
#endif
						nothing_written = 0;
						last_audio_pes_time = tystream->pes_holder_audio->time;
					}
#endif
				} else if (last_audio_pes_time + tystream->audio_median_tick_diff < DTS + (int64_t)3000 &&
						(audio_space > 1552000 || audio_space == -1)) {
						tystream->pes_holder_audio = write_pes_holder(tystream, tystream->pes_holder_audio, video_progress_meter, audio_progress_meter);

#if DEBUGTHREAD
						printf("Wrote audio\n");
#endif
						nothing_written = 0;
						last_audio_pes_time = last_audio_pes_time + tystream->audio_median_tick_diff;
				}

/*
				if(tystream->pes_holder_audio->time) {
					last_audio_pes_time = tystream->pes_holder_audio->time;
				} else {
					last_audio_pes_time = last_audio_pes_time + tystream->audio_median_tick_diff;
				}
*/
				video_space = thread_pipe_freespace(tystream->video_pipe);

				/* Down to basics now - the DTS needs to be 1000 ticks more than the last written audio frame*/
				if((DTS > last_audio_pes_time + (int64_t)1000 && DTS < last_audio_pes_time + (int64_t)15000) &&
					(video_space > 2097152 || video_space == -1)) { //was 15000
				//if(DTS > last_audio_pes_time + (int64_t)1000 && DTS < last_audio_pes_time + (int64_t)5000) {
					/* We are okay to write */
					tystream->pes_holder_video = write_pes_holder(tystream, tystream->pes_holder_video, video_progress_meter, audio_progress_meter);
					j++;
					last_video_pes_type = new_last_video_pes_type;
					last_video_pes_time = new_last_time_pes_video;
#if DEBUGTHREAD
					printf("Wrote video\n");
#endif
					nothing_written = 0;
				} /* else we wait - NO we can have a error in the time lest check for that otherwise we might get stuck*/

//				else if( (last_video_pes_time >  new_last_time_pes_video + ((int64_t)30 *tystream->frame_tick)) ||

				else if (last_audio_pes_time > DTS && (video_space > 2097152 || video_space == -1)) {
					/* We need to write this one anyway */
					tystream->pes_holder_video = write_pes_holder(tystream, tystream->pes_holder_video, video_progress_meter, audio_progress_meter);
					//printf("New video time is " I64FORMAT " \n",tystream->pes_holder_video->time);
					j++;
					//last_video_pes_type = new_last_video_pes_type;
					//last_video_pes_time = new_last_time_pes_video;
#if DEBUGTHREAD
					printf("Wrote emergency\n");
#endif
					nothing_written = 0;
				}

				if( nothing_written ) {
					/* Pipes must be backlogged, sleep to allow transcode and mplex to catch up */
					sleep(1);
				}
		}

	}
#endif

#if DEBUGTHREAD
	printf("Done writing\n");
#endif

#if 0
	if(!tystream->std_alone) {
		pipe_stat_t  write;
		pipe_stat_t  read;
		thread_pipe_stats(tystream->audio_pipe,&read,&write);
		printf("%llu bytes written to audio pipe\n", write);
		thread_pipe_stats(tystream->video_pipe,&read,&write);
		printf("%llu bytes written to video pipe\n", write);
	}
#endif
	return(1);
}


#if 0

int write_pes_holders(tystream_holder_t * tystream, int * video_progress_meter, int * audio_progress_meter) {

	int j;

	int to_write_video;
	int to_write;
	int first;
	long int audio_space;
	long int video_space;

//	int64_t DTS;

	int audio_buffer;

	if(!tystream->pes_holder_video || !tystream->pes_holder_audio) {
		return(0);
	}

	if(tystream->repair) {
		return(0);
	}

	if(!tystream->find_seq) {
		return(0);
	}



	if(tystream->std_alone) {
		while(pes_holders_nr_of_okay(tystream->pes_holder_audio, 10)) {
			tystream->pes_holder_audio = write_pes_holder(tystream, tystream->pes_holder_audio, video_progress_meter, audio_progress_meter);
		}


		to_write = is_okay(tystream->pes_holder_video, tystream->lf_gop_frame_counter_pes_holder_video);

		for(j = 0; j < to_write - 1; j++) {
			first = 0;
			while(tystream->pes_holder_video) {
				if(tystream->pes_holder_video->seq_present) {
					if(first) {
						break;
					} else {
						first = 1;
					}
				}
				tystream->pes_holder_video = write_pes_holder(tystream, tystream->pes_holder_video, video_progress_meter, audio_progress_meter);
			}
		}

	}

#if !defined(TIVO)

	else {
		/* We are writing to pipes and we need to be a bit more careful how we do that */

		/* We will need to find out how many video frames that is waiting to be written to the pipe */
		to_write_video = is_okay_frames(tystream->pes_holder_video, tystream->lf_gop_frame_counter_pes_holder_video);

		/* Leave at least 100 frames as a buffer since we often need to fecth e.g. seq/gops for repair
		if we cut this too low we will fail doing that which is bad - now this affects audio too -
		since we can fill up the audio buffer and lock it - dead lock! - hence we need to caluclate the
		amount of audio we want in the audio buffer */

		if(tystream->tivo_type == DTIVO) {
			audio_buffer = (int)(((int64_t)50 * tystream->frame_tick)/tystream->audio_median_tick_diff);
		} else {
			audio_buffer = (int)(((int64_t)60 * tystream->frame_tick)/tystream->audio_median_tick_diff);
		}

		if(to_write_video <= 60) {
			to_write_video = 0;
		} else {
			to_write_video = to_write_video - 60;
		}
#if 0
		if(!tystream->std_alone) {
			printf("Avalible in audio %ld\n", thread_pipe_freespace(tystream->audio_pipe));
			printf("Avalible in video %ld\n", thread_pipe_freespace(tystream->video_pipe));
		}
#endif

		audio_space = thread_pipe_freespace(tystream->audio_pipe);
		video_space = thread_pipe_freespace(tystream->video_pipe);
		while((audio_space > 1552000 || audio_space == -1) &&  pes_holders_nr_of_okay(tystream->pes_holder_audio, audio_buffer)) {
			tystream->pes_holder_audio = write_pes_holder(tystream, tystream->pes_holder_audio, video_progress_meter, audio_progress_meter);
			audio_space = thread_pipe_freespace(tystream->audio_pipe);
		}

		j = 0;
		while((video_space >  2097152 || video_space == -1) &&  j < to_write_video) {
			tystream->pes_holder_video = write_pes_holder(tystream, tystream->pes_holder_video, video_progress_meter, audio_progress_meter);
			j++;
			video_space = thread_pipe_freespace(tystream->video_pipe);
		}
	}
#endif

	return(1);
}

#endif 0

/****************************************************************************/

pes_holder_t * write_pes_holder(tystream_holder_t * tystream, pes_holder_t * pes_holder, int * video_progress_meter, int * audio_progress_meter) {

	int video;

	video = pes_holder->seq_present + pes_holder->gop_present + pes_holder->i_frame_present
		+ pes_holder->p_frame_present + pes_holder->b_frame_present;

	if(pes_holder->gap) {
		LOG_DEVDIAG("GAP\n");
	}

	if(tystream->mode != DEMUX) {
		return(0);
	}

	if(pes_holder->media == AUDIO) {
		return(write_pes_holder_real_audio(tystream, pes_holder, audio_progress_meter));

	} else if (pes_holder->media == VIDEO ) {
		return(write_pes_holder_real_video(tystream, pes_holder, video_progress_meter));

	} else {
		LOG_DEVDIAG("write_pes_holder: Error Not Video/Audio pes holder!\n");
		return(pes_holder->next);
	}
}

/****************************************************************************/

int nr_throw_away_pes_holder(const pes_holder_t * pes_holder) {

	/* Counter */
	int i;

	/* Tmp pes holder */
	pes_holder_t * tmp_pes_holder;

	/* Init */
	tmp_pes_holder = (pes_holder_t *)pes_holder;

	for (i=0; tmp_pes_holder; tmp_pes_holder=tmp_pes_holder->next, i++);

	return(i);
}
/****************************************************************************/

int add_pes_holder(tystream_holder_t * tystream, pes_holder_t * pes_holder) {

	if(pes_holder->media == AUDIO) {
		return(add_pes_holder_audio(tystream, pes_holder));
	} else if (pes_holder->media == VIDEO) {
		return(add_pes_holder_video(tystream, pes_holder));
	} else {
		LOG_WARNING("write_pes_holder: Error Not a Video/Audio pes holder!\n");
		return(0);
	}
}

/****************************************************************************/

void add_payload(pes_holder_t * pes_holder, payload_t * payload) {

	/* Save payload */
	payload_t * tmp_payload;


	if(pes_holder->payload == NULL) {
		pes_holder->payload = payload;
	} else {
		tmp_payload = pes_holder->payload;
		while(tmp_payload->next) {
			tmp_payload = tmp_payload->next;
		}
		tmp_payload->next = payload;
		tmp_payload->next->previous = tmp_payload;
	}
}
/****************************************************************************/

pes_holder_t * duplicate_pes_holder(const pes_holder_t * pes_holder) {

	/* FIXME This function will not publicate  gap indicator
	or extended data - just pure video - plus pes header */

	/* The new pes_header */
	pes_holder_t * pes_holder_new;

	/* Payload */
	payload_t * tmp_payload;
	payload_t * payload_new;

	pes_holder_new = new_pes_holder(pes_holder->media);

	pes_holder_new->time = pes_holder->time;

	tmp_payload = pes_holder->payload;

	while(tmp_payload) {


		if(tmp_payload->payload_type == I_FRAME ||
			tmp_payload->payload_type == P_FRAME ||
			tmp_payload->payload_type == B_FRAME ||
			tmp_payload->payload_type == PES_VIDEO ||
			tmp_payload->payload_type == SEQ_HEADER ||
			tmp_payload->payload_type == GOP_HEADER ) {

			/* Duplicate this payload */
			payload_new = duplicate_payload(tmp_payload);
			add_payload(pes_holder_new, payload_new);
			if(tmp_payload->payload_type == I_FRAME) {
				pes_holder_new->i_frame_present++;
				pes_holder_new->size = pes_holder_new->size + payload_new->size;
				pes_holder_new->total_size = pes_holder_new->total_size + payload_new->size;
			} else if (tmp_payload->payload_type == P_FRAME) {
				pes_holder_new->p_frame_present++;
				pes_holder_new->size = pes_holder_new->size + payload_new->size;
				pes_holder_new->total_size = pes_holder_new->total_size + payload_new->size;
			} else if (tmp_payload->payload_type == B_FRAME) {
				pes_holder_new->b_frame_present++;
				pes_holder_new->size = pes_holder_new->size + payload_new->size;
				pes_holder_new->total_size = pes_holder_new->total_size + payload_new->size;
			} else if (tmp_payload->payload_type == SEQ_HEADER) {
				pes_holder_new->seq_present++;
				pes_holder_new->size = pes_holder_new->size + payload_new->size;
				pes_holder_new->total_size = pes_holder_new->total_size + payload_new->size;
			} else if (tmp_payload->payload_type == GOP_HEADER) {
				pes_holder_new->gop_present++;
				pes_holder_new->size = pes_holder_new->size + payload_new->size;
				pes_holder_new->total_size = pes_holder_new->total_size + payload_new->size;
			} else {
				pes_holder_new->total_size = pes_holder_new->total_size + payload_new->size;
			}
		}
		tmp_payload = tmp_payload->next;
	}

	return(pes_holder_new);
}
/****************************************************************************/

payload_t * duplicate_payload(const payload_t * payload) {


	payload_t * payload_new;

	payload_new = new_payload();

	payload_new->size 		= payload->size;
	payload_new->tmp_ref 		= payload->tmp_ref;
	payload_new->tmp_ref_added 	= payload->tmp_ref_added;
	payload_new->payload_type 	= payload->payload_type;
	payload_new->fields 		= payload->fields;

	payload_new->payload_data	= (uint8_t *)malloc(sizeof(uint8_t) * payload->size);
	memset(payload_new->payload_data, 0,sizeof(uint8_t) * payload->size);

	memcpy(payload_new->payload_data, payload->payload_data, payload->size);

	return(payload_new);
}
/****************************************************************************/

/* This function reinit most things exept
  time, media type, gap and chunk_nr */

void reinit_pes_holder(pes_holder_t * pes_holder) {

	payload_t * tmp_payload;

	pes_holder->total_size = 0;
	pes_holder->size       = 0;

	pes_holder->seq_present     = 0;
	pes_holder->gop_present     = 0;
	pes_holder->i_frame_present = 0;
	pes_holder->p_frame_present = 0;
	pes_holder->b_frame_present = 0;
	pes_holder->audio_present   = 0;
	pes_holder->data_present    = 0;
	pes_holder->fixed	    = 0;
	pes_holder->seq_fixed	    = 0;
	pes_holder->seq_added	    = 0;
	pes_holder->seq_number      = 0;

	pes_holder->repaired	    = 0;

	tmp_payload = pes_holder->payload;

	while(tmp_payload) {

		if(tmp_payload->payload_type != PES_VIDEO
			&& tmp_payload->payload_type != PES_MPEG
			&& tmp_payload->payload_type != PES_AC3) {
				pes_holder->size =  pes_holder->size + tmp_payload->size;
		}

		pes_holder->total_size = pes_holder->total_size + tmp_payload->size;

		if(tmp_payload->payload_type == SEQ_HEADER) {
			pes_holder->seq_present++;
		}

		if(tmp_payload->payload_type == GOP_HEADER) {
			pes_holder->gop_present++;
		}

		if(tmp_payload->payload_type == I_FRAME) {
			pes_holder->i_frame_present++;
		}

		if(tmp_payload->payload_type == P_FRAME) {
			pes_holder->p_frame_present++;
		}

		if(tmp_payload->payload_type == B_FRAME) {
			pes_holder->b_frame_present++;
		}

		if(tmp_payload->payload_type == CLOSED_CAPTION ||
			tmp_payload->payload_type == XDS_DATA ||
			tmp_payload->payload_type == IPREVIEW_DATA ||
			tmp_payload->payload_type == TELETEXT_DATA ) {
			pes_holder->data_present++;
		}

		if(tmp_payload->payload_type == MPEG_AUDIO_FRAME ||
			tmp_payload->payload_type == AC3_AUDIO_FRAME) {
			pes_holder->audio_present++;
		}
		tmp_payload = tmp_payload->next;
	}


}

/****************************************************************************/

payload_t * payload_fetch_seq_gop(const pes_holder_t * pes_holder) {

	pes_holder_t * tmp_pes_holder;

	payload_t * seq_payload= NULL;
	payload_t * gop_payload= NULL;
	payload_t * tmp_payload= NULL;

	int gotit;

	tmp_pes_holder = (pes_holder_t *)pes_holder;

	gotit = 0;



	while(tmp_pes_holder) {
		if(tmp_pes_holder->seq_present) {

			tmp_payload = tmp_pes_holder->payload;

			while(tmp_payload) {

				if(tmp_payload->payload_type == SEQ_HEADER) {
					seq_payload = duplicate_payload(tmp_payload);
					if(seq_payload) {
						gotit++;
					}
				}

				if(tmp_payload->payload_type == GOP_HEADER) {
					gop_payload = duplicate_payload(tmp_payload);
					if(gop_payload) {
						gotit++;
					}
				}

				tmp_payload = tmp_payload->next;
			}
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}


	if(gotit == 2) {
		seq_payload->previous = NULL;
		seq_payload->next = gop_payload;
		gop_payload->next = NULL;
		gop_payload->previous = seq_payload;
		//printf("MANAGED TO GET SEQ!!!!!!\n");
		return(seq_payload);
	}

	gotit = 0;
	if(seq_payload) {
		free_payload(seq_payload);
	}

	if(gop_payload) {
		free_payload(gop_payload);
	}

	seq_payload = NULL;
	gop_payload = NULL;

	if(!tmp_pes_holder->previous) {
		return(0);
	}


	while(tmp_pes_holder->previous) {
		if(tmp_pes_holder->seq_present) {

			tmp_payload = tmp_pes_holder->payload;

			while(tmp_payload) {

				if(tmp_payload->payload_type == SEQ_HEADER) {
					seq_payload = duplicate_payload(tmp_payload);
					if(seq_payload) {
						gotit++;
					}
				}

				if(tmp_payload->payload_type == GOP_HEADER) {
					gop_payload = duplicate_payload(tmp_payload);
					if(gop_payload) {
						gotit++;
					}
				}

				tmp_payload = tmp_payload->next;
			}
			break;
		}
		tmp_pes_holder = tmp_pes_holder->previous;
	}

	if(gotit == 2) {
		seq_payload->previous = NULL;
		seq_payload->next = gop_payload;
		gop_payload->next = NULL;
		gop_payload->previous = seq_payload;
		//printf("MANAGED TO GET SEQ in second attempt!!!!!!\n");
		return(seq_payload);
	}


	if(seq_payload) {
		free_payload(seq_payload);
	}

	if(gop_payload) {
		free_payload(gop_payload);
	}

	//printf("FAILD TO GET SEQ!!!!!!!!!!!!!\n");

	return(0);

}
/****************************************************************************/

void pes_holder_attache_drift_i_frame(const pes_holder_t * pes_holder, ticks_t time, ticks_t total_drift) {


	pes_holder_t * tmp_pes_holder;
	int gotit;

	tmp_pes_holder = (pes_holder_t *)pes_holder;
	gotit = 0;

	while(tmp_pes_holder->next) {
		if((tmp_pes_holder->time >= time) && tmp_pes_holder->i_frame_present) {
			tmp_pes_holder->drift = total_drift;
			gotit++;
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(gotit) {
		//printf("Gotit - " I64FORMAT " - " I64FORMAT "\n", total_drift, tmp_pes_holder->chunk_nr);
		return;
	}

	/* So we didn't find it :( */
	/* Lets attache it to the fist avalible I-Frame that we bounce into
	while traversing the linked list backwards - it should not be far from the
	real one */
	while(tmp_pes_holder->previous) {
		if(tmp_pes_holder->i_frame_present) {
			tmp_pes_holder->drift = total_drift;

			//printf("Gotit - " I64FORMAT " - " I64FORMAT " \n", total_drift, tmp_pes_holder->chunk_nr);
			return;
		}
		tmp_pes_holder = tmp_pes_holder->previous;
	}

	LOG_WARNING("Didn't manage to attache sync object\n");
	return;

}


/****************************************************************************/

static pes_holder_t * write_pes_holder_real_audio(tystream_holder_t * tystream, pes_holder_t * pes_holder, int * progress_meter_audio) {
	payload_t * payload;

	pes_holder_t * ret_pes_holder;

	pes_holder_t * tmp_pes_holder;
	payload_t * tmp_payload;
	payload_t * tmp_payload2;

	int64_t repeat;
	int64_t super_repeat;
	ticks_t i;

	payload = pes_holder->payload;

	super_repeat = 0;


	if(progress_meter_audio) {
		*progress_meter_audio = audio_pes_count;
		if(audio_pes_count == 100) {
			audio_pes_count = 0;
		} else {
			audio_pes_count++;
		}
	}


	/* Okay if we have a repeate then we most honer it otherwise we will loose sync */
	if(pes_holder->repeat) {
		if(pes_holder->repeat > 5) {
			/* Play audio twise */
			super_repeat = pes_holder->repeat - 1;
			repeat = 1;
		} else {
			repeat = pes_holder->repeat;
		}


		while(payload) {
			if(payload->payload_type == MPEG_AUDIO_FRAME || payload->payload_type == AC3_AUDIO_FRAME) {
				/* FIXME need to check audio size and pad if nessesary */
				if(tystream->write) {
					for(i = 0; i < repeat; i++) {
						if(payload->size == tystream->audio_frame_size) {
							if(tystream->std_alone) {
								write(tystream->audio_out_file, payload->payload_data, payload->size);
							}
#if !defined(TIVO)
							else {
								//printf("Audio writing %i - avalible %ld \n",payload->size , thread_pipe_freespace(tystream->audio_pipe));
								thread_pipe_write(tystream->audio_pipe,payload->size,payload->payload_data);
							}
#endif
						} else {
							LOG_WARNING("Warning: Audio frame size error - fetching a good one\n");
							tmp_payload = fetch_next_correct_audio_payload(tystream, pes_holder);
							if(tmp_payload) {
								if(tystream->std_alone) {
									write(tystream->audio_out_file, tmp_payload->payload_data, tmp_payload->size);
								}
#if !defined(TIVO)
								else {
									//printf("Audio writing %i - avalible %ld \n",payload->size , thread_pipe_freespace(tystream->audio_pipe));
									thread_pipe_write(tystream->audio_pipe,tmp_payload->size, tmp_payload->payload_data);
								}
#endif

							} else {
								LOG_ERROR("Error: Audio frame size error - unable to fetch a good one\n");
							}
						}
					}
				}
			}
			payload = payload->next;
		}


		if(super_repeat) {
			tmp_pes_holder = pes_holder;
			while(tmp_pes_holder && super_repeat > 0) {
				tmp_payload = tmp_pes_holder->payload;
				while(tmp_payload) {
					if(tmp_payload->payload_type == MPEG_AUDIO_FRAME || tmp_payload->payload_type == AC3_AUDIO_FRAME) {
						/* FIXME need to check audio size and pad if nessesary */
						if(tystream->write) {
							if(tmp_payload->size == tystream->audio_frame_size) {
								if(tystream->std_alone) {
									write(tystream->audio_out_file, tmp_payload->payload_data, tmp_payload->size);
								}
#if !defined(TIVO)
								else {
									thread_pipe_write(tystream->audio_pipe,tmp_payload->size, tmp_payload->payload_data);
								}
#endif

							} else {
								LOG_WARNING("Warning: Audio frame size error - fetching a good one\n");
								tmp_payload2 = fetch_next_correct_audio_payload(tystream, tmp_pes_holder);
								if(tmp_payload2) {
									if(tystream->std_alone) {
										write(tystream->audio_out_file, tmp_payload2->payload_data, tmp_payload2->size);
									}
#if !defined(TIVO)
									else {
										//printf("Audio writing %i - avalible %ld \n",payload->size , thread_pipe_freespace(tystream->audio_pipe));
										thread_pipe_write(tystream->audio_pipe,tmp_payload2->size, tmp_payload2->payload_data);
									}
#endif

								} else {
									LOG_ERROR("Error: Audio frame size error - unable to fetch a good one\n");
								}
							}
						}
						super_repeat--;
					}
					tmp_payload = tmp_payload->next;
				}
				tmp_pes_holder = tmp_pes_holder->next;
			}
		}
	}


	payload = pes_holder->payload;
	super_repeat = 0;
	repeat = 0;


	if(off_balance == 0) {
		if(pes_holder->next && (!pes_holder->next->repaired && !pes_holder->next->next->repaired)) {
			repeat = (pes_holder->next->time - pes_holder->time) / (tystream->audio_median_tick_diff -  (tystream->audio_median_tick_diff/40));
			if(pes_holder->time > pes_holder->next->time) {
				LOG_WARNING("WARNING: Audio frame is totally off balance\n");
				repeat = 1;
				off_balance =1;

			}
			if (repeat > 500) {
				repeat=1;
				LOG_WARNING("WARNING: A extensive amount of audio is missing - not compensating\n");
			}

			if(repeat > 5 && repeat < 500) {
				LOG_WARNING("WARNING: A extensive amount of audio is missing\n");
				LOG_WARNING1("WARNING: Compensating by playing audio segment twice %i - frames missing \n", repeat);
				super_repeat =  repeat - 1;
				repeat=1;
			}

			if(repeat > 1) {
				LOG_WARNING("WARNING: Audio frame(s) missing - compensating\n");
				LOG_DEVDIAG1("Time diff audio: " I64FORMAT "\n", pes_holder->next->time - pes_holder->time );
			}

		} else {
			repeat = 1;
		}
	} else {
		repeat = 1;
		off_balance=0;
	}

	/*
	if(pes_holder->next) {
		printf("Time diff audio: " I64FORMAT "\n", pes_holder->next->time - pes_holder->time );
		printf("Time diff audio: " I64FORMAT " - " I64FORMAT "\n", pes_holder->next->time, pes_holder->time );
	}
	*/

	while(payload) {
		if(payload->payload_type == MPEG_AUDIO_FRAME || payload->payload_type == AC3_AUDIO_FRAME) {
			/* FIXME need to check audio size and pad if nessesary */
			if(tystream->write) {
				for(i = 0; i < repeat; i++) {
					if(payload->size == tystream->audio_frame_size) {
						if(tystream->std_alone) {
							write(tystream->audio_out_file, payload->payload_data, payload->size);
						}
#if !defined(TIVO)
						else {
							//printf("Audio writing %i - avalible %ld \n",payload->size , thread_pipe_freespace(tystream->audio_pipe));
							thread_pipe_write(tystream->audio_pipe,payload->size,payload->payload_data);
						}
#endif
					} else {
						LOG_WARNING("Warning: Audio frame size error - fetching a good one\n");
						tmp_payload = fetch_next_correct_audio_payload(tystream, pes_holder);
						if(tmp_payload) {
							if(tystream->std_alone) {
								write(tystream->audio_out_file, tmp_payload->payload_data, tmp_payload->size);
							}
#if !defined(TIVO)
							else {
								//printf("Audio writing %i - avalible %ld \n",payload->size , thread_pipe_freespace(tystream->audio_pipe));
								thread_pipe_write(tystream->audio_pipe,tmp_payload->size, tmp_payload->payload_data);
							}
#endif

						} else {
							LOG_ERROR("Error: Audio frame size error - unable to fetch a good one\n");
						}
					}
				}
			}
		}
		payload = payload->next;
	}


	if(super_repeat) {
		tmp_pes_holder = pes_holder;
		while(tmp_pes_holder && super_repeat > 0) {
			tmp_payload = tmp_pes_holder->payload;
			while(tmp_payload) {
				if(tmp_payload->payload_type == MPEG_AUDIO_FRAME || tmp_payload->payload_type == AC3_AUDIO_FRAME) {
					/* FIXME need to check audio size and pad if nessesary */
					if(tystream->write) {
						if(tmp_payload->size == tystream->audio_frame_size) {
							if(tystream->std_alone) {
								write(tystream->audio_out_file, tmp_payload->payload_data, tmp_payload->size);
							}
#if !defined(TIVO)
							else {
								//printf("Audio writing %i - avalible %ld \n",payload->size , thread_pipe_freespace(tystream->audio_pipe));
								thread_pipe_write(tystream->audio_pipe,tmp_payload->size, tmp_payload->payload_data);
							}
#endif

						} else {
							LOG_WARNING("Warning: Audio frame size error - fetching a good one\n");
							tmp_payload2 = fetch_next_correct_audio_payload(tystream, tmp_pes_holder);
							if(tmp_payload2) {
								if(tystream->std_alone) {
									write(tystream->audio_out_file, tmp_payload2->payload_data, tmp_payload2->size);
								}
#if !defined(TIVO)
								else {
									//printf("Audio writing %i - avalible %ld \n",payload->size , thread_pipe_freespace(tystream->audio_pipe));
									thread_pipe_write(tystream->audio_pipe,tmp_payload2->size, tmp_payload2->payload_data);
								}
#endif

							} else {
								LOG_ERROR("Error: Audio frame size error - unable to fetch a good one\n");
							}
						}
					}
					super_repeat--;
				}
				tmp_payload = tmp_payload->next;
			}
			tmp_pes_holder = tmp_pes_holder->next;
		}
	}




	ret_pes_holder = pes_holder->next;
	free_pes_holder(pes_holder,1);


	return(ret_pes_holder);

}
/****************************************************************************/

static pes_holder_t * write_pes_holder_real_video(tystream_holder_t * tystream, pes_holder_t * pes_holder, int * progress_meter_video) {

	payload_t * payload;

	pes_holder_t * ret_pes_holder;

	payload = pes_holder->payload;

	if (pes_holder->i_frame_present + pes_holder->p_frame_present + pes_holder->b_frame_present > 1 && tystream->tivo_version != V_13) {
		LOG_WARNING("Error in pes payload more than one frame\n");
	}


	if(progress_meter_video) {
		*progress_meter_video = video_pes_count;
		if(video_pes_count == 100) {
			video_pes_count = 0;
		} else {
			video_pes_count++;
		}
	}



	while(payload) {

		if(payload->payload_type == SEQ_HEADER || payload->payload_type == GOP_HEADER ||
			payload->payload_type == I_FRAME || payload->payload_type == P_FRAME ||
			payload->payload_type == B_FRAME) {

			if(tystream->write) {
				if(tystream->std_alone) {
					write(tystream->video_out_file, payload->payload_data, payload->size);
				}
#if !defined(TIVO)
				else {
					//printf("Video writing %i - avalible %ld \n",payload->size , thread_pipe_freespace(tystream->video_pipe));
					//printf("Video writing %i\n",payload->size);
					thread_pipe_write(tystream->video_pipe,payload->size, payload->payload_data);
				}
#endif

			}
		}

		payload = payload->next;
	}


	ret_pes_holder = pes_holder->next;

	if(pes_holder->previous != NULL) {
		free_pes_holder(pes_holder->previous, 1);
		pes_holder->previous = NULL;
	}

	return(ret_pes_holder);
}


/****************************************************************************/
int   write_video_size(tystream_holder_t * tystream, pes_holder_t * pes_holder) {

	payload_t * payload;

	int return_size;

	payload = pes_holder->payload;




	return_size =0;

	while(payload) {

		if(payload->payload_type == SEQ_HEADER || payload->payload_type == GOP_HEADER ||
			payload->payload_type == I_FRAME || payload->payload_type == P_FRAME ||
			payload->payload_type == B_FRAME) {

			return_size += payload->size;

		}

		payload = payload->next;
	}


	return(return_size);

}


/****************************************************************************/


static int add_pes_holder_audio(tystream_holder_t * tystream, pes_holder_t * pes_holder) {

	pes_holder_t * tmp_pes_holder;

	if(tystream->pes_holder_audio == NULL) {
		tystream->pes_holder_audio = pes_holder;
	} else {
		tmp_pes_holder = tystream->pes_holder_audio;
		while(tmp_pes_holder->next) {
			tmp_pes_holder = tmp_pes_holder->next;
		}
		tmp_pes_holder->next = pes_holder;
		tmp_pes_holder->next->previous = tmp_pes_holder;
	}
	return(1);

}

/****************************************************************************/

static int add_pes_holder_video(tystream_holder_t * tystream, pes_holder_t * pes_holder) {


	pes_holder_t * tmp_pes_holder;

	if(tystream->pes_holder_video == NULL) {
		tystream->pes_holder_video = pes_holder;
	} else {
		tmp_pes_holder = tystream->pes_holder_video;
		while(tmp_pes_holder->next) {
			tmp_pes_holder = tmp_pes_holder->next;
		}
		tmp_pes_holder->next = pes_holder;
		tmp_pes_holder->next->previous = tmp_pes_holder;
	}
	return(1);
}

/****************************************************************************/

static payload_t * fetch_next_correct_audio_payload(tystream_holder_t * tystream, const pes_holder_t * pes_holder){


	pes_holder_t * tmp_pes_holder;
	payload_t * tmp_payload;

	tmp_pes_holder = (pes_holder_t * )pes_holder;

	while(tmp_pes_holder) {
		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {
			if((tmp_payload->payload_type == MPEG_AUDIO_FRAME || tmp_payload->payload_type == AC3_AUDIO_FRAME)
				&& tmp_payload->size == tystream->audio_frame_size) {
					return(tmp_payload);
			}
			tmp_payload = tmp_payload->next;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}

	
	/* Hmm so didn't manage to fetch one :( - very strange since there should be at lest a number of them in the stream */
	/* It might be so that we have a switch in audio size */

	return(0);


}


