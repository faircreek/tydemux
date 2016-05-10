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

/**
 * The problem with a junk chunk buffer is that we can have real OOS chunks in it, hence
 * we will need to check if sync here to :(. It's however much easier since we have stored
 * the sync data in the chunk it self.
 *
 * We will also need to check for big holes i.e. we have sync, sync, sync, no sync,
 * no sync as we hab when we first check the chunk in check_chunk
 *
 * We solve both those problems by doing a check for sync between three chunks if
 * chunk 1 and 2 is out of sync but chunk 2 and chunk 3 is in sync then we have a
 * hole if chunk 1 and 2 is out of sync but chunk 1 and 3 is in sync then we have
 * a OSS chunk i.e. a real junk chunk. Hmm better make it more generic if TIVO want to
 * break this they just throw in two or more junk chunks.
 * We have a slight problem if the last chunk is out of sync is it a OSS a hole or what?
 * need to think about this one - read a extra chunk?? - NOPE just don't null the
 * junk_chunk buffer and keep the cons junk counter to 1.
 */


/* This fuction is far from good but I'm to dead to think up something else it will
simply fail if we have two or more OSS chunks in a row that individually is out
of sync*/

/* Count of real OSS chunks droped in check_junk */
static int droped;

/* Internal functions */
static int check_junk(chunk_t * chunk);


chunk_t * check_junk_chunks(tystream_holder_t * tystream) {


	/* Tempory chunks in our test */
	chunk_t * pre_chunk;
	chunk_t * chunks;
	chunk_t * return_chunk;


	/* Markers and counters */
	int out_of_sync;
	int okay;
	int hole;
	int i;

	/* Times */
	ticks_t diff;

	/* Diff indicator */
	indicator_t indicator;

	/* Init */
	out_of_sync = 0;
	okay = 0;
	hole = 0;
	i = 0;

	/* Drop count */
	droped = 0;

	/* No use to test the first one */
	chunks = tystream->junk_chunks;
	chunks = chunks->next;

	/* Basically the same algo as in check chunk but with stored values
	and a extra check_junk func if we find "OSS" chunks */

	while(1) {
		pre_chunk = chunks->previous;

		LOG_DEVDIAG3("First tick " I64FORMAT " Last Tick " I64FORMAT ", Med Ticks " I64FORMAT "\n",
				chunks->first_tick, pre_chunk->last_tick, pre_chunk->med_ticks);

		diff = llabs(chunks->first_tick - pre_chunk->last_tick);
		if(chunks->first_tick >= pre_chunk->last_tick) {
			indicator = POSITIVE;
		} else {
			indicator = NEGATIVE;
		}


		if(diff <= 4 * pre_chunk->med_ticks) {
			chunks->gap = 0;
			okay++;
		} else if (diff <= 10 * pre_chunk->med_ticks) {
			hole++;
			chunks->gap=1;
			chunks->indicator = indicator;
		} else {
			if(!check_junk(chunks) || !chunks->next) {
				out_of_sync++;
			}
			LOG_DEVDIAG4("X - %i First tick " I64FORMAT " Last Tick " I64FORMAT ", Med Ticks " I64FORMAT "\n",
					 i, chunks->first_tick, pre_chunk->last_tick, pre_chunk->med_ticks);
		}

		i++;

		if (chunks->next) {
			chunks = chunks->next;
		} else {
			break;
		}

		if (!chunks->next) {
			/* We're about to parse the last chunk
			If out_of_sync is 1 or more here we are basically
			out of luck trying to repair this tyStream :(
			FIXME need to give some more info audio delay etc...*/
			if(out_of_sync) {
				LOG_FATAL1("ERROR: No way of reparing this TyStream - aborting - %i unrepairable chunks found\n", out_of_sync);
				exit(1);
			}
		}
	}


	if(out_of_sync) {
		/* The last chunk was out of sync :( */
		tystream->cons_junk_chunk = 1;

		/* Turncate the junk chunk buffer */
		chunks->previous->next = NULL;
		chunks->previous = NULL;
	} else  {
		tystream->cons_junk_chunk = 0;

		pre_chunk = chunks;
		chunks    = NULL;
	}

	return_chunk = tystream->junk_chunks;
	tystream->junk_chunks = chunks;


	/* Set med ticks and last tick */
	tystream->med_ticks = pre_chunk->med_ticks;
	tystream->last_tick = pre_chunk->last_tick;
	//print_chunks(return_chunk);
	return(return_chunk);

}




/******************************************************************************************/

static int check_junk(chunk_t * chunk) {

	/* Chunks */
	chunk_t * pre_chunk;

	/* Times */
	ticks_t diff;

	/* Diff indicator */
	indicator_t indicator, indicator_2;

	/* Init */
	pre_chunk = chunk->previous;

	diff = llabs(chunk->next->first_tick - chunk->previous->last_tick);

	if(chunk->next->first_tick >= chunk->previous->last_tick) {
		indicator = POSITIVE;
	} else {
		indicator = NEGATIVE;
	}

	/* First check if we get sync in the next chunk
	i.e. if we have a real OSS chunk in our list of
	junk chunks */

	if (diff <= 10 * chunk->previous->med_ticks) {
		if(ty_debug >= 2) {
			LOG_USERDIAG("We had a OSS\n");
		}
		pre_chunk->gap=0;
		if (diff <= 4 * chunk->previous->med_ticks) {
			chunk->next->gap=0;
		} else {
			chunk->next->gap=1;
			chunk->next->indicator = indicator;
		}
		pre_chunk->next = chunk->next;
		chunk->next->previous = pre_chunk;
		free_chunk(chunk);
		droped++;
		return(1);
	} else {
		/* Now test if we have a big hole i.e. we have
		 sync in the next chunk */

		if(pre_chunk->last_tick <= chunk->first_tick) {
			indicator = POSITIVE;
		} else {
			indicator = NEGATIVE;
		}

		diff = llabs(chunk->next->first_tick - chunk->last_tick);
		if (chunk->next->first_tick > chunk->last_tick) {
			indicator_2 = POSITIVE;
		} else {
			indicator_2 = NEGATIVE;
		}


		if (diff <= 10 * chunk->med_ticks) {

			chunk->gap=1;
			chunk->indicator = indicator;

			if(diff <= 4 * chunk->med_ticks) {
				chunk->next->gap=0;
			} else {
				chunk->next->gap=1;
				chunk->next->indicator = indicator_2;
			}
			return(1);

		} else {

			/* Out of luck - not a OSS chunk and we
			don't find sync in the next chunk :( */
			return(0);
		}
	}
}

