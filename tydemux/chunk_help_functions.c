
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


int get_nr_audio_pes(tystream_holder_t * tystream, const chunk_t * chunks) {

	/* Iteration */
	int i;

	/* Number of pes records */
	int nr_pes_records;

	nr_pes_records = 0;
	if(tystream->tivo_series == S1 || tystream->tivo_series == S2) {
		for(i=0 ; i < chunks->nr_records; i++) {
			switch(chunks->record_header[i].type){
				case 0x3c0:
				case 0x9c0:
					nr_pes_records++;
			}
		}
	} else {
		/* FIXME need to return -1 */
		LOG_WARNING("get_nr_audio_pes: Error tivo series not set\n");
	}
	return(nr_pes_records);
}

int get_nr_video_pes(tystream_holder_t * tystream, const chunk_t * chunks) {

	/* Iteration */
	int i;

	/* Number of pes records */
	int nr_pes_records;

	nr_pes_records = 0;

	if(tystream->tivo_series == S1 ) {
		for(i=0 ; i < chunks->nr_records; i++) {
			switch(chunks->record_header[i].type){
				case 0x6e0:
					nr_pes_records++;
			}
		}
	} else if (tystream->tivo_series == S2) {
		for(i=0 ; i < chunks->nr_records; i++) {
			switch(chunks->record_header[i].type){
				/* S2 tivo has the pes header embedded
				in SEQ, P and B records */
				case 0x7e0:
				case 0xae0:
				case 0xbe0:
					nr_pes_records++;
			}
		}
	} else {
		/* FIXME need to return -1 */
		LOG_WARNING("get_nr_video_pes: Error tivo series not set\n");
	}

	return(nr_pes_records);
}

int get_nr_audio_records(tystream_holder_t * tystream, const chunk_t * chunks) {

	/* Iteration */
	int i;

	/* Number of pes records */
	int nr_audio_records;

	nr_audio_records = 0;
	if(tystream->tivo_version == V_13) {
		for(i=0 ; i < chunks->nr_records; i++) {
			switch(chunks->record_header[i].type){
				case 0x4c0:
					nr_audio_records++;
			}
		}
	} else {
		/* FIXME need to return -1 */
		LOG_WARNING("get_nr_audio_records: You are using a V_13 function on a 2X or newer stream\n");
	}
	return(nr_audio_records);
}


