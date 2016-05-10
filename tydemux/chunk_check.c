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
static void bsort(ticks_t video_time_array[], int size);


/* FIXME DUP CODE */

chunk_t * check_chunk(tystream_holder_t * tystream, chunk_t * chunk) {

	/* iteration */
	int i, j;

	/* Video time array */
	ticks_t * tmp_video_time_array;

	/* Video time ticks */
	ticks_t first_tick;
	ticks_t tmp_max_ticks;
	ticks_t tmp_min_ticks;

	/* Video sync okay or not */
	int okay;
	int test_val;

	/* Nr of video element */
	int nr_video;

	/* Pointers to do magic with */
	uint8_t * pt1;


	/* Init it */
	nr_video = 0;
	okay = 0;
	test_val = 0;
	tmp_max_ticks = 0;
	tmp_min_ticks = INITAL_MIN_TICKS;
	first_tick = 0;

	/**
	 * Video in sync check
	 *
	 * So we passed the checks above - lets now see if we
	 * have Video and Audio in sync i.e. that our PTS
	 * for Video doesn't differ more than 4 * max_ticks from our
	 * previous chunk last PTS for Video.
	 *
	 * NOTE: This is a very rough check - if we find something
	 * that is suspicius - but not totally out of sync (i.e. not more
	 * than 10 * max_ticks (around ten frames), we will raise the gap
	 * flag in the chunk - later on during parsing we will do a
	 * audio sync check to see if we have a real gap or not.
	 * If so we will try to repair the TyStream.
	 *
	 * Now it can happen than we have a really huge gap (more
	 * then 10 * max_ticks) but it's not a junk chunk - what
	 * will happen is that we never will find sync again and
	 * skip all chunks in the rest of the tyStream. Not good
	 * at all.
	 *
	 * The solution is to save all chunks we throw away (with the
	 * exception of the above chunk tests). Then if we have
	 * skiped XX chunks we halt and check if all those (more or less)
	 * chunks are in sync. If so we will insert them into the chunks
	 * linked list and set the gap flag so we can repair it later on.
	 *
	 * NOTE: If we don't find any Video frames hence can't determine
	 * sync we will save the chunk raise the gap flag.
	 *
	 */


	nr_video = get_nr_video_pes(tystream, chunk);

	if(nr_video != 0) {

		/* Okay so we got some video - lest start geting the PTS time
		for each of them - we will use the get_time func instead of
		get_time_X since it's slower */

		tmp_video_time_array = (ticks_t *)malloc(sizeof(ticks_t) * nr_video);
		memset(tmp_video_time_array, 0,sizeof(ticks_t) * nr_video);

		/* This loop will break if TIVO make a change in the way they
		handle the VIDEO PES header - but it's much faster than using
		get_time_X hence we make a short cut but be prepared */
		if(tystream->tivo_series == S1) {
			for(i=0, j=0; i < chunk->nr_records; i++) {
				switch(chunk->record_header[i].type) {
					case 0x6e0:
						if(chunk->record_header[i].size < 16 ) {
							LOG_WARNING2("Record to small - chunk " I64FORMAT " record %i\n", chunk->chunk_number, i);
							test_val++;
						} else {
							pt1 = chunk->records[i].record ;
							pt1 = pt1 + PTS_TIME_OFFSET;
							tmp_video_time_array[j] = get_time(pt1);
							LOG_DEVDIAG1("Check Chunk: Time " I64FORMAT " \n", tmp_video_time_array[j]);
							j++;
						}
				}
			}
		} else {
			/* We have a S2 tystream */
			for(i=0, j=0; i < chunk->nr_records; i++) {
				switch(chunk->record_header[i].type) {
					/* S2 tivo has the pes header embedded
					in SEQ, P and B records */
					case 0x7e0:
					case 0xae0:
					case 0xbe0:
						/* Okay for now on we assume that the embedded pes header starts on
						byte 0 - note this is ment to be quick other wise we can easily
						search for the offset */
						if(chunk->record_header[i].size >= PES_MIN_PTS_SIZE) {
							pt1 = chunk->records[i].record ;
							pt1 = pt1 + PTS_TIME_OFFSET;
							tmp_video_time_array[j] = get_time(pt1);
							LOG_DEVDIAG1("Check Chunk: Time " I64FORMAT " \n", tmp_video_time_array[j]);

							j++;
						} else {
							LOG_WARNING("Check Chunk: Error record to small to have a PES PTS\n");
							test_val++;
						}
				}
			}
		}


		if(nr_video > 1) {
			bsort(tmp_video_time_array, nr_video - test_val);
		}


		if(nr_video > 2) {
			for(i=0; i < nr_video - 1; i++){
				if(tmp_max_ticks < (tmp_video_time_array[i+1] - tmp_video_time_array[i])) {
					tmp_max_ticks = tmp_video_time_array[i+1] - tmp_video_time_array[i];
				}
				LOG_DEVDIAG1("Check Chunk: Tick diff: " I64FORMAT "\n", tmp_video_time_array[i+1] - tmp_video_time_array[i]);
				if(tmp_min_ticks > (tmp_video_time_array[i+1] - tmp_video_time_array[i])) {
					tmp_min_ticks = tmp_video_time_array[i+1] - tmp_video_time_array[i];
				}
			}
		}

		if(tystream->last_tick != 0) {

			first_tick = tmp_video_time_array[0];

			if(first_tick > tystream->last_tick) {

				if(first_tick - tystream->last_tick <= 4 * tystream->med_ticks) {

					/* We are okay - educated guess at the moment */
					okay = 1;
					tystream->cons_junk_chunk=0;

					/* We need to free any junk chunk buffers */
					if(tystream->junk_chunks != NULL) {
						free_junk_chunks(tystream->junk_chunks);
						tystream->junk_chunks = NULL;
					}

				} else if (first_tick - tystream->last_tick <= 30 * tystream->med_ticks) {
					/* We are kind of okay but there probably is a gap
					that we need to fix flag the gap flag */
					LOG_WARNING1("Check Chunk: Warning!! Chunk: " I64FORMAT " - hole in video stream\n", chunk->chunk_number);
					LOG_WARNING2("Check Chunk: first_tick " I64FORMAT ", last_tick " I64FORMAT "\n", first_tick, tystream->last_tick);
					okay = 1;
					tystream->cons_junk_chunk=0;

					/* Flag the chunk so we check if it has a small gap */
					chunk->small_gap=1;

					/* We need to free any junk chunk buffers */
					if(tystream->junk_chunks != NULL) {
						free_junk_chunks(tystream->junk_chunks);
						tystream->junk_chunks = NULL;
					}

				} else {

					/* Okay either we hit a gigantic gap or a OSS chunk that is really junk */

					if(tystream->cons_junk_chunk) {
						LOG_WARNING1("Check Chunk: OOS chunk - " I64FORMAT " chunk(s) in a row skiped\r", tystream->cons_junk_chunk);
					} else {
						LOG_WARNING2("Check Chunk: " I64FORMAT " - Out of Sync with " I64FORMAT " ticks - skipping\n", \
						chunk->chunk_number, first_tick - tystream->last_tick);
						LOG_WARNING2("Check Chunk: first_tick " I64FORMAT ", last_tick " I64FORMAT "\n", first_tick, tystream->last_tick);
					}

					tystream->cons_junk_chunk++;

					if(tystream->cons_junk_chunk == JUNK_CHUNK_BUFFER_SIZE) {

							/* Okay go into repair mode */
							if(tystream->junk_chunks->first_tick >= tystream->last_tick) {
								LOG_WARNING1("Check Chunk: Had big gap - " I64FORMAT " ticks -> and then every thing was bad\n"
								"Will try to revert and repair the TyStream better with a gap than nothig\n",
									tystream->junk_chunks->first_tick - tystream->last_tick);
							} else {
								LOG_WARNING1("Check Chunk: Had big gap - " I64FORMAT " ticks -> and then every thing was bad\n"
								"Will try to revert and repair the TyStream better with a gap than nothig\n",
									tystream->last_tick - tystream->junk_chunks->first_tick);
							}
							chunk->last_tick = tmp_video_time_array[nr_video -1];
							chunk->first_tick = first_tick;
							chunk->gap=1;
							chunk->junk=1;

							if(tmp_max_ticks != 0 && tmp_min_ticks != INITAL_MIN_TICKS ) {
								chunk->med_ticks = (tmp_max_ticks + tmp_min_ticks)/2;
								LOG_DEVDIAG2("Check Chunk: Junk with med_ticks " I64FORMAT " - chunk " I64FORMAT "\n", chunk->med_ticks, chunk->chunk_number);

							} else {
								chunk->med_ticks = STD_VIDEO_MED_TICK;
							}
							tystream->junk_chunks = add_chunk(tystream, chunk, tystream->junk_chunks);
							free(tmp_video_time_array);

							/* Lets do a final check of the chunks and return the resulting
							array of junk chunks */
							return(check_junk_chunks(tystream));
					}
				}

			} else if (first_tick < tystream->last_tick) {

				if(tystream->last_tick - first_tick <= 4 * tystream->med_ticks) {
					/* We are okay - educated guess at the moment */
					okay = 1;
					tystream->cons_junk_chunk=0;

					/* We need to free any junk chunk buffers */
					if(tystream->junk_chunks != NULL) {
						free_junk_chunks(tystream->junk_chunks);
						tystream->junk_chunks = NULL;
					}

				} else if (tystream->last_tick - first_tick <= 30 * tystream->med_ticks) {
					/* We are kind of okay but there probably is a gap
					that we need to fix flag the gap flag */
					LOG_WARNING1("Check Chunk: Warning!! Chunk: " I64FORMAT " - hole in video stream\n", chunk->chunk_number);
					LOG_WARNING2("Check Chunk: first_tick " I64FORMAT ", last_tick " I64FORMAT "\n", first_tick, tystream->last_tick);
					okay = 1;
					tystream->cons_junk_chunk=0;
					chunk->small_gap=1;

					/* We need to free any junk chunk buffers */
					if(tystream->junk_chunks != NULL) {
						free_junk_chunks(tystream->junk_chunks);
						tystream->junk_chunks = NULL;
					}

				} else {
					if(tystream->cons_junk_chunk) {
						LOG_WARNING1("Check Chunk: OOS chunk - " I64FORMAT " chunk(s) in a row skiped \r", tystream->cons_junk_chunk);
					} else {
						LOG_WARNING2("Check Chunk: " I64FORMAT " - Out of Sync with minus " I64FORMAT " ticks - skipping\n",
							chunk->chunk_number, tystream->last_tick  - first_tick);
						LOG_WARNING2("Check Chunk: first_tick " I64FORMAT ", last_tick " I64FORMAT "\n", first_tick, tystream->last_tick);

					}
					tystream->cons_junk_chunk++;
					if(tystream->cons_junk_chunk == JUNK_CHUNK_BUFFER_SIZE) {

							/* Okay go into repair mode */
							if(tystream->junk_chunks->first_tick >= tystream->last_tick) {
								LOG_WARNING1("Check Chunk: Had big gap - " I64FORMAT " ticks -> and then every thing was bad\n"
								"Will try to revert and repair the TyStream better with a gap than nothig\n",
									tystream->junk_chunks->first_tick - tystream->last_tick);
							} else {
								LOG_WARNING1("Check Chunk: Had big gap - " I64FORMAT " ticks -> and then every thing was bad\n"
								"Will try to revert and repair the TyStream better with a gap than nothig\n",
									tystream->last_tick - tystream->junk_chunks->first_tick);
							}
							chunk->last_tick = tmp_video_time_array[nr_video -1];
							chunk->first_tick = first_tick;
							chunk->gap=1;
							chunk->junk=1;

							if(tmp_max_ticks != 0 && tmp_min_ticks != INITAL_MIN_TICKS ) {
								chunk->med_ticks = (tmp_max_ticks + tmp_min_ticks)/2;
								LOG_DEVDIAG2("Check Chunk: Junk with med_ticks " I64FORMAT " - chunk " I64FORMAT "\n", chunk->med_ticks, chunk->chunk_number);

							} else {
								chunk->med_ticks = STD_VIDEO_MED_TICK;
							}

							tystream->junk_chunks = add_chunk(tystream, chunk,tystream->junk_chunks);
							free(tmp_video_time_array);

							/* Lets do a final check of the chunks and return the resulting
							array of junk chunks */

							return(check_junk_chunks(tystream));

					}
				}
			} else if (first_tick == tystream->last_tick) {
				LOG_WARNING2("Check Chunk: Warning!! Chunk: " I64FORMAT " - last/fist is equal - " I64FORMAT "\n", chunk->chunk_number, tystream->last_tick);
				/* We set okay here even if things are funky
				will check later on since the gap is set */
				okay = 1;
				chunk->small_gap=1;
				tystream->cons_junk_chunk=0;

				/* We need to free any junk chunk buffers */
				if(tystream->junk_chunks != NULL) {
					free_junk_chunks(tystream->junk_chunks);
					tystream->junk_chunks = NULL;
				}

			} else {
				/* We set okay here even if things are funky
				will check later on since the gap is set */
				LOG_WARNING1("Check Chunk: Warning!! Chunk: " I64FORMAT " - this is not supposed to happen\n", chunk->chunk_number);
				okay = 1;
				chunk->small_gap=1;
				tystream->cons_junk_chunk=0;

				/* We need to free any junk chunk buffers */
				if(tystream->junk_chunks != NULL) {
					free_junk_chunks(tystream->junk_chunks);
					tystream->junk_chunks = NULL;
				}

			}

		} else {
			/**
			 * FIXME
			 * Okay first chunk to check we assume it's okay
			 * Will definitly need some check to see if all chunks
			 * after this one error out - i.e. the first chunk is bad
			*/
			okay = 1;
			tystream->cons_junk_chunk=0;

			/* We need to free any junk chunk buffers */
			if(tystream->junk_chunks != NULL) {
				free_junk_chunks(tystream->junk_chunks);
				tystream->junk_chunks = NULL;
			}

		}

		if(okay) {

			tystream->last_tick = tmp_video_time_array[nr_video -1];
			chunk->last_tick = tystream->last_tick;
			chunk->first_tick = first_tick;

			free(tmp_video_time_array);

			if(tmp_max_ticks != 0 && tmp_min_ticks != INITAL_MIN_TICKS ) {
				tystream->med_ticks = (tmp_max_ticks + tmp_min_ticks)/2;
				chunk->med_ticks = tystream->med_ticks;
					LOG_DEVDIAG2("Check Chunk: med_ticks " I64FORMAT " - chunk " I64FORMAT "\n", chunk->med_ticks, chunk->chunk_number);
			} else {
				chunk->med_ticks = STD_VIDEO_MED_TICK;
			}
			return(chunk);
		} else {
			/* Okay so we have a bad chunk or at least we think we have
			 * This can very well be a very big hole in the stream and every
			 * chunk after this one is actually in sync.
			 *
			 * To come around that problem we will store all junk chunks
			 * until we have JUNK_CHUNK_BUFFER_SIZE junk chunks in a row
			 * - then we will check
			 * if those junk chunks are in sync or not. If they are we
			 * most probably had a very big whole (time gap) in the stream.
			 *
			 * This can often happen when there is a switch of program within
			 * the same channel e.g. we shift from "Six feet under" preview
			 * to the real show we are want to see. Hence it's kind of a bummer
			 * if tydemux skipps all the fun and only deal with the preview.
			 *
			 * NOTE. this will most probably result in out of sync video/audio
			 * hence we need to deal with this later on when we do the internal
			 * muxing
			 */
			chunk->last_tick = tmp_video_time_array[nr_video -1];
			chunk->first_tick = first_tick;
			chunk->gap=1;
			chunk->junk=1;

			if(tmp_max_ticks != 0 && tmp_min_ticks != INITAL_MIN_TICKS ) {
				chunk->med_ticks = (tmp_max_ticks + tmp_min_ticks)/2;
				LOG_DEVDIAG2("Check Chunk: Junk with med_ticks " I64FORMAT " - chunk " I64FORMAT "\n", chunk->med_ticks, chunk->chunk_number);
			} else {
				chunk->med_ticks = STD_VIDEO_MED_TICK;
			}

			if(tystream->junk_chunks != NULL) {
				tystream->junk_chunks = add_chunk(tystream, chunk, tystream->junk_chunks);
			} else {
				tystream->junk_chunks = chunk;
			}

			free(tmp_video_time_array);

			//LOG_USERDIAG1("Check Chunk: " I64FORMAT " Out of sync - skipping\n", chunk->chunk_number);
			return(0);
		}

	} else {

		/* No video packets definitly need to check the
		sync later on flaging gap */

		chunk->last_tick = 0;
		chunk->first_tick = 0;
		chunk->small_gap=1;
		chunk->med_ticks=STD_VIDEO_MED_TICK;
		tystream->cons_junk_chunk=0;

		/* We need to free any junk chunk buffers */
		if(tystream->junk_chunks != NULL) {
			free_junk_chunks(tystream->junk_chunks);
			tystream->junk_chunks = NULL;
		}
		/* FIXME WE NEED TO DO A AUDIO CHECK */

		LOG_WARNING1("Chunk Chunk: Warning!! chunk " I64FORMAT " No video records - assume it's not okay - skipping\n", chunk->chunk_number);
		/* FIXME */
		free_chunk(chunk);
		//return(chunk);
		return(0);
	}

}

/********************************************************************************************/
static void bsort(ticks_t video_time_array[], int size) {

	ticks_t tmp;
	int i, j;

	for(i=0; i < size-1; i++) {
		for(j=i+1; j < size; j++) {
			if(video_time_array[i] > video_time_array[j]) {
				tmp = video_time_array[i];
				video_time_array[i] = video_time_array[j];
				video_time_array[j] = tmp;
			}
		}
	}
}

