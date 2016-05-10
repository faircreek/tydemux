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

#ifdef FRAME_RATE
uint8_t actual_frame_rate(ticks_t diff) {

	uint8_t frame_rate;

	if(diff < 7512 && diff > 7500) {
		if(ty_debug >= 5) {
			LOG_USERDIAG("Actual frame rate: 23.976 frames/sec\n");
		}
		frame_rate = 1;
	} else if (diff < 7501 && diff > 7696) {
		if(ty_debug >= 5) {
			LOG_USERDIAG("Actual frame rate: 24 frames/sec\n");
		}
		frame_rate = 2;
	} else if (diff < 7210 && diff > 7190) {
		if(ty_debug >= 5) {
			LOG_USERDIAG("Actual frame rate: 25 frames/sec\n");
		}
		frame_rate = 3;
	} else if (diff < 6012 && diff > 6000) {
		if(ty_debug >= 5) {
			LOG_USERDIAG("Actual frame rate: 29.97 frames/sec\n");
		}
		frame_rate = 4;
	} else if (diff < 6001 && diff > 5996) {
		if(ty_debug >= 5) {
			LOG_USERDIAG("Actual frame rate: 30 frames/sec\n");
		}
		frame_rate = 5;
	} else if (diff < 3610 && diff > 3590) {
		if(ty_debug >= 5) {
			LOG_USERDIAG("Actual frame rate: 50 frames/sec\n");
		}
		frame_rate = 6;
	} else if (diff < 3006 && diff > 3000) {
		if(ty_debug >= 5) {
			LOG_USERDIAG("Actual frame rate: 59.94 frames/sec\n");
		}
		frame_rate = 7;
	} else if (diff < 3001 && diff > 2996) {
		if(ty_debug >= 5) {
			LOG_USERDIAG("Actual frame rate: 60 frames/sec\n");
		}
		frame_rate = 8;
	} else {
		if(ty_debug >= 2) {
			LOG_USERDIAG("Actual frame rate unknown!!\n");
		}
		frame_rate = 0;
	}

	return(frame_rate);

}

int set_frame_rate(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* Maker */
	int gotit;

	/* Pes holder */
	pes_holder_t * tmp_pes_holder;

	/* Payload holder */
	payload_t * tmp_payload;

	/* Init */
	gotit = 0;
	tmp_pes_holder  = (pes_holder_t *)seq_pes_holder;
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
		return(0);
	}

	/* Reset present frame rate */

	tmp_payload->payload_data[7] = tmp_payload->payload_data[7] & 0xf0;

	/* Set frame rate */
	tmp_payload->payload_data[7] = tmp_payload->payload_data[7] | tystream->frame_rate;

	return(1);

}

uint8_t control_frame_rate(const pes_holder_t * pes_holder) {

	/* Marker */
	int gotit;

	/* Payload */
	payload_t * tmp_payload;

	/* Frame rate */
	uint8_t frame_rate;

	/* Init */
	tmp_payload = pes_holder->payload;
	gotit = 0;
	frame_rate =0;

	while(tmp_payload) {
		if(tmp_payload->payload_type == SEQ_HEADER) {
			frame_rate = tmp_payload->payload_data[7] & 0x0f;
			gotit =1;
			break;
		}
		tmp_payload = tmp_payload->next;
	}

	if(gotit) {
		switch(frame_rate) {
			case 1:
				if(ty_debug >= 7) {
					LOG_USERDIAG("Frame rate: 23.976 frames/sec\n");
				}
				return((uint8_t)1);
				break;
			case 2:
				if(ty_debug >= 7) {
					LOG_USERDIAG("Frame rate: 24 frames/sec\n");
				}
				return((uint8_t)2);
				break;
			case 3:
				if(ty_debug >= 7) {
					LOG_USERDIAG("Frame rate: 25 frames/sec\n");
				}
				return((uint8_t)3);
				break;
			case 4:
				if(ty_debug >= 7) {
					LOG_USERDIAG("Frame rate: 29.97 frames/sec\n");
				}
				return((uint8_t)4);
				break;
			case 5:
				if(ty_debug >= 7) {
					LOG_USERDIAG("Frame rate: 30 frames/sec\n");
				}
				return((uint8_t)5);
				break;
			case 6:
				if(ty_debug >= 7) {
					LOG_USERDIAG("Frame rate: 50 frames/sec\n");
				}
				return((uint8_t)6);
				break;
			case 7:
				if(ty_debug >= 7) {
					LOG_USERDIAG("Frame rate: 59.94 frames/sec\n");
				}
				return((uint8_t)7);
				break;
			case 8:
				if(ty_debug >= 7) {
					LOG_USERDIAG("Frame rate: 60 frames/sec\n");
				}
				return((uint8_t)0);
				break;
			default:
				if(ty_debug >= 1) {
					LOG_USERDIAG("Forbidden frame rate!!\n");
				}
				return(0);
		}

	} else {
		if(ty_debug >= 2) {
			LOG_WARNING("Error did find SEQ \n");
		}
		return((uint8_t)0);
	}

}

int check_frame_rate_fix_header(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* Maker */
	int gotit;
	int first;
	int change;

	/* Iteration */
	uint16_t counter;

	/* Pes holder */
	pes_holder_t * tmp_pes_holder;

	/* Payload holder*/
	payload_t * tmp_payload;

	/* Results from funcs */
	int result;

	/* Number of frame rate errors */
	int error;

	/* Frame Rate */
	uint8_t frame_rate;

	/* Number of frames */
	uint16_t highest_tmp_ref;

	/* Times */
	ticks_t first_time;
	ticks_t second_time;
	ticks_t time;
	ticks_t diff;

	/* Init */
	error = 0;
	gotit = 0;
	change = 0;

	highest_tmp_ref = get_highest_tmp_ref(seq_pes_holder);

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
		if(ty_debug >= 7 ) {
			LOG_DEVDIAG1("Didn't find SEQ, %i\n", tmp_pes_holder->seq_present);
		}
		return(-1);
	}

	/* Check and set the correct frame rate */
	frame_rate = control_frame_rate(tmp_pes_holder);

	if(!frame_rate) {
		return(-1);
	}
	if(ty_debug >= 7) {
		LOG_DEVDIAG1("Frame rate %i \n",control_frame_rate(tmp_pes_holder));
	}

	if(tystream->frame_rate != frame_rate) {
		result = set_frame_rate(tystream, tmp_pes_holder);
		if(!result) {
			return(-1);
		}
	}

	if(ty_debug >= 7) {
		LOG_DEVDIAG1("Control start: " I64FORMAT "\n", seq_pes_holder->seq_number);
	}

	if(ty_debug >= 7) {
		LOG_DEVDIAG1("Frame rate %i \n",control_frame_rate(tmp_pes_holder));
	}

	/* Hmm, lets see if tystream->second_time is zero if so we
	need to init the times */

	if(tystream->time_init == 0 || seq_pes_holder->repaired) {
		first_time = get_time_of_tmp_ref(tmp_pes_holder, 0);
		second_time = get_time_of_tmp_ref(tmp_pes_holder, 1);
		tystream->time_init = 1;
		if(ty_debug >= 7) {
			LOG_DEVDIAG2("First time " I64FORMAT " Second " I64FORMAT "\n",first_time,second_time);
		}
		counter = 2;
		first = 2;
	} else {
		first = 0;
		first_time = get_time_of_tmp_ref(tmp_pes_holder, - 2);
		second_time = get_time_of_tmp_ref(tmp_pes_holder, - 1);
		counter = 0;
	}

	for(;counter < highest_tmp_ref + 1; counter++) {

		time = get_time_of_tmp_ref(tmp_pes_holder,counter);
		if(ty_debug >= 7) {
			LOG_DEVDIAG1("Time is " I64FORMAT "\n", time);
		}

		/* Frame rate is messured over three frames
		due to 3-2 downpull diff variations*/

		diff = time - first_time;
		first++;
		if(ty_debug >= 7) {
			LOG_DEVDIAG1("Diff " I64FORMAT " \n", diff);
		}
		frame_rate = actual_frame_rate(diff);

		if(frame_rate != tystream->frame_rate) {
			if(ty_debug >= 7) {
				LOG_DEVDIAG("Not the right frame rate\n");
			}
			error++;
		}

		if(first >=4 && tystream->print_frame_rate) {
			if(tystream->present_frame_rate != frame_rate) {
				change++;
			}
			if(change > 20 && tystream->present_frame_rate != frame_rate) {
				LOG_DEVDIAG3("Frame rate change at " I64FORMAT " from %i to %i\n", time,tystream->present_frame_rate, frame_rate);
				tystream->present_frame_rate = frame_rate;
			}
		}

		first_time = second_time;
		second_time = time;
	}
	if(ty_debug >= 7) {
		LOG_DEVDIAG1("Control end: " I64FORMAT "\n", seq_pes_holder->seq_number);
	}
	return(error);
}

/**
 * Small note about frame rate fixing - firstly it's only on DTivo
 * SA Tivo uses the same frame rate all the time - 29.97 or 25
 * depending if it's a UK or US SA Tivo.
 *
 * Now if we have a recoding that mostly is 29.97 we will need
 * to fix the parts that are 23.976 and wise versa.
 * Doing the convention from 29.97 to 23.976 involves a mapping
 * five frames to four (removing) and doing a 23.976 to 29.97
 * involves mapping four frames to five frames (adding)
 *
 * Now we can only add and remove B-Frames without doing other
 * major changes to the stream hence we will need to figure out
 * how do that in a good way
 *
 * Well to be perfect we need to do motion estimation when adding
 * B-Frames but it's way to complicated for our little app :).
 * Also I did some experiments trippleing the amout of B-Frames
 * and it works very well even without motion estimation
 *
 */





int fix_frame_rate(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* We have to different ways of doing this
	 either we have a main frame rate of 23.976
	 and then we have to remove frames.

	 or we have a main frame rate of 29.97 and
	 we have to add frames */

	if(tystream->frame_rate == 1 && tystream->tivo_type == DTIVO) {
		/* 23.976 and we bounced into 29.97 hence remove*/
		return(fix_frame_rate_remove(tystream, seq_pes_holder));
	} else if (tystream->frame_rate == 4 && tystream->tivo_type == DTIVO) {
		/* 29.97 and we bounced into 23.976 hence add */
		return(fix_frame_rate_add(tystream, seq_pes_holder));
	} else {
		return(0);
	}
}



int fix_frame_rate_remove(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* Maker */
	int gotit;

	/* Iteration */
	uint16_t counter;


	/* Number of frame rate errors */
	int error;

	/* Frame Rate */
	uint8_t frame_rate;

	/* Frame Counter*/
	uint16_t frame_counter;

	/* Number of frames */
	uint16_t highest_tmp_ref;

	/* Pes and Payload holder */
	pes_holder_t * tmp_pes_holder;
	payload_t * tmp_payload;

	/* Times */
	ticks_t first_time;
	ticks_t second_time;
	ticks_t time;
	ticks_t diff;

	/* Holders of time and frame types */

	payload_type_t frame_type_frame_3;
	payload_type_t frame_type_frame_4;


	ticks_t time_frame_0;
	ticks_t time_frame_1;
	ticks_t time_frame_2;

	ticks_t new_time_frame_2;
	ticks_t new_time_frame_3;
	ticks_t new_time_frame_4;


	/* Init */
	error = 0;
	gotit = 0;
	if(ty_debug >= 7) {
		LOG_DEVDIAG("Start frame rate\n");
	}
	frame_counter = tystream->frame_counter;
	highest_tmp_ref = get_highest_tmp_ref(seq_pes_holder);

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
		return(0);
	}


	/* Hmm, lets see if tystream->second_time is zero if so we
	need to init the times */

	if(tystream->time_init_second == 0 || seq_pes_holder->repaired) {
		first_time = get_time_of_tmp_ref(tmp_pes_holder, 0);
		second_time = get_time_of_tmp_ref(tmp_pes_holder, 1);
		tystream->time_init_second = 1;
		counter = 2;

	} else {
		//first_time = tystream->first_time;
		//second_time = tystream->second_time;
		first_time = get_time_of_tmp_ref(tmp_pes_holder, - 2);
		second_time = get_time_of_tmp_ref(tmp_pes_holder, - 1);
		counter = 0;
	}
	if(ty_debug >= 7) {
		LOG_DEVDIAG2("First time " I64FORMAT " Second " I64FORMAT "\n",first_time,second_time);
	}

	/* Frame counter is zero lets init it */
	if(!frame_counter || seq_pes_holder->repaired ) {
		time = get_time_of_tmp_ref(tmp_pes_holder, counter);
		/* Okay let examine if we have a problem at the start */
		diff = time - first_time;
		if(ty_debug >= 7) {
			LOG_DEVDIAG1("Diff " I64FORMAT " \n", diff);
		}
		first_time = second_time;
		second_time = time;
		if(tystream->frame_rate  != actual_frame_rate(diff)) {
			/* Huston we have a problem */
			if(ty_debug >= 7) {
				LOG_DEVDIAG("Inited frame counter\n");
			}
			frame_counter = 1;
		} else {
			frame_counter = 0;
		}
		counter++;
	} else {
		if(ty_debug >= 7) {
			LOG_DEVDIAG1("frame_counter: %i \n",frame_counter);
		}
	}




	for(;counter < highest_tmp_ref + 1;) {

		time = get_time_of_tmp_ref(tmp_pes_holder,counter);
		if(ty_debug >= 7) {
			LOG_DEVDIAG1("Time: " I64FORMAT " \n", time);
		}
		/* Frame rate is messured over three frames
		due to 3-2 downpull diff variations*/

		diff = time - first_time;
		if(ty_debug >= 7) {
			LOG_DEVDIAG1("Time " I64FORMAT " \n", time);
			LOG_DEVDIAG1("First Time " I64FORMAT "\n", first_time);
			LOG_DEVDIAG1("Diff " I64FORMAT " \n", diff);
		}
		frame_rate = actual_frame_rate(diff);

		if(frame_rate != tystream->frame_rate) {
			frame_counter++;
		} else {
			frame_counter = 0;
			counter++;
			first_time = second_time;
			second_time = time;
			continue;
		}

		if(frame_counter >= 3) {
			if(ty_debug >= 7) {
				LOG_DEVDIAG("In repair\n");
			}
			frame_type_frame_3 = get_frame_type_of_tmp_ref(tmp_pes_holder, counter - 1);
			frame_type_frame_4 = get_frame_type_of_tmp_ref(tmp_pes_holder, counter);

			time_frame_0 = get_time_of_tmp_ref(tmp_pes_holder, counter - 4);
			time_frame_1 = get_time_of_tmp_ref(tmp_pes_holder, counter - 3);
			time_frame_2 = get_time_of_tmp_ref(tmp_pes_holder, counter - 2);
			if(ty_debug >= 7) {
				LOG_DEVDIAG3("time 0 " I64FORMAT ", time 1 " I64FORMAT ", time 2 " I64FORMAT "\n",time_frame_0,time_frame_1,time_frame_2);
			}
			if((tystream->frame_removed == 3 && frame_type_frame_4 == B_FRAME) ||
				(frame_type_frame_4 == B_FRAME && frame_type_frame_3 != B_FRAME) ||
				(counter == 0 && frame_type_frame_4 == B_FRAME)) {
				/* Remove this frame 4 */
				/* Now frame 2 need to move fwd so the diff between
				0 and 2 7506 */
				new_time_frame_2 =  time_frame_0 + 7506;
				new_time_frame_3 = time_frame_1  + 7506;
				set_time_of_tmp_ref(tmp_pes_holder, counter - 2, new_time_frame_2);
				set_time_of_tmp_ref(tmp_pes_holder, counter - 1, new_time_frame_3);

				remove_frame(tmp_pes_holder, counter);
				fix_tmp_ref_removing_frame(tmp_pes_holder, 1, counter);
				tystream->frame_removed = 4;

				first_time = new_time_frame_2;
				second_time = new_time_frame_3;

				highest_tmp_ref = highest_tmp_ref -1;
				frame_counter = 0;

			} else if (counter > 0 && ((tystream->frame_removed == 4 && frame_type_frame_3 == B_FRAME) ||
				(frame_type_frame_4 != B_FRAME && frame_type_frame_3 == B_FRAME))) {

				/* Remove frame 3 i.e. previous frame */

				new_time_frame_2 =  time_frame_0 + 7506;
				new_time_frame_4 = time_frame_1  + 7506;
				set_time_of_tmp_ref(tmp_pes_holder, counter - 2, new_time_frame_2);
				set_time_of_tmp_ref(tmp_pes_holder, counter, new_time_frame_4);

				remove_frame(tmp_pes_holder, (uint16_t)(counter - 1));
				fix_tmp_ref_removing_frame(tmp_pes_holder, 1, (uint16_t)(counter - 1));
				tystream->frame_removed = 3;

				first_time = new_time_frame_2;
				second_time = new_time_frame_4;

				highest_tmp_ref = highest_tmp_ref -1;
				frame_counter = 0;


			} else {
				/* We are @#$ :( - what to do ?? - contiue we will
				later on catch it with the sync drift */
				if(ty_debug >= 7 ) {
					LOG_DEVDIAG("ERROR in frame remove \n");
				}
				counter++;
				first_time = second_time;
				second_time = time;
			}
		} else {
			counter++;
			first_time = second_time;
			second_time = time;
			if(ty_debug >= 7) {
				LOG_DEVDIAG("Continue\n");
			}
		}
	}
	if(ty_debug >= 7) {
		LOG_DEVDIAG("End frame rate\n");
	}
	tystream->frame_counter = frame_counter;

	return(1);

}


/*********************************/







int fix_frame_rate_add(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* Maker */
	int gotit;

	/* Iteration */
	uint16_t counter;


	/* Number of frame rate errors */
	int error;

	/* Number of frames */
	uint16_t highest_tmp_ref;

	/* Pes and Payload holder */
	pes_holder_t * tmp_pes_holder;
	payload_t * tmp_payload;


	/* Frame Rate */
	uint8_t frame_rate;

	/* Frame Counter*/
	uint16_t frame_counter;


	/* Times */
	ticks_t first_time;
	ticks_t second_time;
	ticks_t time;
	ticks_t diff;

	/* Holders of time and frame types */
	payload_type_t frame_type_frame_2;
	payload_type_t frame_type_frame_3;


	ticks_t time_frame_0;

	ticks_t new_time_frame_1;
	ticks_t new_time_frame_2;
	ticks_t new_time_frame_3;
	ticks_t new_time_frame_4;

	/* Init */
	error = 0;
	gotit = 0;

	frame_counter = tystream->frame_counter;
	highest_tmp_ref = get_highest_tmp_ref(seq_pes_holder);


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
		return(0);
	}

	/* Hmm, lets see if tystream->second_time is zero if so we
	need to init the times */

	if(ty_debug >= 7) {
		LOG_DEVDIAG("Repair frame rate \n");
	}

	if(tystream->time_init_second == 0 || seq_pes_holder->repaired) {
		first_time = get_time_of_tmp_ref(tmp_pes_holder, 0);
		second_time = get_time_of_tmp_ref(tmp_pes_holder, 1);
		tystream->time_init_second = 1;
		counter = 2;

	} else {
		//first_time = tystream->first_time;
		//second_time = tystream->second_time;
		first_time = get_time_of_tmp_ref(tmp_pes_holder, - 2);
		second_time = get_time_of_tmp_ref(tmp_pes_holder, - 1);
		counter = 0;
	}


	/* Frame counter is zero lets init it */
	if(!frame_counter || seq_pes_holder->repaired ) {
		time = get_time_of_tmp_ref(tmp_pes_holder, counter);
		/* Okay let examine if we have a problem at the start */
		diff = time - first_time;

		if(ty_debug >= 7) {
			LOG_DEVDIAG4("Time " I64FORMAT " Second Time " I64FORMAT " First Time " I64FORMAT " Diff " I64FORMAT " \n",time, first_time,second_time, diff);
		}

		first_time = second_time;
		second_time = time;
		if(tystream->frame_rate  != actual_frame_rate(diff)) {
			/* Huston we have a problem */
			if(ty_debug >= 7) {
				LOG_DEVDIAG("Inited frame counter\n");
			}
			frame_counter = 1;
		} else {
			frame_counter = 0;
		}
		counter++;
	} else {
		if(ty_debug >= 7) {
			LOG_DEVDIAG1("frame_counter: %i \n",frame_counter);
		}
	}




	for(;counter < highest_tmp_ref + 1;) {

		time = get_time_of_tmp_ref(tmp_pes_holder,counter);
		if(ty_debug >= 7) {
			LOG_DEVDIAG1("Time: " I64FORMAT " \n", time);
		}
		/* Frame rate is messured over three frames
		due to 3-2 downpull diff variations*/

		diff = time - first_time;
		if(ty_debug >= 7) {
			LOG_DEVDIAG1("Time " I64FORMAT " ", time);
			LOG_DEVDIAG1("Second Time " I64FORMAT " ", second_time);
			LOG_DEVDIAG1("First Time " I64FORMAT " ", first_time);
			LOG_DEVDIAG1("Diff " I64FORMAT " \n", diff);
		}
		frame_rate = actual_frame_rate(diff);

		if(frame_rate != tystream->frame_rate) {
			frame_counter++;

		} else {
			frame_counter = 0;
			counter++;
			first_time = second_time;
			second_time = time;
			continue;
		}


		if(frame_counter >= 3) {
			if(ty_debug >= 7) {
				LOG_DEVDIAG("Repair\n");
			}
			frame_type_frame_2 = get_frame_type_of_tmp_ref(tmp_pes_holder, counter - 1);
			frame_type_frame_3 = get_frame_type_of_tmp_ref(tmp_pes_holder, counter);

			time_frame_0 = get_time_of_tmp_ref(tmp_pes_holder, counter - 3);

			if(ty_debug >= 7) {
				LOG_DEVDIAG1("Time frame 0 " I64FORMAT "\n",time_frame_0);
			}

			if((tystream->frame_added == 2 && frame_type_frame_3 == B_FRAME) ||
				(frame_type_frame_3 == B_FRAME && frame_type_frame_2 != B_FRAME)) {

				/* Duplicate frame 3 and added it as frame 4 */

				new_time_frame_1 = time_frame_0 + 3003;
				new_time_frame_2 = new_time_frame_1 + 3003;
				new_time_frame_3 = new_time_frame_2 + 3003;
				new_time_frame_4 = new_time_frame_3 + 3003;


				add_frame(tystream, tmp_pes_holder, counter);

				if(ty_debug >= 7 ) {
					LOG_DEVDIAG("before Fixing tmp_ref of add\n");
					print_seq(seq_pes_holder);
				}

				fix_tmp_ref_adding_frame(tmp_pes_holder, 1, (uint16_t)(counter + 1));

				if(ty_debug >= 7 ) {
					LOG_DEVDIAG("Fixing tmp_ref of add\n");
					print_seq(seq_pes_holder);
				}


				set_time_of_tmp_ref(tmp_pes_holder, counter - 2, new_time_frame_1);
				set_time_of_tmp_ref(tmp_pes_holder, counter - 1, new_time_frame_2);
				set_time_of_tmp_ref(tmp_pes_holder, counter,     new_time_frame_3);
				set_time_of_tmp_ref(tmp_pes_holder, counter + 1, new_time_frame_4);


				tystream->frame_added = 3;

				first_time = new_time_frame_3;
				second_time = new_time_frame_4;

				highest_tmp_ref = highest_tmp_ref + 1;
				frame_counter = 0;
				counter = counter + 2;

				if(ty_debug >= 7) {
					check_frame_rate(tystream, seq_pes_holder);
				}

			} else if ((tystream->frame_added == 3 && frame_type_frame_2 == B_FRAME) ||
				(frame_type_frame_3 != B_FRAME && frame_type_frame_2 == B_FRAME)) {

				/* Duplicate frame 2 and added it as frame 3 */
				new_time_frame_1 = time_frame_0 + 3003;
				new_time_frame_2 = new_time_frame_1 + 3003;
				new_time_frame_3 = new_time_frame_2 + 3003;
				new_time_frame_4 = new_time_frame_3 + 3003;

				add_frame(tystream, tmp_pes_holder, (uint16_t)(counter - 1));
				fix_tmp_ref_adding_frame(tmp_pes_holder, 1, counter);

				set_time_of_tmp_ref(tmp_pes_holder, counter - 2, new_time_frame_1);
				set_time_of_tmp_ref(tmp_pes_holder, counter - 1, new_time_frame_2);
				set_time_of_tmp_ref(tmp_pes_holder, counter,     new_time_frame_3);
				set_time_of_tmp_ref(tmp_pes_holder, counter + 1, new_time_frame_4);


				tystream->frame_added = 2;

				first_time = new_time_frame_3;
				second_time = new_time_frame_4;

				highest_tmp_ref = highest_tmp_ref + 1;
				frame_counter = 0;
				counter = counter + 2;
				if(ty_debug >= 7) {
					check_frame_rate(tystream, seq_pes_holder);
				}



			} else {
				/* We are @#$ :( - what to do ?? - contiue we will
				later on catch it with the sync drift */

				if(ty_debug >= 2) {
					LOG_ERROR("Error we didn't catch the frame shift\n");
				}

				counter++;
				first_time = second_time;
				second_time = time;
			}
		} else {
			counter++;
			first_time = second_time;
			second_time = time;
		}
	}

	tystream->frame_counter = frame_counter;
	if(ty_debug >= 7) {
		LOG_DEVDIAG("End Repair frame rate \n");
	}
	return(1);

}


/* Pure test function don't use it !! */

int check_frame_rate(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder) {


	/* Maker */
	int gotit;

	/* Iteration */
	uint16_t counter;

	/* Pes holder */
	pes_holder_t * tmp_pes_holder;

	/* Payload holder*/
	payload_t * tmp_payload;


	/* Number of frame rate errors */
	int error;

	/* Frame Rate */
	uint8_t frame_rate;

	/* Number of frames */
	uint16_t highest_tmp_ref;

	/* Times */
	ticks_t first_time;
	ticks_t second_time;
	ticks_t time;
	ticks_t diff;

	/* Init */
	error = 0;
	gotit = 0;

	highest_tmp_ref =get_highest_tmp_ref(seq_pes_holder);

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
		if(ty_debug >= 7 ) {
			LOG_DEVDIAG("Did find SEQ\n");
		}
		return(-1);
	}

	/* Check and set the correct frame rate */
	frame_rate = control_frame_rate(tmp_pes_holder);

	if(!frame_rate) {
		return(-1);
	}
	if(ty_debug >= 7) {
		LOG_DEVDIAG1("Frame rate %i \n",control_frame_rate(tmp_pes_holder));
	}


	/* Hmm, lets see if tystream->second_time is zero if so we
	need to init the times */

	if(tystream->time_init_third == 0 || seq_pes_holder->repaired) {
		first_time = get_time_of_tmp_ref(tmp_pes_holder, 0);
		second_time = get_time_of_tmp_ref(tmp_pes_holder, 1);
		tystream->time_init_third = 1;
		if(ty_debug >= 7) {
			LOG_DEVDIAG2("First time " I64FORMAT " Second " I64FORMAT "\n",first_time,second_time);
		}
		counter = 2;
	} else {
		first_time = get_time_of_tmp_ref(tmp_pes_holder, - 2);
		second_time = get_time_of_tmp_ref(tmp_pes_holder, - 1);
		//first_time = tystream->first_time_check;
		//second_time = tystream->second_time_check;
		counter = 0;
	}
	if(ty_debug >= 1) {
		LOG_DEVDIAG1("Check start: " I64FORMAT "\n", seq_pes_holder->seq_number);
	}
	for(;counter < highest_tmp_ref + 1; counter++) {

		time = get_time_of_tmp_ref(tmp_pes_holder,counter);
		if(ty_debug >= 7) {
			LOG_DEVDIAG1("Time is " I64FORMAT "\n", time);
		}

		/* Frame rate is messured over three frames
		due to 3-2 downpull diff variations*/

		diff = time - first_time;

		if(ty_debug >= 7) {
			LOG_DEVDIAG1("Diff " I64FORMAT " \n", diff);
		}

		frame_rate = actual_frame_rate(diff);

		if(frame_rate != tystream->frame_rate) {
			if(ty_debug >= 7) {
				LOG_DEVDIAG("Not the right frame rate\n");
			}
			error++;
		}
		first_time = second_time;
		second_time = time;
	}
	if(ty_debug >= 7) {
		LOG_DEVDIAG1("Check end: " I64FORMAT "\n", seq_pes_holder->seq_number);
	}


	return(error);
}



#endif
