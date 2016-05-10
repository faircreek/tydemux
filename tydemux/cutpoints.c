/*
 * Copyright (C) 2002, 2003, 2015  John Barrett <johnbarretthcc@computerc.com>
 *                     2003, 2015  Olaf Beck    <olaf_sc@yahoo.com>
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

/* cutpoints.c
 * Functions to maintain cut points
*/

#include "common.h"
#include "global.h"

#define MILLISECS_PER_SEC 1000L
#define MILLISECS_PER_MIN ( MILLISECS_PER_SEC * 60L )
#define MILLISECS_PER_HOUR ( MILLISECS_PER_MIN * 60L )

#define TICKS_PER_MILLISECOND 90
#define MILLISECS_TO_STREAMTIME( ms ) ( ms * TICKS_PER_MILLISECOND )

#define FILE_READ_BUFFER_SIZE 1024



/* Release memory used by the cutpoints
 * Call this to reset the cutpoints and when exiting the program
*/

cutpoint_t * cutpoint_new() {

	cutpoint_t * cutpoint;

	cutpoint = (cutpoint_t *)malloc(sizeof(cutpoint_t));
	memset(cutpoint, 0,sizeof(cutpoint_t));

	cutpoint->cutstart = -1;
	cutpoint->cutstop = -1;

	cutpoint->chunkstart = -1;
	cutpoint->rec_nr_start = -1;

	cutpoint->chunkstop = -1;
	cutpoint->rec_nr_stop = -1;

	cutpoint->cut_is_start_of_stream  = 0;
	cutpoint->cut_is_end_of_stream = 0;

	cutpoint->cuttype = CUTPOINT_NONE;

	cutpoint->audiostart = 0;
	cutpoint->audiostop = 0;


	return(cutpoint);

}

void cutpoint_free_all(tystream_holder_t * tystream)
{
	int n;
	cutpointptr_t *cut_array;
	int number_of_elements;

	number_of_elements = tystream->cuts->nr_of_cutpoints;
	cut_array = tystream->cuts->cut_array;

	for ( n = 0; n < number_of_elements; ++n ) {
		free( cut_array[n] );
	}

	if ( cut_array != NULL ) {
		free( cut_array );
	}

	if( tystream->cuts ) {
		free( tystream->cuts );
		tystream->cuts = NULL;
	}
}

int cutpoint_add_cut_to_array (tystream_holder_t * tystream, cutpoint_t * cutpoint )
{
	cutpointptr_t  * cut_array;
	int nr_of_cutpoints;

	if( !tystream->cuts ) {
		tystream->cuts = (cuts_t *) malloc( sizeof( cuts_t ) );
		memset( tystream->cuts, 0, sizeof( cuts_t ) );
	}

	nr_of_cutpoints = tystream->cuts->nr_of_cutpoints;
	cut_array = tystream->cuts->cut_array;

	/* Resize the cut entry list after every 64 entries */
	if ( !cut_array ) {
		cut_array = malloc( sizeof( cutpointptr_t ) * 64 );
		memset(cut_array,0, sizeof( cutpointptr_t ) * 64);
	} else if ( ( nr_of_cutpoints%64 ) == 0 ) {
		int newSize = sizeof( cutpointptr_t ) * 64;
		newSize *= ( ( nr_of_cutpoints%64 ) + 1);
		cut_array = realloc( cut_array, newSize );
	}


	/* Save this cut in the array of all cuts */
	cut_array[nr_of_cutpoints] = cutpoint;

	nr_of_cutpoints++;

	tystream->cuts->nr_of_cutpoints = nr_of_cutpoints;
	tystream->cuts->cut_array = cut_array;

	return(1);
}

int cutpoint_add_cut(tystream_holder_t * tystream, const gop_index_t * start_gop_index,
	const gop_index_t * stop_gop_index, ecut_t cuttype) {


	cutpoint_t * cutpoint;

	/* Create cut point */
	cutpoint = cutpoint_new();

	/* Set cutpoint values */
	cutpoint->cutstart = start_gop_index->time_of_iframe;
	cutpoint->cutstop = stop_gop_index->time_of_iframe;;

	cutpoint->chunkstart = start_gop_index->chunk_number_seq;
	cutpoint->rec_nr_start = start_gop_index->seq_rec_nr;

	cutpoint->chunkstop = stop_gop_index->chunk_number_seq;
	cutpoint->rec_nr_stop = stop_gop_index->seq_rec_nr;

	/* See if this is a end or start cut */
	if(cutpoint->chunkstart <= 10) {
		cutpoint->cut_is_start_of_stream  = 1;
	}

	if(tystream->index->last_gop_index->chunk_number_seq - stop_gop_index->chunk_number_seq <= 10) {
		cutpoint->cut_is_end_of_stream = 1;
	}

	cutpoint->cuttype = cuttype;
#if 0
	printf("Adding cutpoint - memory \n"
		"Cut start: chunk " I64FORMAT " - Record  %i - time " I64FORMAT " \n"
		"Cut stop:  chunk " I64FORMAT " - Record  %i - time " I64FORMAT " \n"
		"Start stream: %i Stop stream: %i\n",
		cutpoint->chunkstart, cutpoint->rec_nr_start, cutpoint->cutstart,
		cutpoint->chunkstop, cutpoint->rec_nr_stop, cutpoint->cutstop,
		cutpoint->cut_is_start_of_stream, cutpoint->cut_is_end_of_stream);
#endif

	return( cutpoint_add_cut_to_array( tystream, cutpoint ) );
}

/* Create cut from a tyeditor cut string - this contains the seq and
 * chunk info as well as the cut time
 * returns - 0 on success, error code on failure
 */
int cutpoint_create_from_tyeditor_string( tystream_holder_t * tystream, const char *cutstring )
{
	cutpoint_t * cutpoint;
	int64_t value;

	/* Skip until we see the @ marking the start of the cut string */
	while( *cutstring && *cutstring != '@' ) {  /* Identifier */
		++cutstring;
	}
	if( *cutstring == '@' ) {
		++cutstring; /* Skip past the start marker */
	}

	if( !isdigit(*cutstring ) ) {
		return -1;
	}

	/* Create cut point */
	cutpoint = cutpoint_new();

	/* looks like a cut time line - no error checking as the cut file is produced by a program */
	sscanf( cutstring, I64FORMAT, &value );
	cutpoint->cutstart = value;
	cutstring += 11;
	sscanf( cutstring, I64FORMAT, &value );
	cutpoint->chunkstart = value;
	cutstring += 11;
	sscanf( cutstring, I64FORMAT, &value );
	cutpoint->rec_nr_start = (int)value;
	cutstring += 12;

	/* Skip leading white space  */
	while( isspace( *cutstring ) ) {
		++cutstring;
	}

	if( *cutstring != '@' ) {  /* Identifier */
		return -1;
	}
	++cutstring;

	if( !isdigit(*cutstring ) ) {
		return -1;
	}

	sscanf( cutstring, I64FORMAT, &value );
	cutpoint->cutstop = value;
	cutstring += 11;
	sscanf( cutstring, I64FORMAT, &value );
	cutpoint->chunkstop = value;
	cutstring += 11;
	sscanf( cutstring, I64FORMAT, &value );
	cutpoint->rec_nr_stop = (int)value;
	cutstring += 12;

	/* Skip leading white space  */
	while( isspace( *cutstring ) ) {
		++cutstring;
	}
	if( strncasecmp( cutstring, "remove", 6 ) == 0 ) {
		cutpoint->cuttype = CUTTYPE_REMOVE;
	} else if (strncasecmp( cutstring, "system", 6 ) == 0 ) {
		cutpoint->cuttype = CUTTYPE_SYSTEM;
	} else {
		cutpoint->cuttype = CUTTYPE_CHAPTER;
	}


	/* See if this is a end or start cut */
	if(cutpoint->chunkstart <= 10) {
		cutpoint->cut_is_start_of_stream  = 1;
	}
#if 0
	printf("Adding cutpoint - file\n"
		"Cut start: chunk " I64FORMAT " - Record  %i - time " I64FORMAT " \n"
		"Cut stop:  chunk " I64FORMAT " - Record  %i - time " I64FORMAT " \n"
		"Start stream: %i Stop stream: %i\n",
		cutpoint->chunkstart, cutpoint->rec_nr_start, cutpoint->cutstart,
		cutpoint->chunkstop, cutpoint->rec_nr_stop, cutpoint->cutstop,
		cutpoint->cut_is_start_of_stream, cutpoint->cut_is_end_of_stream);
#endif


	/* OLAF - a way of finding if this is an end of stream cutpount if no index is loaded - hmm thats a good one we need to add a extra value*/

	return( cutpoint_add_cut_to_array( tystream, cutpoint ) );
}

/* Read a cut file produced by jdiner's gopeditor program
 * inputs - full path to file
 * returns - 0 on success, error code on failure
 */
int cutpoint_read_gopeditor_file( tystream_holder_t * tystream, const char *szfilename )
{
	int nerror;
	int nchar;
	char sznextline[ FILE_READ_BUFFER_SIZE ];


	FILE *pfile;
	//printf("File name is %s\n", szfilename);
	nerror = 0;

	pfile = fopen( szfilename, READ_MODE );
	if( pfile == NULL ) {
		return(-1);
	}

	while( !nerror && fgets( sznextline, FILE_READ_BUFFER_SIZE, pfile ) != NULL ) {
		nchar = 0;
		cutpoint_create_from_tyeditor_string(tystream, sznextline);

	}

	fclose( pfile );


	return nerror;
}



#if 0
/* Read a cut file produced by jdiner's gopeditor program
 * inputs - full path to file
 * returns - 0 on success, error code on failure
 */
int cutpoint_read_gopeditor_file( tystream_holder_t * tystream, const char *szfilename )
{
	int nerror;
	int nchar;
	char sznextline[ FILE_READ_BUFFER_SIZE ];
	FILE *pfile;

	ticks_t time_start;
	ticks_t time_end;

	nerror = 0;

	pfile = fopen( szfilename, READ_MODE );
	if( pfile == NULL ) {
		return(-1);
	}

	while( !nerror && fgets( sznextline, FILE_READ_BUFFER_SIZE, pfile ) != NULL ) {
		nchar = 0;

		/* Skip leading white space  */
		while( isspace( sznextline[nchar] ) ) {
			++nchar;
		}

		/* only bother with lines that have a number as the first non-space character  */
		if( isdigit( sznextline[nchar] ) ) {
			/* looks like a cut time line - no error checking as the cut file is produced by a program */
			char *szstart, *szend;

			szstart = &( sznextline[nchar] );
			szend = &( sznextline[nchar + 15] );
			szstart[12] = 0;
			szend[12] = 0;

			time_start = cutpoint_convert_time_to_ticks(szstart);
			time_end = cutpoint_convert_time_to_ticks(szend);
			nerror = cut_point_add_ticks(tystream, time_start, time_end, CUTTYPE_REMOVE);

		}
	}

	fclose( pfile );


	return nerror;
}
#endif
/* Add additional cut points
 * inputs - Time ranges in the format timestring-timestring
 *               timestring can be any one of hh:mm:ss.mmm
 *											  hh:mm:ss
 *											  hh:mm
 *											  mm
 *			Type of cut point
 * returns - 1 on success, error code on failure
 */
int cutpoint_add_manual_time_cut( tystream_holder_t * tystream, const char *sztime_range, ecut_t cuttype)
{
	char szstart[ 15 ];
	char szend[ 15 ];
	int nin;
	int nout;

	ticks_t time_start;
	ticks_t time_end;

	nin = nout = 0;

	/* Extract the start time */
	while( isdigit( sztime_range[nin] ) || sztime_range[nin] == ':' || sztime_range[nin] == '.' ) {
		if( nout > 12 ){
			return(-1);  /* Must be bad format */
		}
		szstart[nout++] = sztime_range[nin++];
	}
	szstart[nout] = 0;

	/* The next character should be a '-' */
	nout = 0;
	if( sztime_range[nin] != '-' ) {
		return(-1);  /* Invalid range */
	}

	/* Extract the end time */
	++nin;
	while( isdigit( sztime_range[nin] ) || sztime_range[nin] == ':' || sztime_range[nin] == '.' ) {
		if( nout > 12 ) {
			return(-1);  /* Must be bad format */
		}
		szend[nout++] = sztime_range[nin++];
	}
	szend[nout] = 0;

	time_start = cutpoint_convert_time_to_ticks(szstart);
	time_end = cutpoint_convert_time_to_ticks(szend);
	return(cut_point_add_ticks(tystream, time_start, time_end, cuttype));

}


int cut_point_add_ticks(tystream_holder_t * tystream, ticks_t time_start, ticks_t time_end, ecut_t cuttype) {

	gop_index_t * gop_index_start;
	gop_index_t * gop_index_stop;


	if(time_end <= time_start) {
		return(0);
	}

	gop_index_start = cutpoint_find_gop_index(tystream, time_start);

	gop_index_stop = cutpoint_find_gop_index(tystream, time_end);

        return(cutpoint_add_cut(tystream, gop_index_start, gop_index_stop, cuttype));

}



gop_index_t * cutpoint_find_gop_index(tystream_holder_t * tystream, ticks_t time) {


	/* Hmm we should really sort the way we seek gop_index so we
	can just take the last one we was seeking and then search from
	that point */

	gop_index_t * gop_index;

	ticks_t stream_start_time;
	int gotit;

	gop_index = tystream->index->gop_index;

	/* Find start time of stream so we can offset with that */

	stream_start_time = tystream->index->gop_index->time_of_iframe;

	time = time + stream_start_time;
	gotit = 0;
	while(gop_index) {
		if(gop_index->time_of_iframe >= time) {
			gotit = 1;
			break;
		}
		gop_index = gop_index->next;
	}

	if(gotit) {
		return(gop_index);
	} else {
		return(tystream->index->last_gop_index);
	}

}



ticks_t cutpoint_convert_time_to_ticks( const char *szstartTime) {

	unsigned long ulstart;
	int nstartnumnums, nstarthour, nstartmin, nstartsec, nstartmsec;


	nstartnumnums = nstarthour = nstartmin = nstartsec = nstartmsec = 0;

	/* Extract the start times from the string */
	nstartnumnums = sscanf( szstartTime, "%d:%d:%d.%d", &nstarthour, &nstartmin, &nstartsec, &nstartmsec );

	if( nstartnumnums == 0 ) {
		return(-1);
	}

	if( nstartnumnums == 1 ) { /* Only minutes given */
		nstartmin = nstarthour;
		nstarthour = 0;
	}



	/* Calculate times in milliseconds */
	ulstart = ( nstarthour * MILLISECS_PER_HOUR ) +
			  ( nstartmin * MILLISECS_PER_MIN ) +
			  ( nstartsec * MILLISECS_PER_SEC ) +
				nstartmsec;


	return(MILLISECS_TO_STREAMTIME( ulstart ));
}



/* Hmm this would be prerect if we had C++ things in C:) */



int cutpoint_incut_chunk(tystream_holder_t * tystream, chunknumber_t  chunk_number) {

	int i;


	int nr_of_cutpoints;

	cutpointptr_t  * cut_array;

	if(!tystream->cuts) {
		return(0);
	}

	nr_of_cutpoints = tystream->cuts->nr_of_cutpoints;
	cut_array = tystream->cuts->cut_array;

	/*printf("Chunk nr: " I64FORMAT "\n", chunk_number);

	for(i=0; i < nr_of_cutpoints; i++) {
		printf("cutpoint\n"
			"Cut start: chunk " I64FORMAT " - Record  %i - time " I64FORMAT " \n"
			"Cut stop:  chunk " I64FORMAT " - Record  %i - time " I64FORMAT " \n"
			"Start stream: %i Stop stream: %i\n",
			cut_array[i]->chunkstart, cut_array[i]->rec_nr_start, cut_array[i]->cutstart,
			cut_array[i]->chunkstop, cut_array[i]->rec_nr_stop, cut_array[i]->cutstop,
			cut_array[i]->cut_is_start_of_stream, cut_array[i]->cut_is_end_of_stream);
	}
	*/

	for(i=0; i < nr_of_cutpoints; i++) {

		/* The chunk is in the start of the steam delay init  - dump the chunk */
		if (cut_array[i]->cut_is_start_of_stream && chunk_number <= cut_array[i]->chunkstop) {

			/* However we need to star one chunk before to init the stream */
			if(cut_array[i]->chunkstop == 0 || chunk_number == cut_array[i]->chunkstop - 1) {
				//printf("Return 0\n");
				return(0);
			} else {
				//printf("Return 4\n");
				return(4);

			}

		/* this is the end so just dump it */
		} else if (cut_array[i]->cut_is_end_of_stream && chunk_number > cut_array[i]->chunkstart) {
			//printf("Return 5\n");
			return(5);

		} else if(cut_array[i]->chunkstart < chunk_number &&
			cut_array[i]->chunkstop > chunk_number) {

			/* This is in the middle of a cut so we don't need to parse video */
			//printf("Return 3\n");
			return(3);

		} else if (cut_array[i]->chunkstart == chunk_number) {
			/* We start the cut in this chunk hence we need to parse video up to record X */
			//printf("Return 1\n");
			return(1);

		} else if (cut_array[i]->chunkstop == chunk_number) {
			/* We stop the cut in this chunk hence we need to parse video from record X and on wards*/
			//printf("Return 2\n");
			return(2);
		}
	}

	/* Normal parsing */
	//printf("Returning at end 0\n");
	return(0);


}


int cutpoint_incut_chunk_record(tystream_holder_t * tystream, chunk_t * chunk) {

	int i;

	chunknumber_t chunk_number;

	int nr_of_cutpoints;

	cutpointptr_t  * cut_array;

	if(!tystream->cuts) {
		return(-1);
	}


	chunk_number = chunk->chunk_number;
	nr_of_cutpoints = tystream->cuts->nr_of_cutpoints;
	cut_array = tystream->cuts->cut_array;

	for(i=0; i < nr_of_cutpoints; i++) {
		if(cut_array[i]->chunkstart == chunk_number) {
			return(cut_array[i]->rec_nr_start);
		}

		if(cut_array[i]->chunkstop == chunk_number) {
			return(cut_array[i]->rec_nr_stop);
		}
	}

	return(-1);

}

#if 0
int cutpoint_incut_ticks(tystream_holder_t * tystream, ticks_t ticks) {

	int i;

	int nr_of_cutpoints;

	cutpointptr_t  * cut_array;

	if(!tystream->cuts) {
		return(0);
	}

	nr_of_cutpoints = tystream->cuts->nr_of_cutpoints;

	cut_array = tystream->cuts->cut_array;

	for(i=0; i < nr_of_cutpoints; i++) {
		if(cut_array[i]->cutstart <= ticks &&
			cut_array[i]->cutstop > ticks) {
			return(1);

		}
	}

	return(0);

}
#endif

ticks_t cutpoint_incut_ticks_cutpoint(tystream_holder_t * tystream, ticks_t ticks) {

	int i;

	int nr_of_cutpoints;

	cutpointptr_t  * cut_array;

	if(!tystream->cuts) {
		return(0);
	}

	nr_of_cutpoints = tystream->cuts->nr_of_cutpoints;

	cut_array = tystream->cuts->cut_array;

	//printf("Ticks is " I64FORMAT "\n", ticks);

	for(i=0; i < nr_of_cutpoints; i++) {
		if(cut_array[i]->cutstart <= ticks &&
			cut_array[i]->cutstop > ticks) {
			//printf("In start of a cut\n");
			if(cut_array[i]->cut_is_start_of_stream) {
				/* Okay we should not do anything if
				we are about to start the stream
				if will be done in init */
				return(0);
			}
			if(cut_array[i]->audiostart == 0) {
				cut_array[i]->audiostart = 1;
				return(cut_array[i]->cutstart);
			} else {
				return(-1);
			}
		}

		if(cut_array[i]->cutstop < ticks && cut_array[i]->audiostop == 0 && cut_array[i]->audiostart) {
			/* FIXME - Fist audio outside the cut and we are not subject to gaps ??*/
			//printf("In end cut\n");
			cut_array[i]->audiostop = 1;
			return(cut_array[i]->cutstop);
		}

	}
	//printf("Returning zero \n");

	return(0);

}
