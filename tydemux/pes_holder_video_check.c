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
static pes_holder_t * check_fix_p_frame(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
static pes_holder_t * check_fix_tmp_ref(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
static pes_holder_t * check_fix_fields(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
static pes_holder_t * check_fix_av_drift(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
static pes_holder_t * check_fix_gop_frame_counter(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);

pes_holder_t * next_seq_holder(const pes_holder_t * seq_pes_holder){

	pes_holder_t * tmp_pes_holder;
	int gotit;

	gotit = 0;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	while(tmp_pes_holder) {
		if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
			if(ty_debug >= 9) {
				LOG_DEVDIAG2("B %i, I %i \n", \
					tmp_pes_holder->b_frame_present, \
					tmp_pes_holder->i_frame_present);
			}
			gotit = 1;
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}
	if(!gotit) {
		//printf("MAJOR ERROR\n");
		return(0);
	}

	/* Return the next seq pes holder */
	return(tmp_pes_holder);

}

pes_holder_t * previous_seq_holder(const pes_holder_t * seq_pes_holder){

	pes_holder_t * tmp_pes_holder;
	int gotit;

	gotit = 0;

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	while(tmp_pes_holder) {
		if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
			if(ty_debug >= 9) {
				LOG_DEVDIAG2("B %i, I %i \n", \
					tmp_pes_holder->b_frame_present, \
					tmp_pes_holder->i_frame_present);
			}
			gotit = 1;
			break;
		}
		tmp_pes_holder = tmp_pes_holder->previous;
	}
	if(!gotit) {
		//printf("MAJOR ERROR\n");
		return(0);
	}

	/* Return the next seq pes holder */
	return(tmp_pes_holder);

}


int check_fix_pes_holder_video(tystream_holder_t * tystream) {


	pes_holder_t * result;

	if(tystream->repair) {
		return(0);
	}

	if(!tystream->find_seq) {
		return(0);
	}

	/* Move SEQ/GOP header from last B-Frame in pre SEQ to first I-Frame in the SEQ */
	result = tystream->pes_holder_video;

	if(tystream->lf_seq_pes_holder) {
		LOG_DEVDIAG1("SEQ header - chunk " I64FORMAT "\n", tystream->lf_seq_pes_holder->chunk_nr);
		//printf("SEQ header - chunk " I64FORMAT "\n", tystream->lf_seq_pes_holder->chunk_nr);
	}

	while(result) {
		if(tystream->lf_seq_pes_holder) {
			result = fix_seq_header(tystream, next_seq_holder(tystream->lf_seq_pes_holder));
		} else {
			result = fix_seq_header(tystream, tystream->pes_holder_video);
		}
	}


	/* Fix P-Frame in SA V_2X Tivo;s */
	result = tystream->pes_holder_video;
	if(tystream->lf_seq_pes_holder) {

		if(tystream->lf_p_frame_pes_holder) {
			LOG_DEVDIAG1("P-Frame - chunk " I64FORMAT "\n",tystream->lf_p_frame_pes_holder->chunk_nr);
			//printf("P-Frame - chunk " I64FORMAT "\n",tystream->lf_p_frame_pes_holder->chunk_nr);
		}

		while(result) {
			if(tystream->lf_p_frame_pes_holder) {
				result = check_fix_p_frame(tystream, next_seq_holder(tystream->lf_p_frame_pes_holder));
			} else {
				result = check_fix_p_frame(tystream, tystream->pes_holder_video);
			}
		}
	}



	/* Check and fix temporal reference and number of frames */
	result = tystream->pes_holder_video;
	if(tystream->lf_p_frame_pes_holder) {

		if(tystream->lf_tmp_ref_pes_holder) {
			LOG_DEVDIAG1("Temporal - chunk " I64FORMAT "\n", tystream->lf_tmp_ref_pes_holder->chunk_nr);
			//printf("Temporal - chunk " I64FORMAT "\n", tystream->lf_tmp_ref_pes_holder->chunk_nr);
 		}


		while(result) {
			if(tystream->lf_tmp_ref_pes_holder) {
				result = check_fix_tmp_ref(tystream, next_seq_holder(tystream->lf_tmp_ref_pes_holder));
			} else {
				result = check_fix_tmp_ref(tystream, tystream->pes_holder_video);
			}
		}
	}



	/* Check and fix fields */
	result = tystream->pes_holder_video;
	if (tystream->lf_tmp_ref_pes_holder) {

		if(tystream->lf_field_pes_holder) {
			LOG_DEVDIAG1("Field - chunk " I64FORMAT "\n", tystream->lf_field_pes_holder->chunk_nr);
			//printf("Field - chunk " I64FORMAT "\n", tystream->lf_field_pes_holder->chunk_nr);
 		}

		while(result) {
			if(tystream->lf_field_pes_holder) {
				//printf("Normal field\n");
				result = check_fix_fields(tystream, next_seq_holder(tystream->lf_field_pes_holder));
			} else {
				//printf("Init field\n");
				result = check_fix_fields(tystream, tystream->pes_holder_video);
			}
		}

	}


	/* Check and fix AV drift */
	result = tystream->pes_holder_video;
	if(tystream->lf_field_pes_holder) {
		if(tystream->lf_av_drift_pes_holder_video) {
			LOG_DEVDIAG("AV drift\n");
			//printf("AV - chunk " I64FORMAT " \n",tystream->lf_av_drift_pes_holder_video->chunk_nr);
		}

		while(result) {
			if(tystream->lf_av_drift_pes_holder_video) {
				result = check_fix_av_drift(tystream, next_seq_holder(tystream->lf_av_drift_pes_holder_video));
			} else {
				result = check_fix_av_drift(tystream, tystream->pes_holder_video);
			}
		}
	} else {
		//printf("NO AV drift field pes holder is null\n");
	}


	/* Frame counter so we can set the GOP header */

	result = tystream->pes_holder_video;
	if (tystream->lf_av_drift_pes_holder_video) {

		if(tystream->lf_gop_frame_counter_pes_holder_video) {
			LOG_DEVDIAG1("Field - chunk " I64FORMAT "\n", tystream->lf_field_pes_holder->chunk_nr);
			//printf("Field - chunk " I64FORMAT "\n", tystream->lf_field_pes_holder->chunk_nr);
 		}

		while(result) {
			if(tystream->lf_gop_frame_counter_pes_holder_video) {
				//printf("Normal field\n");
				result = check_fix_gop_frame_counter(tystream, next_seq_holder(tystream->lf_gop_frame_counter_pes_holder_video));
			} else {
				//printf("Init field\n");
				result = check_fix_gop_frame_counter(tystream, tystream->pes_holder_video);
			}
		}

	}



	return(1);

}

/********************************************************************************************************/

static pes_holder_t * check_fix_p_frame(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* pes holder */
	pes_holder_t * tmp_pes_holder;
	payload_t * tmp_payload;

	/* Markers */
	int gotit;

	/* Results */
	int result;

	/* Tmp ref to get times */
	int16_t tmp_ref;

	/* Times */
	ticks_t time_1;

	picture_info_t picture_info;

	/* Init */
	gotit = 0;

	//printf("P Frame " I64FORMAT " \n", seq_pes_holder->seq_number);

	if(tystream->tivo_series == S1) {
		result = is_okay(seq_pes_holder, tystream->lf_seq_pes_holder);
		if(!result) {
			LOG_DEVDIAG("P Frame not okay\n");
			//LOG_DEVDIAG1("SEQ of tmp_ref " I64FORMAT " \n",tystream->lf_tmp_ref_pes_holder->seq_number);
			return(0);
		}
	}

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	while(tmp_pes_holder) {
		if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
			if(tmp_pes_holder->b_frame_present) {
				/* This one is not yet fixed in the SEQ header */
				return(0);
			}
			gotit = 1;
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(!gotit) {
		/* This is the last one */
		LOG_DEVDIAG("Didn't find next SEQ\n");
		return(0);
	}



	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	gotit = 0;
	while(tmp_pes_holder) {

		if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
			gotit = 1;
			break;
		}

		if(!tmp_pes_holder->time && tmp_pes_holder->p_frame_present) {
			tmp_payload = tmp_pes_holder->payload;
			while(tmp_payload) {
				if(tmp_payload->payload_type == P_FRAME) {
					tmp_ref = tmp_payload->tmp_ref;
					time_1 = get_time_of_tmp_ref(seq_pes_holder, tmp_ref - 1);
					get_picture_info_tmp_ref(tystream, seq_pes_holder, tmp_ref -1, &picture_info);

					if(picture_info.repeat_first_field &&  picture_info.progressive_frame) {

						tmp_pes_holder->time = time_1 + tystream->frame_tick  + tystream->frame_tick/2;
					} else {
						tmp_pes_holder->time = time_1 + tystream->frame_tick;
					}

					break;
				}
				tmp_payload = tmp_payload->next;
			}

			if(!tmp_payload) {
				LOG_DEVDIAG("Major error in fix of p frame\n");
			}
		}

		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(!gotit) {
		return(0);
	}

	tystream->lf_p_frame_pes_holder = seq_pes_holder;
	return(next_seq_holder(seq_pes_holder));

}



/*********************************************************************************************************/

static pes_holder_t * check_fix_tmp_ref(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* pes holder */
	pes_holder_t * tmp_pes_holder;

	/* Markers */
	int gotit;

	/* Results */
	int result;

	/* Init */
	gotit = 0;

	//printf("Tmp ref " I64FORMAT " \n", seq_pes_holder->seq_number);



	result = is_okay(seq_pes_holder, tystream->lf_p_frame_pes_holder);
	if(!result) {
		LOG_DEVDIAG("Tmpref not okay\n");
		//printf("Not ready from P \n");
		return(0);
	}



	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	/* This check is nessesary for series 2 which doesn't have seq_fix function */
	LOG_DEVDIAG1("Tmp ref check for chunk: " I64FORMAT "\n", seq_pes_holder->chunk_nr);
	while(tmp_pes_holder) {
		if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
			if(tmp_pes_holder->b_frame_present) {
				/* This one is not yet fixed in the SEQ header */
				LOG_DEVDIAG1("Tmp ref: seq not fixed - " I64FORMAT " \n", tmp_pes_holder->seq_number);
				LOG_DEVDIAG2("Next pes is: seq_p %i, iframe %i\n", tmp_pes_holder->next->seq_present, tmp_pes_holder->next->i_frame_present);
				//printf("SEQ not fixed \n");
				return(0);
			}
			gotit = 1;
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(!gotit) {
		/* This is the last one */
		LOG_DEVDIAG1("Tmp ref: Didn't find next SEQ: %i\n", tmp_pes_holder->seq_present);
		//printf("Didn't findnext seq \n");
		return(0);
	}

	LOG_DEVDIAG1("Tmp ref seq: " I64FORMAT "\n", tmp_pes_holder->seq_number);
	//printf("Tmp ref seq: " I64FORMAT "\n", tmp_pes_holder->seq_number);

	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	result = check_temporal_reference(tmp_pes_holder);

	if(result) {

		LOG_WARNING("Temporal Reference Error - Repairing\n");
		//printf("Temporal Reference Error - Repairing\n");
		//print_seq(tmp_pes_holder);

		/* We have an error
		lets check if there
		was an error in either
		the number of frames
		or the tmp ref*/

		result = check_nr_frames_tmp_ref(tmp_pes_holder);
		//print_seq(tmp_pes_holder);

		if(result != 0) {

			if(result > 0) {
				LOG_WARNING("The Temporal Reference error is a overflow in frames\n");
				//printf("The Temporal Reference error is a overflow in frames\n");
				result = correct_overflow_in_frames(tystream, tmp_pes_holder, result);
				if(result) {
					/* Check if we have a underflow now - it can very well be since we
					missed frames in the SEQ/GOP boundary */
					tmp_pes_holder = (pes_holder_t *)seq_pes_holder;
					result = check_nr_frames_tmp_ref(tmp_pes_holder);
					//printf("NR FRAMES %i\n", result);
					//print_seq(tmp_pes_holder);
					if(result < 0) {
						/* Okay so we had a under flow - let us fix it */
						result = correct_underflow_in_frames(tystream, tmp_pes_holder, result, 0);
						//printf("Result is %i\n", result);
						//print_seq(tmp_pes_holder);
						if(result) {
							/* We need to exit here */
							//printf("Unable to repair overflow\n");
							LOG_FATAL("Unable to repair overflow\n");
							exit(1);
						} else {
							//printf("Overflow in frames repaird\n");
							LOG_WARNING("Overflow in frames repaird\n");
						}
					} else {

						//printf("Unable to repair overflow\n");
						LOG_FATAL("Unable to repair overflow\n");
						exit(1);
					}
				} else {
					//print_seq(tmp_pes_holder);
					LOG_WARNING("Overflow in frames repaird\n");
				}
				//print_seq(tmp_pes_holder);
			} else {
				LOG_WARNING("The Temporal Reference error is a underflow in frames\n");
				//print_seq(tmp_pes_holder);
				result = correct_underflow_in_frames(tystream, tmp_pes_holder, result, 0);
				//print_seq(tmp_pes_holder);
				if(result) {
					LOG_FATAL("Unable to repair underflowflow\n");
					exit(1);
					//print_seq(tmp_pes_holder);
				} else {
					LOG_WARNING("Underflow in frames repaired\n");
				}
			}
		} else {

			//print_seq(tmp_pes_holder);
			//printf("Pure tmp ref error\n");
			//fflush(stdout);
			//sleep(10);
			LOG_WARNING("Pure tmp ref error\n");
			result = fix_tmp_ref(tystream, tmp_pes_holder);
			if(!result) {
				LOG_FATAL("Unable to repair temporal reference error\n");
				exit(1);
			} else {
				LOG_WARNING("Temporal Reference error repaired\n");
			}
			//print_seq(tmp_pes_holder);
		}
	} else if (tmp_pes_holder->seq_added) {
		//print_seq(tmp_pes_holder);

		correct_underflow_in_frames(tystream, tmp_pes_holder, result, 0);
		//print_seq(tmp_pes_holder);

	}

	tystream->lf_tmp_ref_pes_holder = seq_pes_holder;
	//print_seq(tystream->lf_tmp_ref_pes_holder);
	return(next_seq_holder(seq_pes_holder));

}

/*********************************************************************************************/

static pes_holder_t * check_fix_fields(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* pes holder */
	pes_holder_t * tmp_pes_holder;

	/* Markers */
	int gotit;

	/* Results */
	int result;

	/* Init */
	gotit = 0;

	//printf("Fields " I64FORMAT " \n", seq_pes_holder->seq_number);


	result = is_okay(seq_pes_holder, tystream->lf_tmp_ref_pes_holder);
	if(!result) {
		LOG_DEVDIAG("field not okay\n");
		LOG_DEVDIAG1("SEQ nr of p frame is " I64FORMAT "\n",tystream->lf_p_frame_pes_holder->seq_number );
		return(0);
	}


	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	while(tmp_pes_holder) {
		if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
			if(tmp_pes_holder->b_frame_present) {
				/* This one is not yet fixed in the SEQ header */
				LOG_DEVDIAG("SEQ not fixed!!\n");
				return(0);
			}
			gotit = 1;
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(!gotit) {
		/* This is the last one */
		LOG_DEVDIAG1("Didn't find next SEQ: %i\n", tmp_pes_holder->seq_present);
		return(0);
	}



	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	result = check_fields_in_seq(tystream, tmp_pes_holder);

	if(result) {
		/* FIXME */
		LOG_DEVDIAG("Fix of field count yet not implemented\n");
	}


	tystream->lf_field_pes_holder = seq_pes_holder;
	//print_seq(tystream->lf_field_pes_holder);
	return(next_seq_holder(seq_pes_holder));

}



/******************************************************************************************************************/

static pes_holder_t * check_fix_av_drift(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* pes holder */
	pes_holder_t * tmp_pes_holder;

	/* Markers */
	int gotit;


	/* Results */
	int result;

	/* Init */
	gotit = 0;

	//printf("AV Drift " I64FORMAT " \n", seq_pes_holder->seq_number);

	result = is_okay(seq_pes_holder, tystream->lf_field_pes_holder);
	if(!result) {
		LOG_DEVDIAG("drift not okay\n");
		return(0);
	}


	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	while(tmp_pes_holder) {
		if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
			if(tmp_pes_holder->b_frame_present) {
				/* This one is not yet fixed in the SEQ header */
				return(0);
			}
			gotit = 1;
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(!gotit) {
		/* This is the last one */
		LOG_DEVDIAG("Didn't find next SEQ\n");
		return(0);
	}


	if(seq_pes_holder->drift || seq_pes_holder->next->drift) {
		/* okay we have a drift value from a cut or repair
		 A positive value here means that we
		 have to much audio hence we lack video.

		 Now normaly in the check_av_drift a negative
		 value means that we have to much audio
		 hence we lack video.

		 What we do is to take the dift value from the pes
		 and do a * -1 to turn it into check_av_drift
		 value */
		if(!seq_pes_holder->drift) {
			seq_pes_holder->drift = seq_pes_holder->next->drift;
		}

		LOG_USERDIAG2("Got a drift objec - adding drift " I64FORMAT " to tydrift " I64FORMAT "\n"\
			,seq_pes_holder->drift, tystream->drift);

		tystream->drift = tystream->drift + (seq_pes_holder->drift * -1);

		LOG_USERDIAG2("Got a drift objec - after adding drift " I64FORMAT " to tydrift " I64FORMAT "\n"\
			,seq_pes_holder->drift, tystream->drift);

	}



	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;


	/* Pure test REMOVE FIXME */
	/*if(ty_debug >= 7 ) {
		check_frame_rate(tystream, tmp_pes_holder);
	}*/

	if(ty_debug >= 7) {
		//print_seq(tmp_pes_holder);
	}

	/*result = check_temporal_reference(tmp_pes_holder);

	if(result) {
		if(ty_debug >= 2) {
			printf("error in tmp_ref sync_fix 1\n");
			print_tmp_ref(tmp_pes_holder);
		}
	}*/

	//check_sync_drift(tystream, tmp_pes_holder);
	result = check_av_drift_in_seq(tystream, tmp_pes_holder);

	if(result) {
		/* FIXME */
			LOG_DEVDIAG("We had a field time error - fix not yet implemented\n");
			//print_seq(seq_pes_holder);
	}

#if 0
	if(tmp_pes_holder->repaired) {
		printf("Seq before print\n");
				print_seq(previous_seq_holder(tmp_pes_holder));
				print_seq(tmp_pes_holder);
				print_seq(next_seq_holder(tmp_pes_holder));
	}
#endif

	if(llabs(tystream->drift) >= tystream->drift_threshold) {
		//print_seq(seq_pes_holder);



		LOG_DEVDIAG2("SEQ: " I64FORMAT " Drift before fix is: " I64FORMAT " ticks\n", seq_pes_holder->seq_number, tystream->drift);
		//print_seq(seq_pes_holder);

		/* Don't print this drift on UK Tivo since it's always drifting drifting */

		if(tystream->frame_rate != 3) {
			LOG_USERDIAG("A/V sync is drifting beyond allowed parameters - repairing\n");
		}
		fix_drift(tystream, seq_pes_holder);

		if(tystream->frame_rate != 3) {
			LOG_USERDIAG("A/V sync repaired sync is now within allowed parameters \n");
		}
		//print_seq(seq_pes_holder);
		/*
		print_seq(previous_seq_holder(seq_pes_holder));
		print_seq(seq_pes_holder);
		print_seq(next_seq_holder(seq_pes_holder));
		*/
		LOG_DEVDIAG2("SEQ: " I64FORMAT " Drift fixed is: " I64FORMAT " ticks\n", seq_pes_holder->seq_number, tystream->drift);

		result = check_temporal_reference(seq_pes_holder);

		if(result) {
			//print_seq(seq_pes_holder);
			LOG_WARNING("error in tmp_ref sync check\n");
		} else {
			//print_seq(seq_pes_holder);
		}
#if 0
				printf("Seq drift print\n");
				print_seq(previous_seq_holder(tmp_pes_holder));
				print_seq(tmp_pes_holder);
				print_seq(next_seq_holder(tmp_pes_holder));
#endif
	}

#if 0
	if(tmp_pes_holder->repaired) {
				printf("Seq after print\n");
				print_seq(previous_seq_holder(tmp_pes_holder));
				print_seq(tmp_pes_holder);
				print_seq(next_seq_holder(tmp_pes_holder));
	}

#endif
	tystream->lf_av_drift_pes_holder_video = seq_pes_holder;

	return(next_seq_holder(seq_pes_holder));

}

/**********************************************************************************/

static pes_holder_t * check_fix_gop_frame_counter(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* pes holder */
	pes_holder_t * tmp_pes_holder;

	/* Markers */
	int gotit;

	/* Results */
	int result;

	/* Init */
	gotit = 0;

	//printf("Fields " I64FORMAT " \n", seq_pes_holder->seq_number);


	result = is_okay(seq_pes_holder, tystream->lf_av_drift_pes_holder_video);
	if(!result) {
		LOG_DEVDIAG("gop frame not okay\n");
		LOG_DEVDIAG1("SEQ nr of p frame is " I64FORMAT "\n",tystream->lf_av_drift_pes_holder_video->seq_number );
		return(0);
	}


	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	while(tmp_pes_holder) {
		if(tmp_pes_holder->seq_present && tmp_pes_holder != seq_pes_holder) {
			if(tmp_pes_holder->b_frame_present) {
				/* This one is not yet fixed in the SEQ header */
				LOG_DEVDIAG("SEQ not fixed!!\n");
				return(0);
			}
			gotit = 1;
			break;
		}
		tmp_pes_holder = tmp_pes_holder->next;
	}

	if(!gotit) {
		/* This is the last one */
		LOG_DEVDIAG1("Didn't find next SEQ: %i\n", tmp_pes_holder->seq_present);
		return(0);
	}



	tmp_pes_holder = (pes_holder_t *)seq_pes_holder;

	result = count_frame_fix_gop(tystream, tmp_pes_holder);

	if(result) {
		/* FIXME */
		LOG_DEVDIAG("Fix of field count yet not implemented\n");
	}


	tystream->lf_gop_frame_counter_pes_holder_video = seq_pes_holder;

	return(next_seq_holder(seq_pes_holder));

}
