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

/* Internal */

static int get_time_field_info_std_alone(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder,
		int tmp_ref, ticks_t *  time, field_type_t * last_field,
		field_type_t * first_field, int * nr_of_fields);


ticks_t get_time_Y_video(const int record_nr, const chunk_t * chunk, tystream_holder_t * tystream) {


	/* Offset */
	int offset;

	/* Time */
	ticks_t time;

	/* Pointers to do magic with */
	uint8_t * pt1;

	uint8_t * tmp_buffer_time=NULL;


	if(record_nr > chunk->nr_records) {
		/* record not present in chunk */
		return(0);
	}




	if(chunk->record_header[record_nr].type != 0x6e0) {
		/* It's not a pes header return!! */
		return(0);
	}


	/* Now get the offset to the start of the pes record
	We need it to be able to extract the PTS time */
	offset = find_start_code_offset(record_nr, chunk, MPEG_PES_VIDEO, 0, tystream);

	if(offset == -1 || offset > (int)chunk->record_header[record_nr].size) {
		/* Error can find start of PES or offset is bigger than the size of the record */
		return(0);
	}

	/* If the size of the record is bigger than PES_MIN_PTS_SIZE - tystream->DTIVO_MPEG + offset
	then we only need this record to find the PTS time */

	if((int)chunk->record_header[record_nr].size > (PES_MIN_PTS_SIZE + offset)) {
		pt1 = chunk->records[record_nr].record;
		pt1 = pt1 + offset + PTS_TIME_OFFSET;
		return(get_time(pt1));
	} else {
		/* We need the next video record to find the time */
		tmp_buffer_time = (uint8_t *)malloc(sizeof(uint8_t) * chunk->record_header[record_nr].size);
		memset(tmp_buffer_time, 0,sizeof(uint8_t) * chunk->record_header[record_nr].size);

		memcpy(tmp_buffer_time, chunk->records[record_nr].record,
			sizeof(uint8_t) * chunk->record_header[record_nr].size);

		tmp_buffer_time = get_next_record(record_nr+1, chunk, VIDEO,
					chunk->record_header[record_nr].size, tmp_buffer_time, 40);
		if(!tmp_buffer_time) {
			LOG_WARNING2("Get Time X: Error no next record - pes %i chunk " I64FORMAT "\n", \
					record_nr, chunk->chunk_number);
			return(0);
		}

		pt1 = tmp_buffer_time;
		pt1 = pt1 + offset + PTS_TIME_OFFSET;
		time = get_time(pt1);
		free(tmp_buffer_time);
		return(time);
	}

}



ticks_t get_time_X(const int pes_number, const chunk_t * chunk, const media_type_t media,
	tystream_holder_t * tystream) {

	/* Iteration */
	int i;

	/* Number of pes records */
	int nr_pes_records;
	int comp_nr_pes_records;

	/* Offset */
	int offset;

	/* Time */
	ticks_t time;

	/* Pointers to do magic with */
	uint8_t * pt1;

	uint8_t * tmp_buffer_time=NULL;



	comp_nr_pes_records = 0;

	if(media == AUDIO) {

		nr_pes_records = get_nr_audio_pes(tystream, chunk);

	} else if (media == VIDEO) {

		nr_pes_records = get_nr_video_pes(tystream, chunk);

	} else {
		/* Error */
		LOG_ERROR("Get Time X: Wrong media type\n");
		return(0);
	}


	if(nr_pes_records < pes_number + 1) {
		/* We requsted a  pes not present in the chunk */
		LOG_ERROR3("Get Time X: Error OOR pes %i - %d - chunk " I64FORMAT "\n", \
			pes_number, nr_pes_records - 1, chunk->chunk_number);
		return(0);
	}

	if(media == AUDIO) {
		for(i=0 ; i < chunk->nr_records && comp_nr_pes_records < pes_number + 1; i++) {
			switch(chunk->record_header[i].type){
				case 0x3c0:
				case 0x9c0:
					comp_nr_pes_records++;
			}
		}
		/* reset i */
		i--;

		/* We know the record number of the pes we want */

		/* Now get the offset to the start of the pes record
		We need it to be able to extract the PTS time */
		offset = find_start_code_offset(i, chunk, tystream->audio_startcode, 0, tystream);

		if(offset == -1) {
			/* Error can find start of PES */
			return(0);
		}

		/* If the size of the record is bigger than PES_MIN_PTS_SIZE - tystream->DTIVO_MPEG + offset
		then we only need this record to find the PTS time */

		if((int)chunk->record_header[i].size > (PES_MIN_PTS_SIZE - tystream->DTIVO_MPEG + offset)) {
			pt1 = chunk->records[i].record;
			pt1 = pt1 + offset + PTS_TIME_OFFSET - tystream->DTIVO_MPEG;
			return(get_time(pt1));
		} else {
			/* We need the next audio record to find the time */
			tmp_buffer_time = (uint8_t *)malloc(sizeof(uint8_t) * chunk->record_header[i].size);
			memset(tmp_buffer_time, 0,sizeof(uint8_t) * chunk->record_header[i].size);
			memcpy(tmp_buffer_time, chunk->records[i].record,
				sizeof(uint8_t) * chunk->record_header[i].size);

			tmp_buffer_time = get_next_record(i+1, chunk, media,
						chunk->record_header[i].size, tmp_buffer_time, 40);
			if(!tmp_buffer_time) {
				LOG_ERROR2("Get Time X: Error no next record - pes %i chunk " I64FORMAT "\n", \
					pes_number, chunk->chunk_number);
				return(0);
			}

			pt1 = tmp_buffer_time;
			pt1 = pt1 + offset + PTS_TIME_OFFSET - tystream->DTIVO_MPEG;
			time = get_time(pt1);
			free(tmp_buffer_time);
			return(time);
		}
	} else {
		for(i=0 ; i < chunk->nr_records && comp_nr_pes_records < pes_number + 1; i++) {
			switch(chunk->record_header[i].type){
				case 0x6e0:
					comp_nr_pes_records++;
			}
		}
		/* reset i */
		i--;

		/* We know the record number of the pes we want */

		/* Now get the offset to the start of the pes record
		We need it to be able to extract the PTS time */
		offset = find_start_code_offset(i, chunk, MPEG_PES_VIDEO, 0, tystream);

		if(offset == -1) {
			/* Error can find start of PES */
			return(0);
		}

		/* If the size of the record is bigger than PES_MIN_PTS_SIZE - tystream->DTIVO_MPEG + offset
		then we only need this record to find the PTS time */

		if((int)chunk->record_header[i].size > (PES_MIN_PTS_SIZE + offset)) {
			pt1 = chunk->records[i].record;
			pt1 = pt1 + offset + PTS_TIME_OFFSET;
			return(get_time(pt1));
		} else {
			/* We need the next audio record to find the time */
			tmp_buffer_time = (uint8_t *)malloc(sizeof(uint8_t) * chunk->record_header[i].size);
			memset(tmp_buffer_time, 0,sizeof(uint8_t) * chunk->record_header[i].size);
			memcpy(tmp_buffer_time, chunk->records[i].record,
				sizeof(uint8_t) * chunk->record_header[i].size);

			tmp_buffer_time = get_next_record(i+1, chunk, media,
						chunk->record_header[i].size, tmp_buffer_time, 40);
			if(!tmp_buffer_time) {
				LOG_ERROR2("Get Time X: Error no next record - pes %i chunk " I64FORMAT "\n", \
					pes_number, chunk->chunk_number);
				return(0);
			}

			pt1 = tmp_buffer_time;
			pt1 = pt1 + offset + PTS_TIME_OFFSET;
			time = get_time(pt1);
			free(tmp_buffer_time);
			return(time);
		}
	}
}

ticks_t get_time(uint8_t * pt){

	/* Note we assume that all PES record in Tivo has a PTS */

	ticks_t time;


	time = (((pt[0] & 0x0E) << 29) | (pt[1] << 22) | ((pt[2] & 0xFE) << 14) | (pt[3] << 7) | (pt[4] >> 1));
	//LOG_DEVDIAG1("Time: " I64FORMAT "\n", time);
	return((ticks_t)time);

}



int get_time_field_info(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, int tmp_ref, ticks_t *  time, field_type_t * last_field,
		field_type_t * first_field, int * nr_of_fields, tmp_ref_frame_t * frame_array) {


	uint16_t highest_tmp_ref;


	highest_tmp_ref = get_highest_tmp_ref(seq_pes_holder);



	if(tmp_ref < 0 || tmp_ref > highest_tmp_ref || frame_array == NULL) {

		return(get_time_field_info_std_alone(tystream, seq_pes_holder, tmp_ref, time, last_field,
			first_field, nr_of_fields));

	} else {
		*time = get_time_of_tmp_ref(seq_pes_holder, tmp_ref);

		if(frame_array[tmp_ref].picture_info->top_field_first &&
			frame_array[tmp_ref].picture_info->repeat_first_field) {

				*first_field = TOP_FIELD;
				*last_field = TOP_FIELD;
				*nr_of_fields = 3;

		} else if (!frame_array[tmp_ref].picture_info->top_field_first &&
			frame_array[tmp_ref].picture_info->repeat_first_field) {

				*first_field = BOTTOM_FIELD;
				*last_field = BOTTOM_FIELD;
				*nr_of_fields = 3;

		} else if (frame_array[tmp_ref].picture_info->top_field_first &&
			!frame_array[tmp_ref].picture_info->repeat_first_field) {

				*first_field = TOP_FIELD;
				*last_field = BOTTOM_FIELD;
				*nr_of_fields = 2;

		} else if (!frame_array[tmp_ref].picture_info->top_field_first &&
			!frame_array[tmp_ref].picture_info->repeat_first_field) {

				*first_field = BOTTOM_FIELD;
				*last_field = TOP_FIELD;
				*nr_of_fields = 2;
		}
	}

	return(1);

}




static int get_time_field_info_std_alone(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, int tmp_ref, ticks_t *  time, field_type_t * last_field,
		field_type_t * first_field, int * nr_of_fields) {


	/* Tmp ref */
	uint16_t highest_tmp_ref;
	uint16_t pre_higest_tmp_ref;

	/* Picture info */
	picture_info_t picture_info;

	/* Pes holders */
	pes_holder_t * tmp_pes_holder;

	highest_tmp_ref = get_highest_tmp_ref(seq_pes_holder);

	LOG_DEVDIAG1("In std alone time field - tmp_ref %i \n", tmp_ref);


	if(highest_tmp_ref < tmp_ref) {
		return(get_time_field_info_std_alone(tystream, next_seq_holder(seq_pes_holder), tmp_ref - highest_tmp_ref - 1, time, last_field,
			first_field, nr_of_fields));
	} else if (tmp_ref < 0) {

		tmp_pes_holder = previous_seq_holder(seq_pes_holder);
		//print_seq(tmp_pes_holder);
		pre_higest_tmp_ref = get_highest_tmp_ref(tmp_pes_holder);
		tmp_ref = pre_higest_tmp_ref + (tmp_ref + 1);
		LOG_DEVDIAG1("Tmp ref %i\n", tmp_ref);
		return(get_time_field_info_std_alone(tystream, tmp_pes_holder, tmp_ref, time, last_field,
			first_field, nr_of_fields));
	}


	get_picture_info_tmp_ref(tystream, seq_pes_holder, tmp_ref, &picture_info);


	*time = get_time_of_tmp_ref(seq_pes_holder, tmp_ref);

	if(picture_info.top_field_first &&
		picture_info.repeat_first_field) {

			*first_field = TOP_FIELD;
			*last_field = TOP_FIELD;
			*nr_of_fields = 3;

	} else if (!picture_info.top_field_first &&
			picture_info.repeat_first_field) {

			*first_field = BOTTOM_FIELD;
			*last_field = BOTTOM_FIELD;
			*nr_of_fields = 3;

	} else if (picture_info.top_field_first &&
			!picture_info.repeat_first_field) {

			*first_field = TOP_FIELD;
			*last_field = BOTTOM_FIELD;
			*nr_of_fields = 2;

	} else if (!picture_info.top_field_first &&
			!picture_info.repeat_first_field) {

			*first_field = BOTTOM_FIELD;
			*last_field = TOP_FIELD;
			*nr_of_fields = 2;
	}

	return(1);

}

