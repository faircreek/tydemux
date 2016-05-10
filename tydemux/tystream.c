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


tystream_holder_t * new_tystream(operation_mode_t mode){

	/* Iteration */
	int i;

	/* The tystream */
	tystream_holder_t * tystream;

	tystream = (tystream_holder_t *)malloc(sizeof(tystream_holder_t));
	memset(tystream, 0,sizeof(tystream_holder_t));
	/* Init the tystream */

	tystream->chunks  				= NULL;
	tystream->junk_chunks 				= NULL;

	tystream->pes_holder_video			= NULL;
	tystream->lf_seq_pes_holder			= NULL;
	tystream->lf_tmp_ref_pes_holder			= NULL;
	tystream->lf_p_frame_pes_holder			= NULL;
	tystream->lf_field_pes_holder			= NULL;
	tystream->lf_av_drift_pes_holder_video		= NULL;
	tystream->lf_gop_frame_counter_pes_holder_video = NULL;

	tystream->pes_holder_audio			= NULL;

	tystream->tivo_type			= UNDEFINED_TYPE;
	tystream->tivo_version			= V_2X;
	tystream->tivo_series			= UNDEFINED_SERIES;

	tystream->wrong_audio			= 0;
	tystream->right_audio			= 0;
	tystream->med_size			= 0;
	tystream->audio_frame_size		= 0;
	tystream->std_audio_size		= 0;
	tystream->audio_median_tick_diff 	= 0;
	tystream->audio_type			= AUDIO_TYPE_UNKNOW;
	tystream->audio_startcode		= UNDEFINED_CODE;
	tystream->DTIVO_MPEG			= 0;

	/* Setting inital med ticks rather high to avoid
	hole detection in chunk 1 - Hmm back again to
	org 4505 instead of 7505*/
	tystream->med_ticks			= 4505;
	tystream->last_tick			= 0;

	tystream->cons_junk_chunk		= 0;
	tystream->probed			= 0;
	tystream->number_of_chunks		= 0;

	tystream->audio_out_file 		= -1;
	tystream->video_out_file 		= -1;
	tystream->out_file			= -1;
	tystream->in_file			= -1;
	tystream->in_file_size			= 0;
	
	tystream->infile			= NULL;
	tystream->audio_out			= NULL;
	tystream->video_out			= NULL;

	tystream->mode				= mode;

	tystream->start_chunk			= 0;
	tystream->byte_offset			= 0;
	tystream->repair			= 0;

	tystream->find_seq			= 0;
	tystream->time_diff			= 0;
	tystream->indicator			= POSITIVE;


	tystream->frame_rate			= 0;

	tystream->reminder			= 0;
	tystream->drift				= 0;
	tystream->tick_diff			= 0;
	tystream->frame_tick			= 0;
	tystream->drift_threshold		= 0;

	tystream->first_gop			= 1;

	tystream->write				= 1;
	tystream->seq_counter			= 0;

	/* Next take on drift and frame rate */
	tystream->last_field			= UNKNOW_FIELD;
	tystream->reminder_field		= 1501;


	/* Byte offset for miss aligned chunks */
	tystream->byte_offset			= 0;
	tystream->miss_alinged			= 0;

	/* Cuts */
	tystream->cuts				= NULL;
	tystream->index				= NULL;

	tystream->audiocut			= 0;
	tystream->audio_start_drift		= 0;

	tystream->vstream			= NULL;
	tystream->tivo_probe			= 0;
#if !defined(TIVO)
	tystream->input_pipe			= 0;
	tystream->audio_pipe			= 0;
	tystream->video_pipe			= 0;
#endif
	tystream->std_alone			= 0;
	tystream->multiplex			= 0;
	
	/* Remote management */
	tystream->nr_of_remote_chunks		= 0;
	tystream->remote_holder			= NULL;
	tystream->fsid				= 0;

	/* gop and frames */
	tystream->frame_counter 		= 0;
	tystream->frame_field_reminder		= 0;

#ifdef FRAME_RATE

	tystream->time_init			= 0;
	tystream->time_init_second		= 0;
	tystream->time_init_third		= 0;
	/*tystream->first_time			= 0;
	tystream->second_time			= 0;
	tystream->first_time_check		= 0;
	tystream->second_time_check		= 0;
	*/

	tystream->frame_counter			= 0;
	tystream->frame_removed			= 3;
	tystream->frame_added			= 2;

	tystream->last_time			= 0;
#endif




	/* The start code array */

	/**
	 * Video startcode/stream ID - note this one
	 * can very well change but it looks like TIVO
	 * only uses one type

	 * VIDEO PES header		== 0x00 0x00 0x01 0xe0;
	 * SEQ header			== 0x00 0x00 0x01 0xb3;
	 * GOP header			== 0x00 0x00 0x01 0xb8;
	 * I,P,B-Frame header		== 0x00 0x00 0x01 0x00;

	 * AUDIO MPEG PES header	== 0x00 0x00 0x01 0xc0;
	 * AUDIO AC3 PES header		== 0x00 0x00 0x01 0xbd;
	 */


	tystream->start_code_array = (start_code_array_t *)malloc(sizeof(start_code_array_t) * START_CODE_ARRAY);
	memset(tystream->start_code_array, 0,sizeof(start_code_array_t) * START_CODE_ARRAY);

	for(i=0; i < START_CODE_ARRAY; i++) {
		tystream->start_code_array[i].size = 0;
		tystream->start_code_array[i].start_code = NULL;
	}

	tystream->start_code_array[MPEG_PES_VIDEO].start_code = (uint8_t *)malloc(sizeof(uint8_t) * START_CODE_SIZE);
	tystream->start_code_array[MPEG_PES_VIDEO].size = START_CODE_SIZE;
	tystream->start_code_array[MPEG_PES_VIDEO].start_code[0] = 0x00;
	tystream->start_code_array[MPEG_PES_VIDEO].start_code[1] = 0x00;
	tystream->start_code_array[MPEG_PES_VIDEO].start_code[2] = 0x01;
	tystream->start_code_array[MPEG_PES_VIDEO].start_code[3] = 0xe0;
	//tystream->start_code_array[MPEG_PES_VIDEO].start_code = {0x00,0x00, 0x01, 0xe0};

	tystream->start_code_array[MPEG_SEQ].start_code = (uint8_t *)malloc(sizeof(uint8_t) * START_CODE_SIZE);
	tystream->start_code_array[MPEG_SEQ].size = START_CODE_SIZE;
	tystream->start_code_array[MPEG_SEQ].start_code[0] = 0x00;
	tystream->start_code_array[MPEG_SEQ].start_code[1] = 0x00;
	tystream->start_code_array[MPEG_SEQ].start_code[2] = 0x01;
	tystream->start_code_array[MPEG_SEQ].start_code[3] = 0xb3;
	//tystream->start_code_array[MPEG_SEQ].start_code = {0x00,0x00, 0x01, 0xb3};

	tystream->start_code_array[MPEG_GOP].start_code = (uint8_t *)malloc(sizeof(uint8_t) * START_CODE_SIZE);
	tystream->start_code_array[MPEG_GOP].size = START_CODE_SIZE;
	tystream->start_code_array[MPEG_GOP].start_code[0] = 0x00;
	tystream->start_code_array[MPEG_GOP].start_code[1] = 0x00;
	tystream->start_code_array[MPEG_GOP].start_code[2] = 0x01;
	tystream->start_code_array[MPEG_GOP].start_code[3] = 0xb8;
	//tystream->start_code_array[MPEG_PES_GOP].start_code = {0x00,0x00, 0x01, 0xb8};

	tystream->start_code_array[MPEG_I].start_code = (uint8_t *)malloc(sizeof(uint8_t) * START_CODE_SIZE);
	tystream->start_code_array[MPEG_I].size = START_CODE_SIZE;
	tystream->start_code_array[MPEG_I].start_code[0] = 0x00;
	tystream->start_code_array[MPEG_I].start_code[1] = 0x00;
	tystream->start_code_array[MPEG_I].start_code[2] = 0x01;
	tystream->start_code_array[MPEG_I].start_code[3] = 0x00;
	//tystream->start_code_array[MPEG_I].start_code = {0x00,0x00, 0x01, 0x00};

	tystream->start_code_array[MPEG_P].start_code = (uint8_t *)malloc(sizeof(uint8_t) * START_CODE_SIZE);
	tystream->start_code_array[MPEG_P].size = START_CODE_SIZE;
	tystream->start_code_array[MPEG_P].start_code[0] = 0x00;
	tystream->start_code_array[MPEG_P].start_code[1] = 0x00;
	tystream->start_code_array[MPEG_P].start_code[2] = 0x01;
	tystream->start_code_array[MPEG_P].start_code[3] = 0x00;
	//tystream->start_code_array[MPEG_P].start_code = {0x00,0x00, 0x01, 0x00};

	tystream->start_code_array[MPEG_B].start_code = (uint8_t *)malloc(sizeof(uint8_t) * START_CODE_SIZE);
	tystream->start_code_array[MPEG_B].size = START_CODE_SIZE;
	tystream->start_code_array[MPEG_B].start_code[0] = 0x00;
	tystream->start_code_array[MPEG_B].start_code[1] = 0x00;
	tystream->start_code_array[MPEG_B].start_code[2] = 0x01;
	tystream->start_code_array[MPEG_B].start_code[3] = 0x00;
	//tystream->start_code_array[MPEG_B].start_code = {0x00,0x00, 0x01, 0x00};

	tystream->start_code_array[MPEG_PES_AUDIO].start_code = (uint8_t *)malloc(sizeof(uint8_t) * START_CODE_SIZE);
	tystream->start_code_array[MPEG_PES_AUDIO].size = START_CODE_SIZE;
	tystream->start_code_array[MPEG_PES_AUDIO].start_code[0] = 0x00;
	tystream->start_code_array[MPEG_PES_AUDIO].start_code[1] = 0x00;
	tystream->start_code_array[MPEG_PES_AUDIO].start_code[2] = 0x01;
	tystream->start_code_array[MPEG_PES_AUDIO].start_code[3] = 0xc0;
	//tystream->start_code_array[MPEG_PES_AUDIO].start_code = {0x00,0x00, 0x01, 0xc0};

	tystream->start_code_array[AC3_PES_AUDIO].start_code = (uint8_t *)malloc(sizeof(uint8_t) * START_CODE_SIZE);
	tystream->start_code_array[AC3_PES_AUDIO].size = START_CODE_SIZE;
	tystream->start_code_array[AC3_PES_AUDIO].start_code[0] = 0x00;
	tystream->start_code_array[AC3_PES_AUDIO].start_code[1] = 0x00;
	tystream->start_code_array[AC3_PES_AUDIO].start_code[2] = 0x01;
	tystream->start_code_array[AC3_PES_AUDIO].start_code[3] = 0xbd;
	//tystream->start_code_array[AC3_PES_AUDIO].start_code = {0x00,0x00, 0x01, 0xbd};

	tystream->start_code_array[MPEG_EXT].start_code = (uint8_t *)malloc(sizeof(uint8_t) * START_CODE_SIZE);
	tystream->start_code_array[MPEG_EXT].size = START_CODE_SIZE;
	tystream->start_code_array[MPEG_EXT].start_code[0] = 0x00;
	tystream->start_code_array[MPEG_EXT].start_code[1] = 0x00;
	tystream->start_code_array[MPEG_EXT].start_code[2] = 0x01;
	tystream->start_code_array[MPEG_EXT].start_code[3] = 0xb5;
	//tystream->start_code_array[MPEG_EXT].start_code = {0x00,0x00, 0x01, 0xb5};



	return(tystream);

}

void free_tystream(tystream_holder_t * tystream) {

	/* Iteration */
	int i;

	//printf("Free index\n");
	if(tystream->index) {
		free_index(tystream->index);
	}
	
	//printf("Free chunks\n");
	if(tystream->chunks != NULL) {
		free_junk_chunks(tystream->chunks);
	}

	//printf("Free junk\n");
	if(tystream->junk_chunks != NULL) {
		free_junk_chunks(tystream->junk_chunks);
	}

	/* FIXME */
	//printf("Free prevideo\n");
	if(tystream->pes_holder_video && tystream->pes_holder_video->previous) {
		free_pes_holder(tystream->pes_holder_video->previous, 1);
	}

	//printf("Free vido\n");
	if(tystream->pes_holder_video != NULL) {
		free_all_pes_holders(tystream->pes_holder_video);
	}

	//printf("Free audio\n");
	if(tystream->pes_holder_audio != NULL) {
		free_all_pes_holders(tystream->pes_holder_audio);
	}

	//printf("Free start\n");
	for(i=0; i < START_CODE_ARRAY; i++) {
		tystream->start_code_array[i].size = 0;
		if(tystream->start_code_array[i].start_code != NULL) {
			free(tystream->start_code_array[i].start_code);
		}
	}



	free(tystream->start_code_array);
	free(tystream);
	//printf("Free finished\n");

}

int tystream_set_out_files(tystream_holder_t * tystream, int audio_file, int video_file) {

	if(tystream->mode == DEMUX) {
		if(audio_file == -1 || video_file == -1){
			return(0);
		} else {
			tystream->audio_out_file = audio_file;
			tystream->video_out_file = video_file;
			return(1);
		}
	} else {
		return(0);
	}
}

int tystream_set_in_file(tystream_holder_t * tystream, int in_file, off64_t in_file_size) {

	if(in_file == -1) {
		return(0);
	} else {
		tystream->in_file = in_file;
		tystream->in_file_size = in_file_size;
		return(1);
	}
}

int tystream_set_out_file(tystream_holder_t * tystream, int out_file) {

	if(tystream->mode == REMUX && out_file != -1) {
		tystream->out_file = out_file;
		return(1);
	} else {
		return(0);
	}
}


