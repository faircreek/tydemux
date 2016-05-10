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


static int probe_tystream_S2(tystream_holder_t * tystream);
static int probe_tystream_audio(tystream_holder_t * tystream);
static int probe_audio_tick_size(tystream_holder_t * tystream);
static uint8_t actual_frame_rate(ticks_t diff);
static int probe_tystream_frame_rate(tystream_holder_t * tystream);
static ticks_t get_time_pre_b_frame(tystream_holder_t * tystream, chunk_t * chunk, int i);
static ticks_t get_time_post_b_frame(tystream_holder_t * tystream, chunk_t * chunk, int i);
static chunk_t * copy_chunk(chunk_t * chunk);
static void clear_tystream_audio_probe(tystream_holder_t * tystream, int ready);



int tivo_probe_tystream(tystream_holder_t * tystream) {


	if(tystream->chunks == NULL) {
			fprintf(stderr, "Big ERROR - NO CHUNKS\n");
		clear_tystream_audio_probe(tystream, 0);
		return(0);
	}

	if(!probe_tystream_S2(tystream)) {
			fprintf(stderr, "Big ERROR - CAN'T DETECT S1/S2\n");
		clear_tystream_audio_probe(tystream, 0);
		return(0);
	}


	if(!probe_tystream_frame_rate(tystream)){
			fprintf(stderr, "Big ERROR - CAN'T DETECT FRAME RATE\n");
		clear_tystream_audio_probe(tystream, 0);
		return(0);
	}

	if(!probe_tystream_audio(tystream)) {
			fprintf(stderr, "Big ERROR - CAN'T DETECT AUDIO TYPE\n");
		clear_tystream_audio_probe(tystream, 0);
		return(0);
	}

	if(!probe_audio_tick_size(tystream)) {
			fprintf(stderr, "Big ERROR - CAN'T DETECT AUDIO SIZE\n");
		clear_tystream_audio_probe(tystream, 1);
		return(0);
	}

	clear_tystream_audio_probe(tystream, 1);
	return(1);

}



int std_probe_tystream(tystream_holder_t * tystream) {

	/* Iteration */
	off64_t j;

	/* File size */
	off64_t in_file_size;

	/* Chunks */
	chunknumber_t local_nr_chunks;
	chunknumber_t probe_hz;
	chunknumber_t chunk_lnr;
	chunknumber_t int_chunk_nr;


	chunk_t * chunk;

	LOG_USERDIAG("Probing TyStream .....\n");

	chunk = NULL;


	if(tystream->vstream) {
		in_file_size=tystream->vstream->size;
	} else {
		in_file_size=tystream->in_file_size;
	}

	/* Calculate the probe rate */
	local_nr_chunks = (chunknumber_t)(in_file_size/CHUNK_SIZE);
	probe_hz = local_nr_chunks/10;

	for(j=0, chunk_lnr=1,int_chunk_nr=0 ;  j < in_file_size; j = (j + CHUNK_SIZE*probe_hz), chunk_lnr = chunk_lnr + probe_hz, int_chunk_nr++ ){

		chunk = read_chunk(tystream, chunk_lnr, 2048);
		if(chunk != NULL) {
			tystream->chunks = add_chunk(tystream, chunk, tystream->chunks);
			//printf("Chunk %i has %i records\n",int_chunk_nr,chunk->nr_records);
		} else {
			//printf("Chunk was null\n");
		}
	}



	if(tystream->chunks == NULL) {
		LOG_ERROR("Big ERROR - NO CHUNKS\n");
		clear_tystream_audio_probe(tystream, 0);
		return(0);
	}

	if(!probe_tystream_S2(tystream)) {
		LOG_ERROR("Big ERROR - CAN'T DETECT S1/S2\n");
		clear_tystream_audio_probe(tystream, 0);
		return(0);
	}


	if(!probe_tystream_frame_rate(tystream)){
		LOG_ERROR("Big ERROR - CAN'T DETECT FRAME RATE\n");
		clear_tystream_audio_probe(tystream, 0);
		return(0);
	}

	if(!probe_tystream_audio(tystream)) {
		LOG_ERROR("Big ERROR - CAN'T DETECT AUDIO TYPE\n");
		clear_tystream_audio_probe(tystream, 0);
		return(0);
	}

	if(!probe_audio_tick_size(tystream)) {
		LOG_ERROR("Big ERROR - CAN'T DETECT AUDIO SIZE\n");
		clear_tystream_audio_probe(tystream, 1);
		return(0);
	}

	clear_tystream_audio_probe(tystream, 1);
	return(1);


}

void get_start_chunk(tystream_holder_t * tystream) {

	/* interation */
	chunknumber_t j;
	int i;


	/* Marker */
	int gotit;

	/* Chunks */
	int chunk_lnr;
	chunk_t * chunk;
#if !defined(TIVO)
	vstream_t * vstream;
	uint8_t * buffer;
#endif

	if(tystream->right_audio == 0x3c0) {
		LOG_USERDIAG("Seeking TyStream start of MPEG Layer II audio \n");
	} else {
		LOG_USERDIAG("Seeking TyStream start of AC3 (Dolby Digital) audio \n");
	}



	gotit=0;


	if(tystream->std_alone) {

		for(j=0, chunk_lnr=0;  j < tystream->in_file_size; j = (j + CHUNK_SIZE) + tystream->byte_offset, chunk_lnr++){


			chunk = read_chunk(tystream, chunk_lnr, 1);
			if(chunk != NULL) {
				for(i=0; i < chunk->nr_records; i++){
					if(chunk->record_header[i].type == tystream->right_audio){
						gotit=1;
						break;
					}
				}
				free_chunk(chunk);
			}
			if(gotit){
				break;
			}
		}
	}
#if !defined(TIVO)
	else {
		chunk_lnr=0;

		buffer = (uint8_t *)malloc(sizeof(uint8_t) * CHUNK_SIZE);
		while(1) {

			vstream = new_vstream();
			memset(buffer,0,sizeof(uint8_t) * CHUNK_SIZE);
			thread_pipe_peek(tystream->input_pipe, CHUNK_SIZE, buffer);
			vstream->size = CHUNK_SIZE;
			vstream->start_stream = vstream->current_pos = buffer;
			vstream->end_stream = vstream->start_stream + CHUNK_SIZE;
			tystream->vstream = vstream;

			chunk = read_chunk(tystream, chunk_lnr, 1);
			if(chunk != NULL) {
				for(i=0; i < chunk->nr_records; i++){
					if(chunk->record_header[i].type == tystream->right_audio){
						gotit=1;
						break;
					}
				}
				free_chunk(chunk);
			}

			vstream->start_stream = vstream->end_stream = vstream->current_pos = NULL;
			vstream->size = vstream->eof = 0;
			tystream->vstream=NULL;
			free(vstream);

			if(!gotit) {
				chunk_lnr++;
				thread_pipe_read(tystream->input_pipe, CHUNK_SIZE, buffer);
			} else {
				break;
			}
		}
		free(buffer);
	}
#endif

	tystream->start_chunk = chunk_lnr;

	if(tystream->right_audio == 0x3c0) {
		LOG_USERDIAG("Found start of MPEG Layer II audio \n");
	} else {
		LOG_USERDIAG("Found start of AC3 (Dolby Digital) audio \n");
	}
	LOG_USERDIAG1("Skipping to chunk " I64FORMAT " - reseting chunk numbering \n\n", tystream->start_chunk);

	tystream->number_of_chunks = 0;
}


/******************************************************************************/

static int probe_tystream_S2(tystream_holder_t * tystream) {

	/* Iteration */
	int i;

	/* Chunk */
	chunk_t * chunks;

	/* Counters */
	int video_pes;
	int video_b;

	/* Init */
	video_pes = 0;
	video_b = 0;
	chunks = tystream->chunks;


	while(chunks) {
		for(i=0; i < chunks->nr_records; i++){
			switch(chunks->record_header[i].type) {
				case 0x6e0: /* Tivo Series 1 - video pes header */
					video_pes++;
					LOG_DEVDIAG1("Probe S2: Got Video PES header - %i\n", video_pes);
					break;
				case 0xbe0: /* Normal B frame - present in both S1 and S2 */
					video_b++;
					LOG_DEVDIAG1("Probe: Got B-Frame - %i\n", video_b);
					break;
			}
		}
		chunks = chunks->next;
	}
	LOG_DEVDIAG1("Number of B frames %i \n",video_b);
	LOG_DEVDIAG1("Number of S1 PES %i \n",video_pes);

	if(video_b) {
		if(video_pes) {
			tystream->tivo_series = S1;
		} else {

			tystream->tivo_series = S2;
		}
		return(1);
	} else {
		/* Probe faild */
		return(0);
	}


}



/**********************************************************************/

static int probe_tystream_audio(tystream_holder_t * tystream) {

	/* Iteration */
	int i;

	/* Audio counters */
	int mpeg;
	int ac3;

	/* Marker */
	int gotit;

	/* Chunks */
	chunk_t * chunks;

	/* Audio buffers */
	uint8_t * data_buffer;

	/* Poiters to do magic with */
	uint8_t * pt1;

	/* Offset size for buffers */
	int offset;
	int size;

	/* Init */
	chunks = tystream->chunks;
	mpeg = 0;
	ac3 = 0;
	gotit = 0;
	size = 0;
	offset = 0;
	i=0;

	LOG_DEVDIAG("AUDIO PROBE\n");

	while(chunks) {
		for(i=0; i < chunks->nr_records; i++){
			switch(chunks->record_header[i].type) {
				case 0x3c0: /* Audio PES - MPEG */
					mpeg++;
					LOG_DEVDIAG1("Probe: Got mpeg - %i\n", mpeg);
					break;
				case 0x9c0:
					ac3++;
					LOG_DEVDIAG1("Probe: Got ac3 - %i\n", ac3);
					break;
			}
		}
		chunks = chunks->next;
	}

	if(ac3 >= mpeg) {
		LOG_DEVDIAG("Setting AC3\n");
		tystream->wrong_audio = 0x3c0;
		tystream->right_audio = 0x9c0;
		tystream->tivo_type = DTIVO;
		tystream->DTIVO_MPEG = 0;
		tystream->audio_startcode = AC3_PES_AUDIO;
		return(1);
	} else {
		LOG_DEVDIAG("Setting MPEG\n");
		tystream->wrong_audio = 0x9c0;
		tystream->right_audio = 0x3c0;

	/* Okay lets find out if we have a DTIVO MPEG or SA MPEG */

	/* Normal PES header should be like this (simplified):
	 * packet_start code 	24 bits
	 * stream_id	 	8 bits
	 * pes_packet_lengt	16 bits
	 * pes_header_data	24 bits
	 * PTS 			40 bits
	 *
	 * A DTIVO MPEG header looks like this:
	 *
	 * packet_start code 	24 bits
	 * stream_id	 	8 bits
	 * pes_packet_lengt	16 bits
	 * PTS			40 bits
	 *
	 * hence it's missing the whole pes header data :(.
	 *
	 * Luckely the first two bits in the pes header data
	 * starts with 10 while the two first bits in the PTS
	 * starts with 00 hence if we "and" byte 6 with 0x80 we
	 * should be able to detect if this is a DTIVO mpeg or
	 * a SA TIVO MPEG :)
	 */

		tystream->audio_startcode = MPEG_PES_AUDIO;
		chunks = tystream->chunks;

		/* So we have a 1.3 what a bitch :(
		Basically we can't check much in a 1.3 stream
		simply to much info lacking - the check has to be
		done much later which makes it more compicated
		Hence we just set some default values and return */


		while(chunks) {
			/* Must make sure we have at least to audio pes
			in the chunk otherwise it may fail */
			LOG_DEVDIAG1("Number of audio records %i\n", get_nr_audio_pes(tystream, chunks));
			if(get_nr_audio_pes(tystream, chunks) >= 2) {
				for(i=0; i < chunks->nr_records; i++){
					switch(chunks->record_header[i].type) {
						case 0x3c0: /* Audio PES - MPEG */
							mpeg++;
							LOG_DEVDIAG1("Probe: Got mpeg - %i\n", mpeg);
							gotit = 1;
							break;
					}

					if(gotit) {
						break;
					}
				}
			}

			if(gotit) {
				break;
			} else {
				chunks = chunks->next;
			}
		}

		if(!gotit) {
			return(0);
		}

		offset = find_start_code_offset(i, chunks, MPEG_PES_AUDIO, 0, tystream);
		size = chunks->record_header[i].size;
		if(size - offset < 7) {
			/* Need to fetch the next audio record */
			data_buffer = (uint8_t *)malloc((size - offset) * sizeof(uint8_t));
			memset(data_buffer, 0,(size - offset) * sizeof(uint8_t));
			pt1 = chunks->records[i].record + offset;
			memcpy(data_buffer, pt1,size - offset);
			data_buffer = get_next_record(i + 1, chunks, AUDIO, size - offset, data_buffer,0);
			/* Okay find out what we have */
			if((data_buffer[6] & 0x80) == 0x80) {
				/* We have a SA MPEG stream !! */
				tystream->tivo_type = SA;
				tystream->DTIVO_MPEG = 0;
			} else {
				tystream->tivo_type = DTIVO;
				tystream->DTIVO_MPEG = 3;
			}
			free(data_buffer);
		} else {
			if((chunks->records[i].record[6 + offset] & 0x80) == 0x80) {
				/* We have a SA MPEG stream !! */
				tystream->tivo_type = SA;
				tystream->DTIVO_MPEG = 0;
			} else {
				tystream->tivo_type = DTIVO;
				tystream->DTIVO_MPEG = 3;
			}
		}
		return(1);
	}
}

/**************************************************************************/

static int probe_audio_tick_size(tystream_holder_t * tystream) {

	/* Iteration */
	int i;

	/* Chunks */
	chunk_t * chunks;
	chunk_t * right_audio_chunk;

	/* Counting variables */
	int nr_size;
	int nr_time;
	int nr_audio_pes;

	/* Total timediff and size */
	ticks_t total_time;
	ticks_t time_diff;
	ticks_t audio_median_tick_diff;
	int total_size;
	int med_size;

	/* Markers */
	int gotit;


	/* Init */
	chunks = tystream->chunks;
	right_audio_chunk=NULL;
	nr_size = 0;
	nr_time = 0;
	nr_audio_pes = 0;
	total_time = 0;
	gotit = 0;
	total_size = 0;

	//printf("Audio Tick size\n");

	while(chunks) {
		gotit = 0;
		for(i=0; i < chunks->nr_records; i++){
			if(chunks->record_header[i].type == tystream->right_audio) {
				/* okay so lets save  it - in junk_chunks */
				//printf("Audio Tick size copy %i\n", i);
				right_audio_chunk = copy_chunk(chunks);
				//printf("Audio Tick size copy finished %i\n", i);
				if(right_audio_chunk) {
					tystream->junk_chunks = add_chunk(tystream, right_audio_chunk, tystream->junk_chunks);
				}
				gotit = 1;
			} else if (chunks->record_header[i].type == tystream->wrong_audio) {
				gotit = 1;
			}

			if(gotit) {
				break;
			}
		}
		chunks = chunks->next;
	}


	if( tystream->junk_chunks == NULL) {
		/* Abort */
		LOG_ERROR("ABORTING \n");
		return(0);
	}

	right_audio_chunk = tystream->junk_chunks;

	//printf("Audio Tick size - 1\n");

	/* Okay so get the total size - two stage to remove unwanted values FIXME*/
	while(right_audio_chunk) {
		for(i=0; i < right_audio_chunk->nr_records; i++) {
			switch(right_audio_chunk->record_header[i].type){
				case 0x2c0:
				case 0x4c0:
				case 0x3c0:
				case 0x9c0:
					if(right_audio_chunk->record_header[i].size > 300 &&  right_audio_chunk->record_header[i].size < 2000) {
						nr_size++;
						total_size = total_size + right_audio_chunk->record_header[i].size;
						LOG_DEVDIAG1("Size is %i\n",right_audio_chunk->record_header[i].size );
						LOG_DEVDIAG1("Total Size is %i\n", total_size);
						
					}
					break;
			}
		}
		right_audio_chunk = right_audio_chunk->next;
	}

	if(!nr_size) {
		LOG_ERROR("Major error div by zero\n");
		return(0);
	}

	med_size = total_size / nr_size;

	//printf("Audio Tick size - 2\n");

	nr_size = 0;
	total_size = 0;
	right_audio_chunk = tystream->junk_chunks;

	while(right_audio_chunk) {
		for(i=0; i < right_audio_chunk->nr_records; i++) {
			switch(right_audio_chunk->record_header[i].type){
				case 0x2c0:
				case 0x4c0:
				case 0x3c0:
				case 0x9c0:
					if((int)right_audio_chunk->record_header[i].size > (med_size - 200) &&
					   (int)right_audio_chunk->record_header[i].size < (med_size + 200)) {
						nr_size++;
						total_size = total_size + right_audio_chunk->record_header[i].size;
						LOG_DEVDIAG1("Size is %i\n",right_audio_chunk->record_header[i].size );
						LOG_DEVDIAG1("Total Size is %i\n", total_size);
					}
					break;
			}
		}
		right_audio_chunk = right_audio_chunk->next;
	}

	if(!nr_size) {
		LOG_ERROR("Major error div by zero\n");
	}

	//printf("Audio Tick size - 3 \n");

	med_size = total_size / nr_size;

	tystream->med_size = med_size;

	/* This is the typical sizes of TIVO mpeg/ac3 audio
	 * Hence we want to control if we have a typical one
	 * if we don't we give a waring and tell the user to
	 * save the tystream. FIXME WE SHOULD EXIT
	 */
	if(med_size > 1532 && med_size < 1572) {
		tystream->std_audio_size = 1552;
		tystream->audio_frame_size = 1536;
		tystream->audio_type = DTIVO_AC3;
	} else if ( med_size > 840 && med_size < 910) {
		tystream->std_audio_size = 880;
		tystream->audio_frame_size = 864;
		tystream->audio_type = SA_MPEG;
	} else if ( med_size > 760 && med_size < 800) {
		tystream->std_audio_size = 780;
		tystream->audio_frame_size = 768;
		tystream->audio_type = DTIVO_MPEG_1;
	} else if ( med_size > 568 && med_size < 608) {
		tystream->std_audio_size = 588;
		tystream->audio_frame_size = 576;
		tystream->audio_type = DTIVO_MPEG_2;
	} else if ( med_size > 460 && med_size < 500) {
		tystream->std_audio_size = 492;
		tystream->audio_frame_size = 480;
		tystream->audio_type = DTIVO_MPEG_3;
	} else if ( med_size > 328 && med_size < 368) {
		/* This one is a bit of guess work FIXME */
		tystream->std_audio_size = 348;
		tystream->audio_frame_size = 336;
		tystream->audio_type = DTIVO_MPEG_4;
		LOG_ERROR("probe_audio_tick_size: PLEASE SEND A REPORT - SAVE THE TYSTREAM\n");
	} else {
		tystream->std_audio_size = 0;
		tystream->audio_frame_size = 0;
		tystream->audio_type = AUDIO_TYPE_UNKNOW;
		LOG_WARNING("probe_audio_tick_size: Warning: can't determine std audio size PLEASE SEND A REPORT - SAVE THE TYSTREAM\n");
	}

	/* Now we collect the median tickdiff of the audio stream */

	right_audio_chunk = tystream->junk_chunks;

	while(right_audio_chunk) {
		nr_audio_pes = get_nr_audio_pes(tystream, right_audio_chunk);
		for(i=0; i < nr_audio_pes - 2; i++) {
			time_diff = get_time_X(i + 1, right_audio_chunk, AUDIO, tystream)
				- get_time_X(i, right_audio_chunk, AUDIO, tystream);
			if(time_diff > 1000 && time_diff < 4000) {
				total_time = total_time + time_diff;
				nr_time++;
			}
		}
		right_audio_chunk = right_audio_chunk->next;
	}


	//printf("Out of Audio Tick\n");
	audio_median_tick_diff = total_time / nr_time;
	
	if(audio_median_tick_diff < 3245 && audio_median_tick_diff > 3235) {
		tystream->audio_median_tick_diff = 3240;
	} else if (audio_median_tick_diff < 2165 && audio_median_tick_diff > 2155) {
		tystream->audio_median_tick_diff = 2160;
	} else if (audio_median_tick_diff < 2885 && audio_median_tick_diff > 2875) {
		tystream->audio_median_tick_diff = 2880;
	} else {
		LOG_WARNING1("probe_audio_tick_size: Warning: can't determine audio_median_tick_diff "I64FORMAT"PLEASE SEND A REPORT - SAVE THE TYSTREAM\n", audio_median_tick_diff);
		return(0);
	}

	return(1);

}



/*****************************************************************/


static uint8_t actual_frame_rate(ticks_t diff) {

	uint8_t frame_rate;

	if(diff < 7512 && diff > 7500) {
		LOG_DEVDIAG("Actual frame rate: 23.976 frames/sec\n");
		frame_rate = 1;
	} else if (diff < 7501 && diff > 7696) {
		LOG_DEVDIAG("Actual frame rate: 24 frames/sec\n");
		frame_rate = 2;
	} else if (diff < 7210 && diff > 7190) {
		LOG_DEVDIAG("Actual frame rate: 25 frames/sec\n");
		frame_rate = 3;
	} else if (diff < 6012 && diff > 6000) {
		LOG_DEVDIAG("Actual frame rate: 29.97 frames/sec\n");
		frame_rate = 4;
	} else if (diff < 6001 && diff > 5996) {
		LOG_DEVDIAG("Actual frame rate: 30 frames/sec\n");
		frame_rate = 5;
	} else if (diff < 3610 && diff > 3590) {
		LOG_DEVDIAG("Actual frame rate: 50 frames/sec\n");
		frame_rate = 6;
	} else if (diff < 3006 && diff > 3000) {
		LOG_DEVDIAG("Actual frame rate: 59.94 frames/sec\n");
		frame_rate = 7;
	} else if (diff < 3001 && diff > 2996) {
		LOG_DEVDIAG("Actual frame rate: 60 frames/sec\n");
		frame_rate = 8;
	} else {
		LOG_DEVDIAG("Actual frame rate unknown!!\n");
		frame_rate = 0;
	}

	return(frame_rate);

}


/*************************************************************************************/

static int probe_tystream_frame_rate(tystream_holder_t * tystream) {

	/* Iteration */
	int i;

	/* Chunk */
	chunk_t * chunks;

	/* Frame rate */
	uint8_t tmp_frame_rate;

	/* Times */
	ticks_t time_pre_b_frame;
	ticks_t time_post_b_frame;
	/* Counters */
	int f_23, f_24, f_25, f_29, f_30, f_50, f_59, f_60, unknown;

	chunks = tystream->chunks;

	f_23 = 0;
	f_24 = 0;
	f_25 = 0;
	f_29 = 0;
	f_30 = 0;
	f_50 = 0;
	f_59 = 0;
	f_60 = 0;
	unknown = 0;

	tmp_frame_rate = 0;


	while(chunks) {
		for(i=0; i < chunks->nr_records; i++){
			switch(chunks->record_header[i].type) {
				case 0xae0: /* P Frame */
					time_pre_b_frame = get_time_pre_b_frame(tystream, chunks, i);
					time_post_b_frame = get_time_post_b_frame(tystream, chunks, i);
					LOG_DEVDIAG2(I64FORMAT " : " I64FORMAT " \n",time_pre_b_frame,time_post_b_frame);
					if(time_pre_b_frame && time_post_b_frame) {
						tmp_frame_rate = actual_frame_rate(time_post_b_frame - time_pre_b_frame);
					} else {
						tmp_frame_rate = 0;
					}
					switch(tmp_frame_rate) {
						case(1):
							f_23++;
							break;
						case(2):
							f_24++;
							break;
						case(3):
							f_25++;
							break;
						case(4):
							f_29++;
							break;
						case(5):
							f_30++;
							break;
						case(6):
							f_50++;
							break;
						case(7):
							f_59++;
							break;
						case(8):
							f_60++;
							break;
						default:
							unknown++;
							break;
					}
			}
		}
		chunks = chunks->next;
	}

	if(ty_debug >= 7 ) {
		printf("unknown %i f_23 %i f_24 %i f_25 %i f_29 %i f_30 %i f_50 %i f_59 %i f_60 %i\n",
			unknown, f_23, f_24, f_25, f_29, f_30, f_50, f_59, f_60);
	}

	if(unknown && !f_23 && !f_24 && !f_25 && !f_29 && !f_30 && !f_50 && !f_59 && !f_60) {
		return(0);
	}


	if(f_23 >= f_24 && f_23 >= f_25 && f_23 >= f_29 && f_23 >= f_30 && f_23 >= f_50 && f_23 >= f_59 && f_23 >= f_60) {
		/* Frame rate is 23.976 f/s */
		/*tystream->frame_rate = 1;
		tystream->present_frame_rate = 1;
		tystream->tick_diff = 7506;
		tystream->frame_tick = 3753;
		tystream->drift_threshold = (tystream->frame_tick/2) + (tystream->frame_tick/20);*/
		tystream->frame_rate = 4;
		tystream->tick_diff = 6006;
		tystream->frame_tick = 3003;
		tystream->drift_threshold = (tystream->frame_tick/2) + (tystream->frame_tick/20);
		return(1);

	} else if(f_24 >= f_23 && f_24 >= f_25 && f_24 >= f_29 && f_24 >= f_30 && f_24 >= f_50 && f_24 >= f_59 && f_24 >= f_60) {
		/* Frame rate is 24 f/s */
		tystream->frame_rate = 2;
		tystream->tick_diff = 7500;
		tystream->frame_tick = 3750;
		tystream->drift_threshold = (tystream->frame_tick/2) + (tystream->frame_tick/20);

		return(1);

	} else if(f_25 >= f_23 && f_25 >= f_24 && f_25 >= f_29 && f_25 >= f_30 && f_25 >= f_50 && f_25 >= f_59 && f_25 >= f_60) {
		/* Frame rate is 25 f/s */
		tystream->frame_rate = 3;
		tystream->tick_diff = 7200;
		tystream->frame_tick = 3600;
		tystream->drift_threshold = (tystream->frame_tick/2) + (tystream->frame_tick/20);
		return(1);

	} else if(f_29 >= f_23 && f_29 >= f_24 && f_29 >= f_25 && f_29 >= f_30 && f_29 >= f_50 && f_29 >= f_59 && f_29 >= f_60) {
		/* Frame rate is 29.97 f/s */
		tystream->frame_rate = 4;
		tystream->tick_diff = 6006;
		tystream->frame_tick = 3003;
		tystream->drift_threshold = (tystream->frame_tick/2) + (tystream->frame_tick/20);
		return(1);

	} else if(f_30 >= f_23 && f_30 >= f_24 && f_30 >= f_25 && f_30 >= f_29 && f_30 >= f_50 && f_30 >= f_59 && f_30 >= f_60) {
		/* Frame rate is 30 f/s */
		tystream->frame_rate = 5;
		tystream->tick_diff = 6000;
		tystream->frame_tick = 3000;
		tystream->drift_threshold = (tystream->frame_tick/2) + (tystream->frame_tick/20);
		return(1);

	} else if(f_50 >= f_23 && f_50 >= f_24 && f_50 >= f_25 && f_50 >= f_29 && f_50 >= f_30 && f_50 >= f_59 && f_50 >= f_60) {
		/* Frame rate is 50 f/s */
		tystream->frame_rate = 6;
		tystream->tick_diff = 3600;
		tystream->frame_tick = 1800;
		tystream->drift_threshold = (tystream->frame_tick/2) + (tystream->frame_tick/20);
		return(1);

	} else if(f_59 >= f_23 && f_59 >= f_24 && f_59 >= f_25 && f_59 >= f_29 && f_59 >= f_30 && f_59 >= f_50 && f_59 >= f_60) {
		/* Frame rate is 59.94 f/s */
		tystream->frame_rate = 7;
		tystream->tick_diff = 3002;
		tystream->frame_tick = 1501;
		tystream->drift_threshold = (tystream->frame_tick/2) + (tystream->frame_tick/20);
		return(1);

	} else if(f_60 >= f_23 && f_60 >= f_24 && f_60 >= f_25 && f_60 >= f_29 && f_60 >= f_30 && f_60 >= f_50 && f_60 >= f_59) {
		/* Frame rate is 60 f/s */
		tystream->frame_rate = 8;
		tystream->tick_diff = 3000;
		tystream->frame_tick = 1500;
		tystream->drift_threshold = (tystream->frame_tick/2) + (tystream->frame_tick/20);
		return(1);

	} else {
		/* We are %W#% return 0 */
		return(0);
	}
}

/***************************************************************************************/



static ticks_t get_time_pre_b_frame(tystream_holder_t * tystream, chunk_t * chunk, int i) {

	int gotit;
	int j;

	uint8_t * pt1;

	gotit = 0;
	for(j=i; j > -1; j--) {
		switch(chunk->record_header[j].type) {
			case 0xbe0:
				if(tystream->tivo_series == S2) {
					pt1 = chunk->records[j].record ;
					pt1 = pt1 + PTS_TIME_OFFSET;
					return(get_time(pt1));
				}
				gotit=1;
				break;
		}
		if(gotit) {
			break;
		}
	}

	if(!gotit) {
		return(0);
	}

	if(j > 0 && chunk->record_header[j - 1].type == 0x6e0) {
		pt1 = chunk->records[j - 1].record;
		pt1 = pt1 + PTS_TIME_OFFSET;
		return(get_time(pt1));
	} else {
		return(0);
	}
}

/***************************************************************************************/

static ticks_t get_time_post_b_frame(tystream_holder_t * tystream, chunk_t * chunk, int i) {

	int gotit;
	int j;

	uint8_t * pt1;

	gotit = 0;
	for(j=i; j < chunk->nr_records; j++) {
		switch(chunk->record_header[j].type) {
			case 0xbe0:
				if(tystream->tivo_series == S2) {
					pt1 = chunk->records[j].record ;
					pt1 = pt1 + PTS_TIME_OFFSET;
					return(get_time(pt1));
				}
				gotit=1;
				break;
		}
		if(gotit) {
			break;
		}
	}

	if(!gotit) {
		return(0);
	}

	if(j < chunk->nr_records && chunk->record_header[j - 1].type == 0x6e0) {
		pt1 = chunk->records[j - 1].record;
		pt1 = pt1 + PTS_TIME_OFFSET;
		return(get_time(pt1));
	} else {
		return(0);
	}
}

/******************************************************************************************/

/* NOT A REAL COPY FAST and DIRTY */
static chunk_t * copy_chunk(chunk_t * chunk) {

	/* Iteration */
	int i;

	/* Chunk to return */
	chunk_t * return_chunk;

	return_chunk = (chunk_t *)malloc(sizeof(chunk_t));
	memset(return_chunk, 0,sizeof(chunk_t));
	return_chunk->nr_records = chunk->nr_records;
	return_chunk->seq_start = chunk->nr_records;
	return_chunk->next = NULL;
	return_chunk->previous = NULL;
	return_chunk->junk=0;

	return_chunk->record_header = (record_header_t *)malloc(sizeof(record_header_t) * chunk->nr_records);
	memset(return_chunk->record_header, 0,sizeof(record_header_t) * chunk->nr_records);
	return_chunk->records = (record_t *)malloc(sizeof(record_t) * chunk->nr_records);
	memset(return_chunk->records, 0,sizeof(record_t) * chunk->nr_records);

	for(i=0; i < chunk->nr_records; i++){
		return_chunk->record_header[i].size = chunk->record_header[i].size;
		return_chunk->record_header[i].type = chunk->record_header[i].type;
		return_chunk->record_header[i].extended_data = NULL;
		if(chunk->record_header[i].size) {
			return_chunk->records[i].record = (uint8_t *)malloc(sizeof(uint8_t) * chunk->record_header[i].size);
			memset(return_chunk->records[i].record, 0,sizeof(uint8_t) * chunk->record_header[i].size);
			memcpy(return_chunk->records[i].record, chunk->records[i].record,sizeof(uint8_t) * chunk->record_header[i].size);
		} else {
			return_chunk->records[i].record = NULL;
		}
	}

	return(return_chunk);

}

/******************************************************************************************/

static void clear_tystream_audio_probe(tystream_holder_t * tystream, int ready) {

	tystream->probed = ready;
	if(tystream->chunks) {
		free_junk_chunks(tystream->chunks);
		tystream->chunks = NULL;
	}
	if(tystream->junk_chunks != NULL) {
		free_junk_chunks(tystream->junk_chunks);
		tystream->junk_chunks=NULL;
	}
	tystream->chunks = NULL;

}
