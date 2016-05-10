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

/* FIXME DUP CODE */

/* Internal stubs */

/* Video */
/* Used by parse_chunk_video_remainder_S1 to separate P-Frames from B-Frames in SA/UK tivos */
static pes_holder_t * fix_pes_holder(tystream_holder_t * tystream, pes_holder_t * pes_holder);
/* Main video parse funcs */
static int parse_chunk_video_remainder_S1(int i, tystream_holder_t * tystream, chunk_t * chunks, pes_holder_t * pes_holder, int nr_records);
static int parse_chunk_video_S1(tystream_holder_t * tystream, chunk_t * chunks, int cut);
static int parse_chunk_video_remainder_S2(int i, tystream_holder_t * tystream, chunk_t * chunks, pes_holder_t * pes_holder, int nr_records);
static int parse_chunk_video_S2(tystream_holder_t * tystream, chunk_t * chunks, int cut);
static int parse_chunk_video(tystream_holder_t * tystream, chunk_t * chunks, int cut);

/* Audio */
/* Main audio parse funcs */
static int parse_chunk_audio_V_2X(tystream_holder_t * tystream, chunk_t * chunks);




static int parse_chunk_video_remainder_S1(int i, tystream_holder_t * tystream, chunk_t * chunks, pes_holder_t * pes_holder, int nr_records) {

	/* Payload holder */
	payload_t * payload;

	/* Data collector module */
	module_t data_collector_module;

	/* Temproal reference */
	uint16_t tmp_ref;

	/* Results */
	int result;

	/* Marker */
	int gotit;

	/* Payload type */
	payload_type_t type;

	/* Reset gotit */
	gotit=0;

	/* And now the rest of the video pes */
	for(; i < nr_records; i++) {

		data_collector_module.buffer_size = 0;
		data_collector_module.data_buffer = NULL;
		result=0;
		type = UNKNOWN_PAYLOAD;
		tmp_ref = 0;

		switch(chunks->record_header[i].type){
			case 0x7e0:
				/* SEQ Header */
				result = get_video(i, &data_collector_module, chunks, MPEG_SEQ, tystream);
				if(!result) {
					LOG_WARNING2("parse_chunk_video: ERROR - seq header - chunk " I64FORMAT ", record %i\n",
						chunks->chunk_number, i);
					return(0);
				}
				type = SEQ_HEADER;
				pes_holder->seq_present++;
				break;

			case 0x8e0:
				/* I-Frame */
				result = get_video(i, &data_collector_module, chunks, MPEG_I, tystream);
				if(!result) {
					LOG_WARNING2("parse_chunk_video: ERROR - i-frame - chunk " I64FORMAT ", record %i\n",
						chunks->chunk_number, i);
					return(0);
				}
				tmp_ref = get_temporal_reference(data_collector_module.data_buffer);
				type = I_FRAME;
				pes_holder->i_frame_present++;
				break;

			case 0xae0:
				/* P-Frame */
				result = get_video(i, &data_collector_module, chunks, MPEG_P, tystream);
				if(!result) {
					LOG_WARNING2("parse_chunk_video: ERROR - p-frame - chunk " I64FORMAT ", record %i\n",
						chunks->chunk_number, i);
					return(0);
				}
				tmp_ref = get_temporal_reference(data_collector_module.data_buffer);
				type = P_FRAME;
				pes_holder->p_frame_present++;
				break;

			case 0xbe0:
				/* B-Frame */
				result = get_video(i, &data_collector_module, chunks, MPEG_B, tystream);
				if(!result) {
					LOG_WARNING2("parse_chunk_video: ERROR - b-frame - chunk " I64FORMAT ", record %i\n",
						chunks->chunk_number, i);
					return(0);
				}
				tmp_ref = get_temporal_reference(data_collector_module.data_buffer);
				type = B_FRAME;
				pes_holder->b_frame_present++;
				break;

			case 0xce0:
				/* GOP Header */
				result = get_video(i, &data_collector_module, chunks, MPEG_GOP, tystream);
				if(!result) {
					LOG_WARNING2("parse_chunk_video: ERROR - gop header - chunk " I64FORMAT ", record %i\n",
						chunks->chunk_number, i);
					return(0);
				}
				type = GOP_HEADER;
				pes_holder->gop_present++;
				break;

			case 0xe01:
				/* Closed Caption */
				type = CLOSED_CAPTION;
				pes_holder->data_present++;
				break;

			case 0xe02:
				/* XDS */
				type = XDS_DATA;
				pes_holder->data_present++;
				break;

			case 0x6e0:
				gotit=1;
				break;
		}
		if(gotit) {
			break;
		} else if (type != UNKNOWN_PAYLOAD) {
			payload = NULL;
			payload = new_payload();
			payload->payload_type = type;
			payload->tmp_ref = tmp_ref;
			if(type == XDS_DATA || type == CLOSED_CAPTION) {
				payload->size = 2;
				payload->payload_data = (uint8_t *)malloc(sizeof(uint8_t) * 2);
				memset(payload->payload_data, 0,sizeof(uint8_t) * 2);
				memcpy(payload->payload_data,chunks->record_header[i].extended_data, sizeof(uint8_t) * 2);
			} else {
				payload->size = data_collector_module.buffer_size;
				payload->payload_data = data_collector_module.data_buffer;
			}

			/* Add the payload (SEQ header) to the pes_holder */
			add_payload(pes_holder, payload);

			pes_holder->total_size = pes_holder->total_size + payload->size;
			pes_holder->size =  pes_holder->size + payload->size;
		}
	}

	if(!gotit) {
		if(chunks->next != NULL) {
			if(!chunks->next->gap) {
				/* Catch the reminder of the video pes */
				return(parse_chunk_video_remainder_S1(0,tystream, chunks->next, pes_holder, chunks->next->nr_records));
			} else {
				return(0);
			}
		} else {
			return(0);
		}
	} else {
		/* Add the pes holder */
		if (pes_holder->i_frame_present + pes_holder->p_frame_present + pes_holder->b_frame_present > 1 && tystream->tivo_version == V_2X) {
			if(!tystream->multiplex) {
				pes_holder = fix_pes_holder(tystream, pes_holder);
			}
		}
		result=0;
		result = add_pes_holder(tystream, pes_holder);

		if(result == 0) {
			LOG_WARNING1("parse_chunk_video: ERROR - adding pes holder - chunk " I64FORMAT "\n",
				chunks->chunk_number);
		}
		/* FIXME How to return when we didn't manage to add the pes holder */
		return(1);
	}

}

/* FIXME NOTE THIS ONE DOESN'T HAVE A PES HEADER!! FIXME
This is a quick fix so repair some what work in on SA S1 streams
Proper repair needs real PTS time stamps! Proper PES holders
are also needed for the remux */

static pes_holder_t * fix_pes_holder(tystream_holder_t * tystream, pes_holder_t * pes_holder) {

	/* Pes holder array */
	pes_holder_t * tmp_pes_holder;
	pes_holder_t * tmp_new_pes_holder;

	payload_t * tmp_payload;
	payload_t * pre_payload;

	/* Marker */
	int first;

	tmp_pes_holder = pes_holder;
	tmp_payload = pes_holder->payload;
	first = 0;

	if(ty_debug >= 7) {
		LOG_DEVDIAG("In fix pes holder\n");
	}

	while(tmp_payload) {
		if((tmp_payload->payload_type == I_FRAME || tmp_payload->payload_type == P_FRAME
			|| tmp_payload->payload_type == B_FRAME) && first != 1) {

			first = 1;
			tmp_payload = tmp_payload->next;
			continue;


		} else  if ((tmp_payload->payload_type == I_FRAME || tmp_payload->payload_type == P_FRAME
			|| tmp_payload->payload_type == B_FRAME) && first) {

			tmp_new_pes_holder = NULL;
			tmp_new_pes_holder = new_pes_holder(VIDEO);

			/* Attache the payload to the pes holder */
			tmp_new_pes_holder->payload = tmp_payload;

			/* Split payload */
			pre_payload = tmp_payload->previous;

			/* Reset previous and next */
			tmp_payload->previous = NULL;
			pre_payload->next = NULL;

			/* Now reinit both pes_holders */
			reinit_pes_holder(tmp_pes_holder);
			reinit_pes_holder(tmp_new_pes_holder);

			tmp_new_pes_holder->time = 0;

			tmp_pes_holder->next = tmp_new_pes_holder;
			tmp_new_pes_holder->previous = tmp_pes_holder;


			tmp_payload = tmp_payload->next;
			tmp_pes_holder = tmp_pes_holder->next;
			continue;

		} else {
			tmp_payload = tmp_payload->next;
		}
	}

	return(pes_holder);
}




static int parse_chunk_video_S1(tystream_holder_t * tystream, chunk_t * chunks, int cut) {

	/* Interation */
	int i;

	/* Pes holder */
	pes_holder_t * pes_holder;

	/* Payload holder */
	payload_t * payload;
	payload_t * seq_payload;


	/* Data collector module */
	module_t data_collector_module;

	/* number of records */
	int nr_records;


	/* Results */
	int result;

	/* Marker */
	int gotit;
	int gap;

	if(cut == 2) {
		/* We never start a cut in a gap - hence even if this is a gap there is no use to
		repair */
		gap = 0;
	} else {
		gap = 1;
	}

	/* Okay so if the cut == 2 then we should not parse record up to that record minus one (the PES
	that goes with the I frame  (however if this is the start of the stream we parse the whole thing
	*/
	if(cut == 2) {
		i = cutpoint_incut_chunk_record(tystream, chunks)  - 2;
	} else {
		i = 0;
	}

	if(cut == 1) {
		nr_records = cutpoint_incut_chunk_record(tystream, chunks);
	} else {
		nr_records = chunks->nr_records;
	}



	LOG_DEVDIAG2("parse_chunk_video: chunk " I64FORMAT " - gap %i\n", chunks->chunk_number, chunks->gap);

	/* This is the main loop where we get all video pes
	in the chunk */
	/* FIXME Check the nr pes algo */
	for(; i < nr_records; i++) {
		switch(chunks->record_header[i].type){
			case 0x6e0:
				/* Okay we got a video pes header
				let make a pes holder */
				pes_holder = NULL;
				pes_holder = new_pes_holder(VIDEO);
				pes_holder->chunk_nr = chunks->chunk_number;
				/* FIXME We don't catch the GAP if we don't have a
				PES header in the chunk !!! */
				//printf("parse_chunk_video: chunk " I64FORMAT " - record % i - gap %i\n", chunks->chunk_number, i , chunks->gap);

				if(chunks->gap && gap) {
					if(cut == 1) {
						pes_holder->gap=2;
						tystream->repair = 2;
					} else {
						pes_holder->gap=1;
						tystream->repair =1;
					}
					LOG_DEVDIAG2("parse_chunk_video:GAP - pes header - chunk " I64FORMAT ", record %i\n",
						chunks->chunk_number, i);
					gap=0;
				}


				data_collector_module.buffer_size = 0;
				data_collector_module.data_buffer = NULL;
				result = get_video(i, &data_collector_module, chunks, MPEG_PES_VIDEO, tystream);
				if(!result) {
					if(pes_holder->gap) {
						gap = 1;
					}
					LOG_DEVDIAG2("parse_chunk_video: ERROR - pes header - chunk " I64FORMAT ", record %i\n",
						chunks->chunk_number, i);
					free_pes_holder(pes_holder, 1);
					break;
				}
				payload = NULL;
				payload = new_payload();
				payload->size = data_collector_module.buffer_size;
				payload->payload_type = PES_VIDEO;
				payload->payload_data = data_collector_module.data_buffer;

				/* Get the time from the pes header */
				pes_holder->time = get_time(payload->payload_data + PTS_TIME_OFFSET);

				/* Add the payload (pes header) to the pes_holder */
				add_payload(pes_holder, payload);

				/* Init sizes */
				pes_holder->total_size = data_collector_module.buffer_size;

				if(cut == 2 ) {
					/* We need a SEQ here the problem
					is if this is the first seq in the
					stream and we made a cut :( - solved in cutpoints */
					seq_payload = payload_fetch_seq_gop(tystream->pes_holder_video);
					if(seq_payload) {
						add_payload(pes_holder, seq_payload);
						reinit_pes_holder(pes_holder);
					} else {
						LOG_WARNING("Error in cut get seq\n");
					}
					pes_holder->repaired = 1;
					pes_holder->make_closed_gop = 1;
					cut = 0;
				}



				/* Reset gotit */
				gotit=0;

				gotit = parse_chunk_video_remainder_S1(i+1, tystream, chunks, pes_holder, nr_records);

				if(!gotit) {
					if(pes_holder && pes_holder->gap) {
						gap = 1;
					}

					LOG_DEVDIAG2("parse_chunk_video: Error - getting pes - chunk " I64FORMAT ", record %i\n",
						chunks->chunk_number, i);
					/* We never added the pes holder if this faild FIXME */
					free_all_pes_holders(pes_holder);

				}
				break;
		}
	}
	/* FIXME How to return when we didn't manage to add some of the pes holders */
	if(chunks->gap && gap && chunks->next) {
		//printf("Fallback setting gap to next chunk\n");
		chunks->next->gap =1;
	} else if (chunks->gap && gap) {
		LOG_ERROR1("We will fail to repair Chunk " I64FORMAT "\n", chunks->chunk_number);
	}


	return(1);
}


/******************************/


static int parse_chunk_video_remainder_S2(int i, tystream_holder_t * tystream, chunk_t * chunks, pes_holder_t * pes_holder, int nr_records) {

	/* Payload holder */
	payload_t * payload;

	/* Data collector module */
	module_t data_collector_module;

	/* Temporal reference */
	uint16_t tmp_ref;

	/* Results */
	int result;

	/* Marker */
	int gotit;

	/* Payload type */
	payload_type_t type;

	/* Reset gotit */
	gotit=0;

	/* And now the rest of the video pes */
	for(; i < nr_records; i++) {

		data_collector_module.buffer_size = 0;
		data_collector_module.data_buffer = NULL;
		tmp_ref = 0;
		result=0;
		type = UNKNOWN_PAYLOAD;

		switch(chunks->record_header[i].type){
			case 0x8e0:
				/* I-Frame */
				result = get_video(i, &data_collector_module, chunks, MPEG_I, tystream);
				if(!result) {
					LOG_WARNING2("parse_chunk_video: ERROR S2 - i-frame - chunk " I64FORMAT ", record %i\n",
							chunks->chunk_number, i);
					return(0);
				}
				tmp_ref = get_temporal_reference(data_collector_module.data_buffer);
				type = I_FRAME;
				pes_holder->i_frame_present++;
				break;

			case 0xce0:
				/* GOP Header */
				result = get_video(i, &data_collector_module, chunks, MPEG_GOP, tystream);
				if(!result) {
					LOG_WARNING2("parse_chunk_video: ERROR S2 - gop header - chunk " I64FORMAT ", record %i\n",
							chunks->chunk_number, i);
					return(0);
				}
				type = GOP_HEADER;
				pes_holder->gop_present++;
				break;

			case 0xe01:
				/* Closed Caption */
				type = CLOSED_CAPTION;
				pes_holder->data_present++;
				break;

			case 0xe02:
				/* XDS */
				type = XDS_DATA;
				pes_holder->data_present++;
				break;

			case 0x7e0: /* SEQ type record */
			case 0xae0: /* P-Frame record */
			case 0xbe0: /* B-Frame record */
				gotit=1;
				break;
		}
		if(gotit) {
			break;
		} else if (type != UNKNOWN_PAYLOAD) {

			payload = NULL;
			payload = new_payload();
			payload->payload_type = type;
			payload->tmp_ref = tmp_ref;
			if(type == XDS_DATA || type == CLOSED_CAPTION) {
				payload->size = 2;
				payload->payload_data = (uint8_t *)malloc(sizeof(uint8_t) * 2);
				memset(payload->payload_data, 0, sizeof(uint8_t) * 2);
				memcpy(payload->payload_data,chunks->record_header[i].extended_data, sizeof(uint8_t) * 2);
			} else {
				payload->size = data_collector_module.buffer_size;
				payload->payload_data = data_collector_module.data_buffer;
			}

			/* Add the payload (SEQ header) to the pes_holder */
			add_payload(pes_holder, payload);
			pes_holder->total_size = pes_holder->total_size + payload->size;
			pes_holder->size =  pes_holder->size + payload->size;

		}
	}

	if(!gotit) {
		if(chunks->next != NULL) {
			if(!chunks->next->gap) {
				/* Catch the reminder of the video pes */
				return(parse_chunk_video_remainder_S2(0,tystream, chunks->next, pes_holder, chunks->next->nr_records));
			} else {
				if(pes_holder) {
					free_pes_holder(pes_holder,1);
					pes_holder = NULL;
				}
				return(0);
			}
		} else {
			if(pes_holder) {
				free_pes_holder(pes_holder,1);
				pes_holder = NULL;
			}

			return(0);
		}
	} else {
		/* Add the pes holder */
		result=0;
		result = add_pes_holder(tystream, pes_holder);
		if(result == 0) {
			LOG_WARNING1("parse_chunk_video: ERROR S2 - adding pes holder - chunk " I64FORMAT "\n",
				chunks->chunk_number);
		}
		/* FIXME How to return when we didn't manage to add the pes holder */
		return(1);
	}

}


static int parse_chunk_video_S2(tystream_holder_t * tystream, chunk_t * chunks, int cut) {

	/* Interation */
	int i;

	/* Pes holder */
	pes_holder_t * pes_holder;

	/* Payload holder */
	payload_t * payload;

	/* Data collector module */
	module_t data_collector_module;

	/* number of records */
	int nr_records;

	/* Results */
	int result;

	/* Marker */
	int gotit;
	int gap;


	if(cut == 2) {
		/* We never start a cut in a gap - hence even if this is a gap there is no use to
		repair */
		gap = 0;
	} else {
		gap = 1;
	}

	/* Okay so if the cut == 2 then we should not parse records up to that
	record  rember S2 has a PES holder embeded in the SEQ */
	if(cut == 2) {
		i = cutpoint_incut_chunk_record(tystream, chunks);
	} else {
		i = 0;
	}

	if(cut == 1) {
		nr_records = cutpoint_incut_chunk_record(tystream, chunks);
	} else {
		nr_records = chunks->nr_records;
	}


	/* This is the main loop where we get all video pes headers
	in the chunk - S2 has them embedded in SEQ, B-Frame and P-Frame records */
	/* FIXME Check the nr pes algo */

	for(; i < nr_records; i++) {
		switch(chunks->record_header[i].type){
			case 0x7e0: /* SEQ type record */
			case 0xae0: /* P-Frame record */
			case 0xbe0: /* B-Frame record */
 				/* Okay we got a embeded video pes header
				let make a pes holder */
				pes_holder = NULL;
				pes_holder = new_pes_holder(VIDEO);
				if(chunks->gap && gap) {
					if(cut == 1) {
						pes_holder->gap=2;
						tystream->repair =2;
					} else {
						pes_holder->gap=1;
						tystream->repair =1;
					}
					gap=0;
				}

				if(cut == 2 && chunks->record_header[i].type == 0x7e0) {
					pes_holder->repaired = 1;
					pes_holder->make_closed_gop = 1;
					cut = 0;
				}

				data_collector_module.buffer_size = 0;
				data_collector_module.data_buffer = NULL;
				result = get_video(i, &data_collector_module, chunks, MPEG_PES_VIDEO, tystream);
				if(!result) {
					if(pes_holder->gap) {
						gap = 1;
					}

					if(ty_debug >= 2) {
						LOG_WARNING2("parse_chunk_video: ERROR S2 - pes header - chunk " I64FORMAT ", record %i\n",
							chunks->chunk_number, i);
					}
					free_pes_holder(pes_holder,1);
					return(0);
				}
				payload = NULL;
				payload = new_payload();
				payload->size = data_collector_module.buffer_size;
				payload->payload_type = PES_VIDEO;
				payload->payload_data = data_collector_module.data_buffer;

				/* Get the time from the pes header */
				pes_holder->time = get_time(payload->payload_data + PTS_TIME_OFFSET);

				/* Add the payload (pes header) to the pes_holder */
				add_payload(pes_holder, payload);

				/* Init sizes */
				pes_holder->total_size = data_collector_module.buffer_size;

				/* Reset the  variables and get the SEQ, P/B-Frame */
				data_collector_module.buffer_size = 0;
				data_collector_module.data_buffer = NULL;
				result=0;
				payload = NULL;
				payload = new_payload();

				if(chunks->record_header[i].type == 0x7e0) {
					/* SEQ Header */
					result = get_video(i, &data_collector_module, chunks, MPEG_SEQ, tystream);
					if(!result) {
						LOG_WARNING2("parse_chunk_video: ERROR S2 - seq header - chunk " I64FORMAT ", record %i\n",
								chunks->chunk_number, i);
						free_pes_holder(pes_holder,1);
						return(0);
					}
					payload->payload_type = SEQ_HEADER;
					pes_holder->seq_present++;

				} else if ( chunks->record_header[i].type == 0xae0) {
					/* P-Frame record */
					result = get_video(i, &data_collector_module, chunks, MPEG_P, tystream);
					if(!result) {
						LOG_WARNING2("parse_chunk_video: ERROR S2- p-frame - chunk " I64FORMAT ", record %i\n",
								chunks->chunk_number, i);
						free_pes_holder(pes_holder,1);
						return(0);
					}
					payload->tmp_ref = get_temporal_reference(data_collector_module.data_buffer);
					payload->payload_type = P_FRAME;
					pes_holder->p_frame_present++;

				} else {
					/* B-Frame record */
					result = get_video(i, &data_collector_module, chunks, MPEG_B, tystream);
					if(!result) {
						LOG_WARNING2("parse_chunk_video: ERROR S2 - b-frame - chunk " I64FORMAT ", record %i\n",
								chunks->chunk_number, i);
						free_pes_holder(pes_holder,1);
						return(0);
					}
					payload->tmp_ref = get_temporal_reference(data_collector_module.data_buffer);
					payload->payload_type = B_FRAME;
					pes_holder->b_frame_present++;
				}

				payload->size = data_collector_module.buffer_size;
				payload->payload_data = data_collector_module.data_buffer;

				/* Add the payload (SEQ header or P/B-Frame ) to the pes_holder */
				add_payload(pes_holder, payload);
				pes_holder->total_size = pes_holder->total_size + data_collector_module.buffer_size;
				pes_holder->size =  pes_holder->size + data_collector_module.buffer_size;


				/* Reset gotit */
				gotit=0;

				gotit = parse_chunk_video_remainder_S2(i+1, tystream, chunks, pes_holder, nr_records);

				if(!gotit) {
					if(pes_holder && pes_holder->gap) {
						gap = 1;
					}
					LOG_WARNING2("parse_chunk_video: Error S2 - getting pes - chunk " I64FORMAT ", record %i\n",
						chunks->chunk_number, i);
					/* We never added the pes holder if this faild FIXME */
					free_pes_holder(pes_holder, 1);

				}
				break;
		}
	}
	/* FIXME How to return when we didn't manage to add some of the pes holders */
	if(chunks->gap && gap && chunks->next) {
		//printf("Fallback setting gap to next chunk\n");
		chunks->next->gap =1;
	} else if (chunks->gap && gap) {
		LOG_ERROR1("We will fail to repair Chunk " I64FORMAT "\n", chunks->chunk_number);
	}


	return(1);
}





static int parse_chunk_video(tystream_holder_t * tystream, chunk_t * chunks, int cut) {
	if(tystream->tivo_series == S1) {
		return(parse_chunk_video_S1(tystream, chunks, cut));
	} else if (tystream->tivo_series == S2) {
		return(parse_chunk_video_S2(tystream, chunks, cut));
	} else {
		LOG_ERROR("parse_chunk_video: Error Tivo series undefined \n");
		return(0);
	}
}




static int parse_chunk_audio_V_2X(tystream_holder_t * tystream, chunk_t * chunks) {

	/* Interation */
	int i;

	/* Pes holder */
	pes_holder_t * pes_holder;

	/* Payload holder */
	payload_t * payload;

	/* Data collector module */
	audio_module_t data_collector_module;

	/* Results */
	int result;

	/* Marker */
	int gap;
	
	/* Drift */
	ticks_t time;
	ticks_t start_drift;
	ticks_t end_drift;
	ticks_t total_drift;
	int repeat;

	gap = 1;


	for(i=0; i < chunks->nr_records; i++) {
		switch(chunks->record_header[i].type){
			case 0x3c0:
			case 0x9c0:
				/* Okay we got a audio pes header
				let make a pes holder */
				pes_holder = NULL;
				pes_holder = new_pes_holder(AUDIO);
				pes_holder->chunk_nr = chunks->chunk_number;
				if(chunks->gap && gap) {
					pes_holder->gap=1;
					gap=0;
				}
				data_collector_module.data_buffer_size = 0;
				data_collector_module.data_buffer = NULL;

				data_collector_module.pes_buffer_size = 0;
				data_collector_module.pes_data_buffer = NULL;

				result = get_audio(i, &data_collector_module, chunks, tystream->audio_startcode, tystream);
				if(!result) {
					if(pes_holder->gap == 1) {
						gap = 1;
					}
					LOG_WARNING2("parse_chunk_video: ERROR - pes header - chunk " I64FORMAT ", record %i\n",
						chunks->chunk_number, i);
					free_pes_holder(pes_holder,1);
					break;
				}
				/* Get the pes header into the payload */
				payload = NULL;
				payload = new_payload();
				payload->size = data_collector_module.pes_buffer_size;
				if(chunks->record_header[i].type == 0x3c0) {
					payload->payload_type = PES_MPEG;
				} else {
					payload->payload_type = PES_AC3;
				}
				payload->payload_data = data_collector_module.pes_data_buffer;

				/* Get the time from the pes header NOTE DTivo MPEG pes headers are
				repair that this point */
				pes_holder->time = get_time(payload->payload_data + PTS_TIME_OFFSET);





				/* Add the payload (pes header) to the pes_holder */
				add_payload(pes_holder, payload);
				/* Init sizes */
				pes_holder->total_size = data_collector_module.pes_buffer_size;





				/* Get the audio data into the payload */
				payload = NULL;
				payload = new_payload();
				payload->size = data_collector_module.data_buffer_size;
				if(chunks->record_header[i].type == 0x3c0) {
					payload->payload_type = MPEG_AUDIO_FRAME;
				} else {
					payload->payload_type = AC3_AUDIO_FRAME;
				}
				payload->payload_data = data_collector_module.data_buffer;

				/* Add the payload (pes header) to the pes_holder */
				add_payload(pes_holder, payload);

				/* Init sizes */
				pes_holder->total_size = pes_holder->total_size + data_collector_module.data_buffer_size;
				pes_holder->size = data_collector_module.data_buffer_size;
				pes_holder->audio_present = 1;

				/*if(pes_holder->time == (ticks_t)44703356 || pes_holder->time == (ticks_t)44705516 ||
					pes_holder->time == (ticks_t)92214714 || pes_holder->time == (ticks_t)92216874) {
						printf("In mystic cut\n");
				}*/


				/* Okay we check if we are with in a cut */
				time = cutpoint_incut_ticks_cutpoint(tystream, pes_holder->time);

				if(time) {
					/* Okay so we are in some sort of cut */
					if(time == -1) {
						//LOG_USERDIAG("Normal cut\n");
						/* Normal cut just drop the pes */
						free_pes_holder(pes_holder, 1);
						break;
					}
					//printf("Time is " I64FORMAT "\n", time);
					if(time != -1) {
						/* We are either just out of the cut or just at the start of the cut */
						if(tystream->audiocut) {
							/* We are just out of the cut - now we need to see how much drift we created */
							end_drift = time - pes_holder->time;
							//LOG_USERDIAG2("End -  " I64FORMAT " - pestime " I64FORMAT "\n", pes_holder->time, time);

							/* Now lest see how much drift we had in the start */
							start_drift = tystream->audio_start_drift;

							total_drift = start_drift + end_drift;

							/* Should we compensate in the audio stream ? */
							if(total_drift < 0) {
								/* We are missing audio playtime */
								repeat = (int)( llabs(total_drift) / tystream->audio_median_tick_diff);
								if(repeat) {
									pes_holder->repeat = repeat;
									/* End drift is always negative */
									total_drift = total_drift + repeat * tystream->audio_median_tick_diff;
								}
							}

							/* Attache sync object to i_frame_pes holder - we have not yet fixed the seq hence we
							will attach to the i frame since it will become the seq holder */
							//LOG_USERDIAG("Attaching drift object\n");
							pes_holder_attache_drift_i_frame(tystream->pes_holder_video, time, total_drift);

							/* Mark this pes as repaired so we don't repeate audio */
							pes_holder->repaired = 1;

							/* Reset the audio cut */
							tystream->audiocut = 0;
							tystream->audio_start_drift = 0;

							if(pes_holder->gap) {
								pes_holder->gap = 0;
							}

						} else {

							/* We must be at the start of the cut */
							/* However if this is the start of the stream we must not set
							the values here since this is really the stop and not the start */
							/*if(!tystream->pes_holder_audio) {
								add_pes_holder(tystream, pes_holder);
								break;
							}*/
							start_drift = pes_holder->time - time;
							//LOG_USERDIAG2("Start drift time: " I64FORMAT " - pestime " I64FORMAT "\n", pes_holder->time, time);
							tystream->audiocut = 1;
							tystream->audio_start_drift = start_drift;
							free_pes_holder(pes_holder, 1);
//							if(pes_holder->gap) {
//								pes_holder->gap = 2;
//							}
							break;
						}
					}
				}

				/* Add the pes holder and break */
				add_pes_holder(tystream, pes_holder);
				break;
		}
	}

	/* FIXME How to return when we didn't manage to add some of the pes holders */
	if(chunks->gap && gap && chunks->next) {
		//printf("Fallback setting gap to next chunk\n");
		chunks->next->gap =1;
	} else if (chunks->gap && gap) {
		LOG_ERROR1("We will fail to repair Chunk " I64FORMAT "\n", chunks->chunk_number);
	}



	return(1);
}




static int parse_chunk_audio(tystream_holder_t * tystream, chunk_t * chunks) {

	if(tystream->tivo_version == V_2X) {
		return(parse_chunk_audio_V_2X(tystream, chunks));
	} else {
		if (ty_debug >= 1) {
			LOG_ERROR("parse_chunk_audio: Error Tivo version undefied or unsupported\n");
		}
		return(0);
	}
}


chunk_t * parse_chunk(tystream_holder_t * tystream, chunk_t * chunks) {


	int cut_value;

	cut_value = cutpoint_incut_chunk(tystream, chunks->chunk_number);

	LOG_DEVDIAG2("Chunk " I64FORMAT " - cutvalue %i \n", chunks->chunk_number,cut_value);
	//printf("Chunk " I64FORMAT " - cutvalue %i \n", chunks->chunk_number,cut_value);

	switch(cut_value) {

		case 0:
			parse_chunk_video(tystream, chunks, 0);
			parse_chunk_audio(tystream, chunks);
			break;
		case 1:
			parse_chunk_video(tystream, chunks, 1);
			parse_chunk_audio(tystream, chunks);
			break;
		case 2:
			parse_chunk_video(tystream, chunks, 2);
			parse_chunk_audio(tystream, chunks);
			break;
		case 3:
			parse_chunk_audio(tystream, chunks);
			break;
		default:
			LOG_ERROR("Error in cuts parse_chunk\n");
			break;
	}

	if(chunks->previous) {
		free_chunk(chunks->previous);
		chunks->previous = NULL;
		if(chunks->next) {
			return(chunks->next);
		} else {
			return(0);
		}
	} else {
		if (chunks->next) {
			return(chunks->next);
		} else {
			return(0);
		}
	}
}


int chunk_okay_to_parse(const chunk_t * chunks) {


	int treshhold;
	chunk_t * tmp_chunks;

	/* Init */
	tmp_chunks = (chunk_t *)chunks;
	treshhold = 0;

	while(tmp_chunks) {
		treshhold++;
		if(treshhold > 3) {
			return(1);
		}
		tmp_chunks = tmp_chunks->next;
	}

	return(0);

}






