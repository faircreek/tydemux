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

static pes_holder_t * check_start_seq_pes_okay(pes_holder_t * seq_pes_holder);
static int frametotc(int gop_timecode0_frame, uint8_t frame_rate);

int set_smpte_field(const tystream_holder_t * tystream, const pes_holder_t * pes_holder) {

	/* Marker */
	int gotit;
	int tc;
	int tmp;
	int bit;
	int i, j;
	/* Payload */
	payload_t * tmp_payload;

	/* Init */
	tmp_payload = pes_holder->payload;
	gotit = 0;

	while(tmp_payload) {
		if(tmp_payload->payload_type == GOP_HEADER) {
			tc = frametotc(tystream->frame_counter, tystream->frame_rate);
			//printf("%08x\n", tc);
			if(tc == -1) {
				//printf("Error in gop time\n");
				return(0);
			}
			for(i=0, j=56; i < 25; i++, j--) {
				tmp = tc;
				bit = tc >> i;
				//printf("%i\n",bit);
				bit = bit & 0x00000001;
				//printf("Bit %i - value %i\n",j, bit);
				setbit(tmp_payload->payload_data, j, bit);
			}
			return(1);
			break;
		}
		tmp_payload = tmp_payload->next;
	}

	LOG_WARNING("Error did find GOP \n");
	return(0);

}


int count_frame_fix_gop(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {

	/* Markers */
	int seq_header;
	int frames;
	int reminder;


	/* Payload and pes holders */
	payload_t * tmp_payload;
	pes_holder_t * tmp_pes_holder;



	/* Init */
	seq_header = 0;
	frames = 0;
	reminder = 0;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	if(!tmp_pes_holder->gop_present) {
		return(0);
	}

	while(tmp_pes_holder) {

		tmp_payload = tmp_pes_holder->payload;
		while(tmp_payload) {

			/* Run into the next SEQ header */
			if((tmp_payload->payload_type == SEQ_HEADER ||
				tmp_payload->payload_type == GOP_HEADER)
				&& tmp_pes_holder != seq_pes_holder) {
				if(tystream->frame_field_reminder) {
					if(reminder) {
						tystream->frame_counter = tystream->frame_counter + frames + 1;
						tystream->frame_field_reminder = 0;
					} else {
						tystream->frame_counter = tystream->frame_counter + frames;
					}
				} else {
					if(reminder) {
						tystream->frame_counter = tystream->frame_counter + frames;
						tystream->frame_field_reminder = 1;
					} else {
						tystream->frame_counter = tystream->frame_counter + frames;
					}
				}
				//printf("Okay - %i \n", tystream->frame_counter);
				return(1);
			}

			if(tmp_payload->payload_type == SEQ_HEADER) {
				seq_header = 1;
			}

			if(tmp_payload->payload_type == GOP_HEADER && seq_header) {
				if(!set_smpte_field(tystream, tmp_pes_holder)) {
					LOG_ERROR("Error while setting smtpe time\n");
				} else {
					//printf("Setting SMTPE\n");
				}
			}

			if((tmp_payload->payload_type == I_FRAME ||
				tmp_payload->payload_type == P_FRAME ||
				tmp_payload->payload_type == B_FRAME) && seq_header) {
				if(tmp_payload->fields == TBT || tmp_payload->fields == BTB) {
					if(reminder) {
						frames = frames + 2;
						reminder = 0;
					} else {
						frames++;
						reminder++;
					}
				} else {
					frames++;
				}
			}

			tmp_payload = tmp_payload->next;
		}


		tmp_pes_holder = tmp_pes_holder->next;
	}
	printf("Not Okay - %i \n", tystream->frame_counter);
	return(0);

}


int set_gop_closed_bit(const pes_holder_t * pes_holder) {

	/* Marker */
	int gotit;

	/* Payload */
	payload_t * tmp_payload;

	/* Init */
	tmp_payload = pes_holder->payload;
	gotit = 0;

	while(tmp_payload) {
		if(tmp_payload->payload_type == GOP_HEADER) {
			setbit(tmp_payload->payload_data, 57 ,1); //57
			gotit =1;
			break;
		}
		tmp_payload = tmp_payload->next;
	}

	if(gotit) {
		return(1);
	} else {
		if(ty_debug >= 2) {
			LOG_WARNING("Error did find GOP \n");
		}
		return(0);
	}

}

int is_closed_gop(const pes_holder_t * pes_holder) {

	/* Marker */
	int gotit;

	/* Return value */
	int gop_bit;

	/* Payload */
	payload_t * tmp_payload;

	/* Init */
	tmp_payload = pes_holder->payload;
	gotit = 0;
	gop_bit = -1;
	while(tmp_payload) {
		if(tmp_payload->payload_type == GOP_HEADER) {
			gop_bit = getbit(tmp_payload->payload_data, 57);
			gotit =1;
			break;
		}
		tmp_payload = tmp_payload->next;
	}

	if(gotit) {
		return(gop_bit);
	} else {
		if(ty_debug >= 2) {
			LOG_WARNING("Error did find GOP \n");
		}
		return(-1);
	}

}




pes_holder_t * gop_make_closed(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder, data_times_t * video_times){


	/* Markers */
	int got_i_frame;
	int got_p_frame;
//	int temp_ref_error;
//	int temp_ref_nr_frames_error;
	int seq_header;

	/* B Frame counter */
	uint16_t nr_b_frames;

	/* Payload and pes holders */
	payload_t * tmp_payload;
	payload_t * tmp_payload_2;
	payload_t * tmp_free_payload;
	pes_holder_t * tmp_pes_holder;

	/* Init */
	got_i_frame = 0;
	got_p_frame = 0;
	seq_header = 0;
	nr_b_frames = 0;
#if 0
	if(tystream->first_gop && tystream->tivo_series == S2) {
		set_gop_closed_bit(seq_pes_holder);
		temp_ref_error = check_temporal_reference(seq_pes_holder);
		temp_ref_nr_frames_error = check_nr_frames_tmp_ref(seq_pes_holder);

		LOG_DEVDIAG2("First GOP: tmp_ref_error %i frames_error %i\n",temp_ref_error,temp_ref_nr_frames_error);

		tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

		if( temp_ref_error >= 1 && temp_ref_nr_frames_error == 0) {
			LOG_WARNING("Tmp ref error in first gop\n");
			/*FIXME  write func to fix */

		} else  if(temp_ref_error >= 1 && temp_ref_nr_frames_error >= 1) {
			/* FIXME */
			LOG_WARNING("Frame count error - Fixing in frame rate check\n");
			temp_ref_error = 0;
		}
		while(tmp_pes_holder) {
			tmp_payload = tmp_pes_holder->payload;
			while(tmp_payload) {
				if(tmp_payload->payload_type == SEQ_HEADER) {
					seq_header = 1;
				}
				if (tmp_payload->payload_type == I_FRAME && seq_header) {
					video_times->data_start = tmp_pes_holder->time;
				}
				tmp_payload = tmp_payload->next;
			}
			tmp_pes_holder = tmp_pes_holder->next;
		}

		tystream->first_gop = 0;

		tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

		while(tmp_pes_holder) {
			tmp_payload = tmp_pes_holder->payload;
			while(tmp_payload) {
				if(tmp_payload->payload_type == SEQ_HEADER) {
					return(tmp_pes_holder);
				}
				tmp_payload = tmp_payload->next;
			}
			tmp_pes_holder = tmp_pes_holder->next;
		}
		return(0);
	}
#endif

	seq_pes_holder = check_start_seq_pes_okay(seq_pes_holder);

	if(!seq_pes_holder) {
		LOG_ERROR("Error to seq_pes_holder is null after check\n");
		LOG_ERROR("Error: Hence not enough framres to close the gop\n");
		return(0);
	}

	//print_seq(seq_pes_holder);

	if(check_temporal_reference(seq_pes_holder)) {
		LOG_ERROR("Error: Temporal reference error when trying to close gop\n");
		return(0);
	}

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;
	//printf("First GOP: tmp_ref_error %i frames_error %i\n",temp_ref_error,temp_ref_nr_frames_error);
	//printf("B-Frame %i, I-Frame %i\n",seq_pes_holder->b_frame_present, seq_pes_holder->i_frame_present);
	//printf("NEXT: B-Frame %i, I-Frame %i\n",seq_pes_holder->next->b_frame_present, seq_pes_holder->next->i_frame_present);


	/* FIXME */
#if 0
	if(temp_ref_error) {
		LOG_WARNING("Tmp ref error in close gop\n");
		//print_seq(seq_pes_holder);
	}

	if( temp_ref_error >= 1 && temp_ref_nr_frames_error == 0) {
		LOG_WARNING("Tmp ref error in close gop\n");


	} else  if(temp_ref_error >= 1 && temp_ref_nr_frames_error >= 1) {
		LOG_WARNING("close gop Frame count error - Fixing in frame rate check\n");
		temp_ref_error = 0;
	}
#endif

	/* Most of the time we have a B-Frame-SEQ-GOP or the P-Frame-SEQ-GOP in the pes
	holder - what we need to do is to remove the B/P-Frame
	and then move the SEQ-GOP to the I-Frame PES holder */

	while(tmp_pes_holder) {

		tmp_payload = tmp_pes_holder->payload;

		while(tmp_payload) {

			if(tmp_payload->payload_type == SEQ_HEADER) {
				seq_header = 1;
			}

			if(tmp_payload->payload_type == B_FRAME && got_p_frame != 1) {

				/* This one is junk and needs to be dumped */
				if(tmp_payload->previous) {
					if(tmp_payload->next) {
						tmp_payload_2 = tmp_payload->previous;
						tmp_payload_2->next = tmp_payload->next;
						tmp_payload->next->previous = tmp_payload_2;
						tmp_free_payload = tmp_payload;
						tmp_payload = tmp_payload_2->next;
					} else {
						tmp_payload_2 = tmp_payload->previous;
						tmp_payload_2->next = NULL;
						tmp_free_payload = tmp_payload;
						tmp_payload = tmp_payload_2->next;
					}
				} else {
					if(tmp_payload->next) {
						tmp_pes_holder->payload = tmp_payload->next;
						tmp_pes_holder->payload->previous = NULL;
						tmp_free_payload = tmp_payload;
						tmp_payload = tmp_pes_holder->payload;

					} else {
						/* there is no payload beside this one?? */
						tmp_pes_holder->payload = NULL;
						tmp_free_payload = tmp_payload;
						tmp_payload = tmp_pes_holder->payload;
					}
				}
				if(seq_header) {
					nr_b_frames++;
				}
				tmp_pes_holder->b_frame_present--;
				free_payload(tmp_free_payload);
				continue;

			} else if ((tmp_payload->payload_type == P_FRAME || tmp_payload->payload_type == I_FRAME)  && got_p_frame != 1 && got_i_frame) {
				got_p_frame = 1;
				break;

			} else if (tmp_payload->payload_type == P_FRAME && got_p_frame != 1 && got_i_frame != 1) {

				/* Shit we will need to remove this P frame*/
				if(tmp_payload->previous) {
					if(tmp_payload->next) {
						tmp_payload_2 = tmp_payload->previous;
						tmp_payload_2->next = tmp_payload->next;
						tmp_payload->next->previous = tmp_payload_2;
						tmp_free_payload = tmp_payload;
						tmp_payload = tmp_payload_2->next;
					} else {
						tmp_payload_2 = tmp_payload->previous;
						tmp_payload_2->next = NULL;
						tmp_free_payload = tmp_payload;
						tmp_payload = tmp_payload_2->next;
					}
				} else {
					if(tmp_payload->next) {
						tmp_pes_holder->payload = tmp_payload->next;
						tmp_pes_holder->payload->previous = NULL;
						tmp_free_payload = tmp_payload;
						tmp_payload = tmp_pes_holder->payload;

					} else {
						/* there is no payload beside this one?? */
						tmp_pes_holder->payload = NULL;
						tmp_free_payload = tmp_payload;
						tmp_payload = tmp_pes_holder->payload;
					}
				}

				tmp_pes_holder->p_frame_present--;
				free_payload(tmp_free_payload);
				continue;

			} else if (tmp_payload->payload_type == I_FRAME) {
				video_times->data_start = tmp_pes_holder->time;
				got_i_frame = 1;
			}

			tmp_payload = tmp_payload->next;
		}

		if(got_p_frame) {
			break;
		} else {
			tmp_pes_holder = tmp_pes_holder->next;
		}

	}

	if(nr_b_frames) {
		LOG_DEVDIAG("Fixing tmp ref\n");
		fix_tmp_ref_after_close_gop(seq_pes_holder, nr_b_frames);
	}



	if(got_p_frame) {
		LOG_DEVDIAG2("GOP %i SEQ %i\n", seq_pes_holder->gop_present, seq_pes_holder->seq_present);
		set_gop_closed_bit(seq_pes_holder);
		return(seq_pes_holder);
	} else {
		return(0);
	}
}

pes_holder_t * move_seq_to_i_frame(pes_holder_t *seq_pes_holder) {

	/* Marker */
	int gotit;

	/* Payload */
	payload_t * tmp_payload;
	payload_t * i_pes_tmp_payload;
	payload_t * i_tmp_payload;

	/* I-Frame holder */
	pes_holder_t  * i_pes_holder;

	/* Init */
	tmp_payload = seq_pes_holder->payload;
	gotit = 0;

	if(seq_pes_holder->seq_present && seq_pes_holder->next && seq_pes_holder->next->i_frame_present) {

		while(tmp_payload) {
			if(tmp_payload->payload_type == SEQ_HEADER) {
				gotit = 1;
				break;
			}
			tmp_payload = tmp_payload->next;
		}

		if(!gotit) {
			return(0);
		}
		gotit =0;

		/* Turncate the payload before the SEQ */
		if(tmp_payload->previous) {
			//printf("We have something \n");
			tmp_payload->previous->next = NULL;
		} else {
			LOG_DEVDIAG("Hmm is this really like this \n");
			/* If no prev then we know it's the start */
			seq_pes_holder->payload = NULL;
		}

		tmp_payload->previous = NULL;

		i_pes_holder = seq_pes_holder->next;
		i_pes_tmp_payload = i_pes_holder->payload;

		if(i_pes_holder->seq_added) {
			/* This means that we have added a missing I frame
			and we will need to just attache the SEQ at the
			very start */
			i_tmp_payload = i_pes_tmp_payload;

			i_pes_holder->payload = tmp_payload;
			tmp_payload->previous = NULL;

		} else {


			/* Insert SEQ/GOP between PES_VIDEO header and I FRAME */

			while(i_pes_tmp_payload) {
				if(i_pes_tmp_payload->payload_type == PES_VIDEO) {
					gotit = 1;
					break;
				}
				i_pes_tmp_payload = i_pes_tmp_payload->next;
			}

			if(!gotit) {
				LOG_WARNING("No way of moving SEQ\n");
				return(0);
			}


			gotit = 0;

			i_tmp_payload = i_pes_tmp_payload->next;
			i_pes_tmp_payload->next = tmp_payload;
			tmp_payload->previous = i_pes_tmp_payload;
		}


		seq_pes_holder->size = seq_pes_holder->size - tmp_payload->size;
		seq_pes_holder->total_size = seq_pes_holder->total_size - tmp_payload->size;

		i_pes_holder->size = i_pes_holder->size + tmp_payload->size;
		i_pes_holder->total_size = i_pes_holder->total_size + tmp_payload->size;

		seq_pes_holder->seq_present--;
		seq_pes_holder->gop_present--;

		i_pes_holder->seq_present++;
		i_pes_holder->gop_present++;


		/* ffw to end of tmp_payload  - attache I Frame */
		tmp_payload = tmp_payload->next;

		while(tmp_payload) {
			seq_pes_holder->size = seq_pes_holder->size - tmp_payload->size;
			seq_pes_holder->total_size = seq_pes_holder->total_size - tmp_payload->size;

			i_pes_holder->size = i_pes_holder->size + tmp_payload->size;
			i_pes_holder->total_size = i_pes_holder->total_size + tmp_payload->size;
			if(tmp_payload->next) {
				tmp_payload = tmp_payload->next;
			} else {
				break;
			}
		}

		i_tmp_payload->previous = tmp_payload;
		tmp_payload->next = i_tmp_payload;

		//LOG_DEVDIAG1("Next pes holder %i\n", i_pes_holder->next->b_frame_present);


		return(i_pes_holder);

	} else {
		return(0);
	}
}

int check_fix_progressive_seq(tystream_holder_t * tystream, payload_t * payload) {


	/* Sizes and offsets */
	int offset;

	/* Pointer to magic with */
	uint8_t * pt1;



	offset = find_extension_in_payload(tystream, payload, (uint8_t)0x01);

	if(offset == -1) {
		return(0);
	}

	pt1 = payload->payload_data;
	pt1 = pt1 + offset + 4;

	setbit(pt1, 12, 1);

	return(1);

}


static pes_holder_t * check_start_seq_pes_okay(pes_holder_t * seq_pes_holder) {

	int nr_frames;

	pes_holder_t * tmp_pes_holder;

	tmp_pes_holder = seq_pes_holder;

	nr_frames = 0;

	while(tmp_pes_holder) {

		if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
			break;
		}

		nr_frames++;
		tmp_pes_holder = tmp_pes_holder->next;
	}

	/* We need 6 frames high counted to start the stream */
	if(nr_frames > 4) {
		return(seq_pes_holder);
	} else if (!tmp_pes_holder) {
		return(0);
	}

	//return(check_start_seq_pes_okay(tmp_pes_holder));

	// OLAF - Re my email - I've put return(0) for now but please check
	return(0);
}

static int frametotc(int gop_timecode0_frame, uint8_t frame_rate) {

	int frame = gop_timecode0_frame;
	int fps, pict, sec, minute, hour, tc;

	/* Note: no drop_frame_flag support here, so we're simply rounding
	the frame rate as per 6.3.8 13818-2 */
	switch(frame_rate) {
		case 0:
			/* Invalid */
			return(-1);
		case 1:
		case 2:
			fps = 24;
			break;
		case 3:
			fps = 25;
			break;
		case 4:
		case 5:
			fps = 30;
			break;
		case 6:
			fps = 50;
			break;
		case 7:
		case 8:
			fps = 60;
			break;
	}

	pict = frame%fps;
	frame = (frame-pict)/fps;
	sec = frame%60;
	frame = (frame-sec)/60;
	minute = frame%60;
	frame = (frame-minute)/60;
	hour = frame%24;
	tc = (hour<<19) | (minute<<13) | (1<<12) | (sec<<6) | pict;

        return tc;
}
