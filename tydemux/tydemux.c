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
#include <tygetopt.h>

#if !defined(TIVO)
tystream_holder_t *probe_ty_stream( demux_start_params_t *demux_params )
{
	/* Probe */
	tystream_holder_t *tystream;
	vstream_t * vstream;

	tystream = new_tystream(DEMUX);

	/* We need a switch for tivo version in typrocess */
	tystream->tivo_version = V_2X;

	/* We are reading from a pipe :( */
	vstream = new_vstream();
	/* Lets fake it and do a read buffer */
	vstream->start_stream = (uint8_t *)malloc(sizeof(uint8_t) * CHUNK_SIZE * 200);
	memset(vstream->start_stream, 0,sizeof(uint8_t) * CHUNK_SIZE * 200);
	vstream->size = sizeof(uint8_t) * CHUNK_SIZE * 200;
	vstream->current_pos = vstream->start_stream;
	vstream->end_stream = vstream->start_stream + vstream->size;

	if(vstream->size != thread_pipe_peek(demux_params->input_pipe, (unsigned long)vstream->size,
		(char *)vstream->start_stream)) {

		/* Error in collecting chunks - stream to small ?? */
		LOG_WARNING("TyStream to small was not able to read 200 chunks");
		LOG_WARNING("We will now try to read a smaller amount");
		free_vstream(vstream);
		
		vstream = new_vstream();
		vstream->start_stream = (uint8_t *)malloc(sizeof(uint8_t) * CHUNK_SIZE * 50);
		memset(vstream->start_stream, 0,sizeof(uint8_t) * CHUNK_SIZE * 50);
		vstream->size = sizeof(uint8_t) * CHUNK_SIZE * 50;
		vstream->current_pos = vstream->start_stream;
		vstream->end_stream = vstream->start_stream + vstream->size;
		if(vstream->size != thread_pipe_peek(demux_params->input_pipe, (unsigned long)vstream->size,
			(char *)vstream->start_stream)) {
			LOG_ERROR("Error reading TyStream from stdin - the TyStream is to small");
			free_vstream(vstream);
			free_tystream(tystream);
			return(0);
		}
	}
	tystream->vstream = vstream;

	if(!std_probe_tystream(tystream)) {
		LOG_ERROR( "Failed in probe." );
		return(0);
	}

	if(tystream->miss_alinged) {
		LOG_WARN("\nYour stream is not aligned\n\n");
	}

	/* JOHN How to do - if a gui we most probably want to read the values
	directly from the tystream_holder_t */
	print_probe(tystream);

	if(!tystream->vstream) {
		tystream->in_file = -1;
		tystream->in_file_size = 0;
	} else {
		free_vstream(vstream);
		tystream->vstream=NULL;
	}

	return ( tystream );
}

#endif

#if !defined(TIVO)
int tydemux(tystream_holder_t * tystream, demux_start_params_t *params) {
#else
int tydemux(tystream_holder_t * tystream) {
#endif
	/* Iteration */
	int64_t ul;
	int64_t chunk_nr;
	int64_t lastchunk;

	if(tystream->std_alone && !tystream->remote_holder) {
		lastchunk = (tystream->in_file_size/CHUNK_SIZE) - tystream->start_chunk;
		for(ul=0, chunk_nr = 0; ul < lastchunk; ul++, chunk_nr++) {

			tystream->number_of_chunks = chunk_nr;

			old_prog_meter(tystream);

			get_chunk(tystream, chunk_nr, NULL);

			get_pes_holders(tystream);

			if(!tystream->find_seq) {
				tystream_init(tystream);
			}

			if(tystream->repair) {
				if(!repair_tystream(tystream)) {
					return(0);
				}
			}

			check_fix_pes_holder_video(tystream);

			write_pes_holders(tystream, NULL, NULL);

			/*if( params->progress ) {
				*(params->progress) = (int)( (ul*99)/lastchunk );
			}*/
		}
	} else if(tystream->remote_holder && tystream->std_alone) {
		lastchunk = tystream->nr_of_remote_chunks - (int64_t)1;
		for(ul=0, chunk_nr = 0; ul < lastchunk; ul++, chunk_nr++) {

			tystream->number_of_chunks = chunk_nr;

			old_prog_meter(tystream);

			get_chunk(tystream, chunk_nr, NULL);

			get_pes_holders(tystream);

			if(!tystream->find_seq) {
				tystream_init(tystream);
			}

			if(tystream->repair) {
				if(!repair_tystream(tystream)) {
					return(0);
				}
			}

			check_fix_pes_holder_video(tystream);

			write_pes_holders(tystream, NULL, NULL);

			/* No params in std alone mode */
			/*if( params->progress ) {
				*(params->progress) = (int)( (ul*99)/lastchunk );
			}*/

		}
	}
#if !defined(TIVO)
	else {
		int audio_offset_done = 0;

		chunk_nr = 0;

		LOG_USERDIAG("demuxing\n");

		/* Try to determine the total stream size for progress information */
		lastchunk = thread_pipe_streamsize(tystream->input_pipe)/CHUNK_SIZE;
		if( !lastchunk ) {
			lastchunk = tystream->nr_of_remote_chunks;
		}

		while(1) {

			tystream->number_of_chunks = chunk_nr;

			prog_meter(tystream);

			/* just add chunk progress here */
			if(get_chunk(tystream, chunk_nr, NULL) == -1) {
				/* End of file return */
				//printf("Breaking out of loop\n");
				return(1);
			}

			get_pes_holders(tystream);

			if(!tystream->find_seq) {
				tystream_init(tystream);
			}

			if( tystream->find_seq && !audio_offset_done ) {
				int offset = (int) (tystream->time_diff/90) * -1;
				LOG_USERDIAG1( "Audio offset (for mplex -O) is %d\r\n", offset );
				audio_offset_done = 1;

				if( params->wait_for_audio_offset ) {
					/* mplex will be blocked waiting for us to give it the audio offset */
					/* This was worked out in tystream_init so we can now pass it */
					*(params->audio_offset) = offset;
					*(params->wait_for_audio_offset) = 0;   /* Release mplex from waiting */
				}
			}

			if(tystream->repair) {
				if(!repair_tystream(tystream)) {
					LOG_FATAL("Unable to repair stream\n");
					exit(1);
				}
			}
			check_fix_pes_holder_video(tystream);
			/* just add video and audio progress here */
			write_pes_holders(tystream, NULL, NULL);
			chunk_nr++;

			if( params->progress ) {
				if( lastchunk ) {
					*(params->progress) = (int)( (chunk_nr*99)/lastchunk );
				} else {
					*(params->progress) = (int)(chunk_nr%99);
				}
			}
		}
	}
#endif
	return(1);
}


int parse_args(int argc, char *argv[], tystream_holder_t * tystream) {

	/* Args*/
	int flags;

	/* Switches */
//	char * tmp_audio_type = NULL;
	char * infile         = NULL;
	char * audio_out      = NULL;
	char * video_out      = NULL;
	int  tmp_tivo_ver   = -1;

	/* File handlers etc.*/
	off64_t byte_offset;

	/* Debug */
//	int debug = 1;

	/* Write or not */
	int write;
	int error;

	/* Init */
	write = 1;
	byte_offset = 0;
	error =0;
#if !defined(TIVO)
	tygetopt_unlock();
#endif
	while ((flags = getopt(argc, argv, "?hi:v:b:a:s:c:")) != -1) {
		switch (flags) {
		case 'i':
			if(optarg[0]=='-') {
				error++;
			} else {
				infile = optarg;
			}
			break;
		case 'a':
			if(optarg[0]=='-') {
				error++;
			} else {
				audio_out = optarg;
			}
			break;
		case 'v':
			if(optarg[0]=='-') {
				error++;
			} else {
				video_out = optarg;
			}
			break;
		case 'c':  /* Read cutpoint file */
			if(optarg[0]=='-') {
				error++;
			} else {
				if( cutpoint_read_gopeditor_file(tystream, optarg) != 0 ) {
					 error++;
				}
				//printf("added cutpoints\n");
			}
			break;
		case 's':
			if(optarg[0]=='-') {
				error++;
			} else {
				tmp_tivo_ver = atoi(optarg);
			}
			break;

		case 'b':
			if(optarg[0]=='-') {
				error++;
			} else {
				/* FIXME make a check that it's a digit */
				byte_offset = (off64_t)atoi(optarg);
			}
			break;
		case '?':
			error++;
			break;
		case 'h':
			error++;
			break;
		default:
			error++;
		}
	}
#if !defined(TIVO)
	tygetopt_unlock();
#endif

	if(error) {
		return(tydemux_usage());
	}

	if(tystream->std_alone) {

		if (infile == NULL) {
			return(tydemux_usage());
		}

		tystream->infile = infile;

		if(write != 0) {
			if (audio_out == NULL || video_out == NULL) {
				return(tydemux_usage());
			}
			tystream->audio_out = audio_out;
			tystream->video_out = video_out;
		}

		/* This one is allready set in the typrocess */
		if(tmp_tivo_ver == -1) {
			return(tydemux_usage());
		}


		if(tmp_tivo_ver < 1 || tmp_tivo_ver > 2) {
			return(tydemux_usage());
		} else {
			if (tmp_tivo_ver == 1) {
				LOG_ERROR("Tivo version 1.3/1.5 is not supported\n" \
					"Last version supporting 1.3/1.5 is tydemux 0.4.2\n");
				return(0);
			} else {
				tystream->tivo_version = V_2X;
			}
		}
		/* FIXME need to port to new logging !! need too check if digit */
		//set_debug_level(debug);
		set_debug_level(1);

		if(byte_offset) {
			tystream->byte_offset = byte_offset;
			//printf("Byteoffset " I64FORMAT "\n", byte_offset);
		}
	}

	return(1);

}

#if !defined(TIVO)

static mutex_handle_t process_lock = 0;
static pipe_handle_t awrite_pipe = 0;
static pipe_handle_t vwrite_pipe = 0;

/* Failsafe to ensure the caller is not permanently blocked if we exit prematurely */
void demux_exit_handler( void )
{
	if( awrite_pipe || vwrite_pipe || process_lock ) {
		LOG_PROGRESS("!!ERROR!!");
		LOG_FATAL("demux finished prematurely and did not exit gracefully\n");
	}

	if( awrite_pipe ) {
		thread_pipe_eof( awrite_pipe, 0 );
		awrite_pipe = 0;
	}
	if( vwrite_pipe ) {
		thread_pipe_eof( vwrite_pipe, 0 );
		vwrite_pipe = 0;
	}
	if( process_lock ) {
		thread_mutex_unlock( process_lock );
		process_lock = 0;
	}

}

int tydemux_thread( void * user_data ) {

	tystream_holder_t * tystream = NULL;
	demux_start_params_t *params;
	int return_value;
	FILE *logfile = stderr;

	atexit( demux_exit_handler);

	if( !user_data ) {
		return(-1);   /* We need some parameters, something has gone horribly wrong  */
	}

	params = ( demux_start_params_t *) user_data;

	/* Initialise the logger */
	if( params->logfile ) {
		logfile = params->logfile;
	}
	logger_init( params->loglevel, logfile, params->loghookfn, 1 );

	/* Tell the main program we are running and that it should block itself on the thread_lock mutex */
	if( params->thread_lock ) {
		thread_mutex_lock( params->thread_lock );
	}
	process_lock = params->thread_lock;

	/* Tell the main program we are running and that it should block itself on the thread_lock mutex */
	params->running = 1;

	if(params->remote_holder) {
		tystream = tydemux_open_probe_remote(params->remote_holder, params->fsid);
		if( !tystream ) {
			return( -1 );
		}
		//printf("Remote in init \n");
		tystream->remote_holder = params->remote_holder;
		tystream->fsid = params->fsid;

	} else {
		tystream = probe_ty_stream( params );
		if( !tystream ) {
			return( -1 );
		}
		tystream->input_pipe = params->input_pipe;
	}

	/* Process tydemux commands */
	if(!parse_args( params->argc, params->argv, tystream)) {
		return(-1);
	}


	tystream->video_pipe = params->video_pipe;
	tystream->audio_pipe = params->audio_pipe;
	awrite_pipe = params->audio_pipe;
	vwrite_pipe = params->video_pipe;

	/* Add the cut list */
	if( params->cutcount && params->cutlist ) {
		int n;
		for( n = 0; n < params->cutcount; ++n ) {
			if( !cutpoint_create_from_tyeditor_string( tystream, params->cutlist[n] ) ) {
				return 0;
			}
		}
	}

	if(!tystream->remote_holder) {
		/* Adjust byte offset  - remote doesn't need this*/
		if(!adj_byte_offset(tystream)) {
			return(0);
		}

	}

	/* Skip to start chunk  - remote doesn't need this*/
	if(!tystream->remote_holder) {
		get_start_chunk(tystream);
	}

	/* All set up, let's go */
	return_value = tydemux(tystream, params);

	//printf("Marking end of stream\n");

	{
		int ntimeout = 10;  /* x 2 seconds = 20 sec */
		long audiobytes, videobytes;
		audiobytes = videobytes = 1;

		/* Manually flush the write buffers to prevent deadlocks with mplex */
		while( ntimeout-- && (audiobytes >0 || videobytes > 0) ) {
			if( audiobytes > 0 ) {
				/* Still more audio bytes to flush */
				audiobytes = thread_pipe_buffer_topup( tystream->audio_pipe );
			}
			if( videobytes > 0) {
				/* Still more video bytes to flush */
				videobytes = thread_pipe_buffer_topup( tystream->video_pipe );
			}
			if( audiobytes > 0 || videobytes > 0 ) {
				sleep(2);  /* mplex still churning */
			}
		}
	}

	//printf("Out of flush\n");

	/* Mark EOF on out output */
	thread_pipe_eof(tystream->audio_pipe, 0);
	thread_pipe_eof(tystream->video_pipe, 0);
	awrite_pipe = 0;
	vwrite_pipe = 0;
	if(tystream->remote_holder) {
		tydemux_close_remote_tystream(tystream->remote_holder);
	}
	free_tystream( tystream );
	logger_free();

	//printf("Unlock thread\n");
	/* All done - release the main process */
	if( params->thread_lock ) {
		thread_mutex_unlock( params->thread_lock );
	}
	process_lock = 0;
	//printf("Returning \n");
	if(return_value) {
		return (0);
	} else {
		return(-1);
	}

}
#endif


