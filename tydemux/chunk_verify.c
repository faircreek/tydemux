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

int check_audio_sync(const chunk_t * present_chunk, const chunk_t * check_chunk,
	int seek_backwards, int seek_forward, tystream_holder_t * tystream) {


	/* Number of pes records */
	int nr_pes_records;
	int nr_pes_records_next;

	/* Chunks to play with */
	chunk_t * tmp_present_chunk;
	chunk_t * tmp_check_chunk;

	/* Time values */
	ticks_t test_time;
	ticks_t next_time;
#if 0
	ticks_t final_time;
#endif
	ticks_t diff;

	/* init */

	/* First check if we have any audio pes headers
	and how many we have */

	nr_pes_records = get_nr_audio_pes(tystream, present_chunk);

	if(!nr_pes_records && seek_backwards) {
		/* Lets try to find some in the previous chunk
		Nope no "recursion" backwards we will just keep
		one previous chunk */

		if(present_chunk->previous != NULL) {
			nr_pes_records = get_nr_audio_pes(tystream, present_chunk->previous);
			tmp_present_chunk = present_chunk->previous;
		} else {
			/* Give up and send a error */
			return(-1);
		}
	} else {
		tmp_present_chunk = (chunk_t *)present_chunk;
	}

	if (!nr_pes_records) {
		return(-1);
	}

	tmp_check_chunk = (chunk_t *)check_chunk;

	nr_pes_records_next = get_nr_audio_pes(tystream, tmp_check_chunk);

	if(!nr_pes_records_next && seek_forward ) {
		tmp_check_chunk = tmp_check_chunk->next;
		while(tmp_check_chunk) {
			if((nr_pes_records_next = get_nr_audio_pes(tystream, tmp_check_chunk))) {
				break;
			}
			tmp_check_chunk = tmp_check_chunk->next;
		}
	}

	if(!nr_pes_records_next) {
		/* error out */
		return(-1);
	}


	/* Now find our first pes time in the test chunk */
	test_time = get_time_X(0, tmp_present_chunk, AUDIO,tystream );
	next_time = get_time_X(0, tmp_check_chunk, AUDIO, tystream);
	if (!test_time || !next_time) {
		return(-1);
	}

#if 0
	if(!(test_time = get_time_X(0, tmp_present_chunk, AUDIO,tystream ))) {
		/* We didn't get the first time error out */
		return(-1);
	}

	/* Now get the next time */

	if(!(next_time = get_time_X(0, tmp_check_chunk, AUDIO, tystream))) {
		/* We didn't get the first time error out */
		return(-1);
	}
#endif

	diff = llabs(test_time + (tystream->audio_median_tick_diff * nr_pes_records) - next_time);

	if(diff <= (tystream->audio_median_tick_diff/10)) {
		/* No gap */
		return(0);
	} else {

		if(ty_debug >= 3) {
			LOG_USERDIAG2("Check Gap: Gap detected - " I64FORMAT ".%03lu sec diff" \
			       "- will try to repair\n",diff/PTS_CLOCK, \
			       (unsigned long)((diff%PTS_CLOCK)/(PTS_CLOCK/1000)));
		}
		return(1);

	}



#if 0
	final_time = test_time + (tystream->audio_median_tick_diff * nr_pes_records);

	if(final_time >= next_time) {
		if((final_time - next_time) <= (tystream->audio_median_tick_diff/10)) {
			/* No gap */
			return(0);
		} else {
			if(ty_debug >= 1) {
				/* We are just using test time here */
				test_time = final_time - next_time;
				LOG_WARNING2("Check Gap: Gap detected - " I64FORMAT ".%03lu sec diff" \
				       "- will try to repair\n",test_time/PTS_CLOCK, \
				       (unsigned long)(test_time%PTS_CLOCK)/(PTS_CLOCK/1000)));
			}
			return(1);
		}
	} else {
		if((next_time - final_time) <= (tystream->audio_median_tick_diff/10)) {
			/* no gap */
			return(0);
		} else {
			if(ty_debug >= 1) {
				/* We are just using test time here */
				test_time = next_time - final_time ;
				LOG_WARNING2("Check Gap: Gap detected - " I64FORMAT ".%03lu sec diff" \
				       "- will try to repair\n",test_time/PTS_CLOCK, \
				       (unsigned long)((test_time%PTS_CLOCK)/(PTS_CLOCK/1000)));
			}
			return(1);
		}
	}
#endif
}
 
