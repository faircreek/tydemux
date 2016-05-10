/*
 * Copyright (C) 2002  John Barrett <johnbarretthcc@hotmail.com>
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

/* "$Id: threadlib.c,v 1.32 2003/03/28 00:51:37 mrbassman Exp $" */
/* Portable threading and Inter-thread communication library */

#include "tycommon.h"

#include <assert.h>
#include <string.h>
#include <time.h>

/* Include files */
#include "tymalloc.h"
#include "threadlib.h"

#define MIN_PEEK_BUFFERSIZE	1024

typedef struct {
	void *user_data;
	thread_start_function_t start_function;
} tl_start_params_t;

typedef struct {
	unsigned long size;
	char *buffer;
} tl_write_buffer_t;

typedef struct {
#ifdef _WIN32
	HANDLE handle_in;
	HANDLE handle_out;
#endif
	int phandles[2];
	FILE *fileh;
	int eof;
	unsigned long bytes_in_pipe;
	unsigned long pipe_size;
	int limit_pipesize;
	mutex_handle_t mutex;
	pipe_stat_t bytesread;
	pipe_stat_t byteswritten;
	unsigned char *peek_buffer;
	unsigned long peek_buffer_size;
	unsigned long peek_next_read;
	tl_write_buffer_t **write_buffers;
	int write_buffers_count;
	int first_write_buffer;
	int last_write_buffer;
	int in_write;
	unsigned long bytes_buffered;
	unsigned long buffer_limit;
} tl_pipedata_t;

static void refill_write_pipe(tl_pipedata_t *pipedata, int block );
static unsigned long add_to_write_buffer(tl_pipedata_t *pipedata, unsigned long bytes, const char *buffer );

/* Thread startup interface functions */
#ifdef WIN32
DWORD WINAPI local_thread_start(LPVOID lpParameter)
#else
void *local_thread_start( void *lpParameter)
#endif
{
	thread_start_function_t start_function;
	void *user_data;
	int result = 0;

	tl_start_params_t *params;
	params = (tl_start_params_t *) lpParameter;

	start_function = params->start_function;
	user_data = params->user_data;

	/* Release our local parameters */
	free( params );

	/* call the users thread start function */
	result = (start_function) ( user_data );

#ifdef WIN32
	return( (DWORD) result );
#else
	return( (void *) result );
#endif

}

/* Start the given function on a new thread passing the given user data */
/* Returns 0 on failure otherwise a thread handle							   */
thread_handle_t thread_start( thread_start_function_t start_function, void *user_data  )
{
	tl_start_params_t *params;
	thread_handle_t th = 0;

	params = malloc( sizeof(tl_start_params_t) );
	params->user_data = user_data;
	params->start_function = start_function;

#ifdef WIN32
	{
		DWORD dwThreadID;
		th = (thread_handle_t) CreateThread( NULL, 0, local_thread_start, (LPVOID) params, 0, &dwThreadID );
	}
#else
        {
                pthread_t threadid;
		if( pthread_create( &threadid, NULL, local_thread_start, params ) == 0 ) {
		        th = (thread_handle_t) threadid;
		}
        }
#endif

	return th;
}

/* Stops the given thread  */
/* Returns 0 on success otherwise an error code */
int thread_stop( thread_handle_t thread_id, int exit_code )
{
	int result = 0;
#ifdef WIN32
//	OutputDebugString("in thread exit\n");
	{
		// Give the thread a few seconds to die naturally
		DWORD dwExitCode = STILL_ACTIVE;
		int delay = 5; // Up to 5 seconds
		while( delay-- ) {
//			OutputDebugString("Checking exit status\n");
			if( GetExitCodeThread( (HANDLE) thread_id, &dwExitCode) ) {
				if( dwExitCode == STILL_ACTIVE ) {
//					OutputDebugString("Waiting for thread exit\n");
					sleep(1);
				} else {
					break;
				}
			} else {
				break;  // Failed to get the exit code - just kill it
			}
		}
//		OutputDebugString("Done waiting for thread exit\n");

		if( dwExitCode == STILL_ACTIVE ) {
			// Kill it the hard way
//			OutputDebugString("Hard terminating thread\n");
			if( !TerminateThread( (HANDLE) thread_id, (DWORD) exit_code ) )
				result = GetLastError();
		}
	}
#else
	/* I cannot find a way of checking the thread's run status under pthreads
	 * so we need this sleep to give the threads time to close properly before we cancel
	 * them - If somebody can figure out how to get the thread status in pthreads, remove
	 * this sleep
	 */
	sleep(2);

    exit_code = exit_code; /* not supportrd under unix */
    result = pthread_cancel( (pthread_t) thread_id );
#endif

	return ( result );
}

/* Stops the caller's thread - equivalent of calling exit() in a main() function */
void thread_exit( int exit_code )
{
#ifdef WIN32
	ExitThread( exit_code );
#else
	pthread_exit( (void *) exit_code );
#endif

}

/* Get the exit code from a thread */
int thread_exit_code( thread_handle_t thread_id)
{
	int exit_code = 0;

#ifdef WIN32
	DWORD dwCode = 0;

	if( GetExitCodeThread( (HANDLE) thread_id, &dwCode ) )
		exit_code = (int) dwCode;
#else
	/* NO support under unix ? */
        thread_id = thread_id;
#endif

	return( exit_code );
}

/* Create a mutex to protect shared memory accesses							 */
/* NOTE: The mutex should only be created once, The handle can be passed	 */
/* in a structure pointed to by the thread user_data parameter				 */
/* Returns a mutex handle or 0 if the call fails							 */
mutex_handle_t thread_mutex_create()
{
	mutex_handle_t mh = 0;

#ifdef WIN32
	mh = (mutex_handle_t) CreateMutex(NULL, FALSE, NULL);
#else
	{
		/* Create a mutex, initialise it and store it's pointer in mh */
		pthread_mutex_t *pmutex;
		pmutex = malloc( sizeof(pthread_mutex_t) );
		if( pthread_mutex_init(pmutex, 0) ) {
			free( pmutex ); /* Error */
		} else {
			mh = (mutex_handle_t) pmutex;
		}
	}
#endif

	return ( mh );
}

/* Lock a mutex to protect shared memory accesses							 */
/* Returns 0 on success otherwise an error code								 */
int thread_mutex_lock( mutex_handle_t mh )
{
#ifdef WIN32
	WaitForSingleObject( (HANDLE) mh, INFINITE);
#else
	pthread_mutex_lock((pthread_mutex_t *) mh);
#endif
	return 0;
}

/* Unlock a mutex after shared memory accesses								 */
/* Returns 0 on success otherwise an error code								 */
int thread_mutex_unlock( mutex_handle_t mh )
{
#ifdef WIN32
	ReleaseMutex( (HANDLE) mh );
#else
	pthread_mutex_unlock((pthread_mutex_t *) mh);
#endif

	return 0;
}

/* Create a mutex to protect shared memory accesses							 */
/* Both the thread and the main process should call this					 */
/* Returns a mutex handle or 0 if the call fails							 */
int thread_mutex_free( mutex_handle_t mh )
{
#ifdef WIN32
	CloseHandle( (HANDLE) mh );
#else
	pthread_mutex_t *pmutex = (pthread_mutex_t *) mh;

	pthread_mutex_destroy( pmutex );
	free( pmutex);
#endif
	return 0;
}


/* Create a FIFO pipe, used to push information to or pull information from   */
/* a thread																	  */
/* NOTE: The pipe should only be created once, The pipe handle can be passed  */
/* in a structure pointed to by the thread user_data parameter				  */
/* Returns the pipe handle, or 0 if error						 			  */
pipe_handle_t thread_pipe_create( unsigned long buffersize )
{
	pipe_handle_t ph = 0;
	tl_pipedata_t *pipedata;

	pipedata = malloc( sizeof(tl_pipedata_t) );
	memset( pipedata, 0, sizeof( tl_pipedata_t ) );
	pipedata->mutex = thread_mutex_create();

	/* Linux has a maximum pipe size of 4K - use write buffers in both environments */
	pipedata->buffer_limit = buffersize;
	pipedata->pipe_size = min( buffersize, 4096 );
	pipedata->limit_pipesize = 1;

#ifdef WIN32
	/* Windows pipes are blocking even when there is enough room for more data */
	/* Setting the pipe size to more than double what we need seems to solve the problem */
	/* At the moment I am suuming it is a bug in the Windows pipe implementation */
	if( _pipe( pipedata->phandles, 10240, O_BINARY | O_NOINHERIT ) ) {
//	if( _pipe( pipedata->phandles, pipedata->pipe_size, O_BINARY | O_NOINHERIT ) ) {
#else
	if( pipe( pipedata->phandles ) ) {
#endif
		/* Error */
		free( pipedata );
	} else {
		ph = (pipe_handle_t) pipedata;
	}

	return( ph );
}

/* Create a read only FIFO pipe, from a file								  */
/* Returns the pipe handle, or 0 if error						 			  */
pipe_handle_t thread_pipe_create_from_file( const char *filename )
{
	pipe_handle_t ph = 0;
	tl_pipedata_t *pipedata;

	pipedata = malloc( sizeof(tl_pipedata_t) );
	memset( pipedata, 0, sizeof(tl_pipedata_t) );
	pipedata->mutex = thread_mutex_create();

	pipedata->phandles[0] = open(filename, OPEN_READONLY_FLAGS );
	if( pipedata->phandles[0] == -1 ) {
		/* Error */
		free( pipedata );
	} else {
		ph = (pipe_handle_t) pipedata;
	}

	return( ph );
}

/* Create a write only FIFO pipe, to a file									  */
/* Returns the pipe handle, or 0 if error						 			  */
pipe_handle_t thread_pipe_create_to_file( const char *filename, int truncate )
{
	int flags;

	pipe_handle_t ph = 0;
	tl_pipedata_t *pipedata;

	pipedata = malloc( sizeof(tl_pipedata_t) );
	memset( pipedata, 0, sizeof(tl_pipedata_t) );
	pipedata->mutex = thread_mutex_create();

	flags = truncate ? OPEN_CREATE_FLAGS : OPEN_WRITE_FLAGS;
	pipedata->phandles[1] = open(filename, flags, OPEN_WRITE_PERMISSION );
	if( pipedata->phandles[1] == -1 ) {
		/* Error */
		free( pipedata );
	} else {
		ph = (pipe_handle_t) pipedata;
	}

	return( ph );

}

/* Create a read only FIFO pipe, from a file handle							  */
/* Returns the pipe handle, or 0 if error						 			  */
pipe_handle_t thread_pipe_create_with_handle( FILE *fh )
{
	tl_pipedata_t *pipedata;

	pipedata = malloc( sizeof(tl_pipedata_t) );
	memset( pipedata, 0, sizeof(tl_pipedata_t) );
	pipedata->mutex = thread_mutex_create();

	pipedata->fileh = fh;

	return( (pipe_handle_t) pipedata );
}

/* Return the total number of bytes the stream can return */
/* Returns 0 if unknown */
uint64_t thread_pipe_streamsize( pipe_handle_t ph )
{
	uint64_t pipe_size = 0;
	stat64_t file_info;

	tl_pipedata_t *pipedata = (tl_pipedata_t *) ph;

	if( pipedata ) {
		if( pipedata->phandles[0] ) {
			if( fstat64(pipedata->phandles[0], &file_info) != -1 ) {
				pipe_size = file_info.st_size;
			}
		}
	}

	return (pipe_size);
}

/* Mark the end of file for a FIFO pipe										  */
/* Called by the sending side of the pipe to indicate there is no more data	  */
/* i.e. Reached the end of the input										  */
/* Returns 0 on success otherwise an error code								  */
int thread_pipe_eof( pipe_handle_t ph, int flush )
{
	int result = 0;

	tl_pipedata_t *pipedata = (tl_pipedata_t *) ph;

	if( pipedata ) {

		if( flush ) {
			time_t tStart = time(NULL);

			pipedata->in_write = 1;

			/* Flush the write buffers first */
			while( pipedata->write_buffers && pipedata->first_write_buffer != pipedata->last_write_buffer) {
				/* refill the write pipe */
				refill_write_pipe( pipedata, 1 );
				/* If 15 seconds have elapsed - give up flushing */
				if( (time(NULL) - tStart > 15) ) {
					break;
				} else {
					sleep(0);
				}
			}

			/* also wait till the pipe itself has been emptied */
			/* If 15 seconds have elapsed - give up waiting */
			while( pipedata->bytes_in_pipe && (time(NULL) - tStart < 15) ) {
				sleep(0);
			}
			pipedata->in_write = 0;
		}

		pipedata->eof = 1;

		if( pipedata->fileh ) {
			fclose( pipedata->fileh );
			pipedata->fileh = NULL;
		}

		/* Just close the write side of the pipe */
		if( pipedata->phandles[1] ) {
			close( pipedata->phandles[1] );
			pipedata->phandles[1] = 0;
		}
#ifdef WIN32
		else if( pipedata->handle_out ) {
			CloseHandle( pipedata->handle_out );
			pipedata->handle_out = NULL;
		}
#endif
	} else {
		result = -1;
	}

	return ( result );
}

/* Check for the end of file for the read side of a FIFO pipe */
/* Returns 1 on end of file otherwise 0	  */
int thread_pipe_iseof( pipe_handle_t ph )
{
	tl_pipedata_t *pipedata = (tl_pipedata_t *) ph;

	return ( pipedata->eof );

}

/* Free a FIFO pipe														      */
/* NOTE: The pipe should only be freed once. You should ensure all threads    */
/* using the pipe have finished with it before trying to close it			  */
/* Returns 0 on success otherwise an error code								  */
int thread_pipe_free( pipe_handle_t ph )
{
	int result = 0;

	tl_pipedata_t *pipedata = (tl_pipedata_t *) ph;

	if( pipedata ) {
		if( pipedata->phandles[0] ) {
			close( pipedata->phandles[0] );
		}
		if( pipedata->phandles[1] ) {
			close( pipedata->phandles[1] );
		}
#ifdef WIN32
		if( pipedata->handle_in ) {
			CloseHandle( pipedata->handle_in );
		}
		if( pipedata->handle_out ) {
			CloseHandle( pipedata->handle_out );
		}
#endif
		if( pipedata->fileh ) {
			fclose( pipedata->fileh );
			pipedata->fileh = NULL;
		}

		/* Free memory used for the write buffers */
		if( pipedata->write_buffers ) {
			int n;
			for( n = 0; n < pipedata->write_buffers_count; ++ n ) {
				if( pipedata->write_buffers[n] ) {
					free( pipedata->write_buffers[n] ) ;
				}
			}
			free( pipedata->write_buffers );
		}

		thread_mutex_free(pipedata->mutex);

		free( pipedata );
	} else {
		result = -1;
	}

	return ( result );
}

/* Refill the write side of the pipe with any stored write buffers */
/* block = 0, no blocking, 1 = must block, 2 = block until room in buffer */
static void refill_write_pipe(tl_pipedata_t *pipedata, int block )
{
	if( !block ) {
		/* As we may not release processing time during a write call */
		/* We should be nice to other threads and offer them some processor time */
		/* This is in our interests too as they may clear out some buffer space */
		sleep(0);
	}

	thread_mutex_lock(pipedata->mutex);
	if( pipedata->limit_pipesize ) {

		if( pipedata->write_buffers && pipedata->first_write_buffer != pipedata->last_write_buffer) {
			long result;
			int writeit;
			tl_write_buffer_t *pbuffer;

			do {
				result = 0;
				writeit = 0;

				pbuffer = pipedata->write_buffers[pipedata->first_write_buffer];
				assert( pbuffer );
				assert( pbuffer->buffer );
				assert( pbuffer->size );

				if( (block == 2) && (pipedata->bytes_buffered < pipedata->buffer_limit ) ) {
					/* We were blocking because the buffer is full */
					/* there is now room so we don't need to do that any more */
					block = 0;
				}

				/* Is there room in the pipe for the next buffer? */
				if( block > 0 || ((pipedata->bytes_in_pipe + pbuffer->size + 256) <= pipedata->pipe_size) ) {
					writeit = 1;

					/* move on */
					pipedata->write_buffers[pipedata->first_write_buffer] = NULL;
					if( ++(pipedata->first_write_buffer) >= pipedata->write_buffers_count ) {
						pipedata->first_write_buffer = 0;
					}
					pipedata->bytes_in_pipe += pbuffer->size;
					pipedata->bytes_buffered -= pbuffer->size;
				}

				if( writeit ) {

					/* Add this buffer to the write pipe */
					thread_mutex_unlock(pipedata->mutex);

					if( pipedata->phandles[1] ) {
						result = write( pipedata->phandles[1], pbuffer->buffer, pbuffer->size );
					}
#ifdef WIN32
					else if( pipedata->handle_out ) {
						DWORD written;
						if( WriteFile( pipedata->handle_out, pbuffer->buffer, pbuffer->size, &written, NULL ) ) {
							result = (long) written;
						}
					}
#endif
					thread_mutex_lock(pipedata->mutex);

					assert( result == (long)(pbuffer->size) );

					/* Free the buffer */
					free( pbuffer->buffer );
					free( pbuffer );
				}
			} while( writeit && result && pipedata->first_write_buffer != pipedata->last_write_buffer );
		}
	}
	thread_mutex_unlock(pipedata->mutex);
}

/* Add this data to the write buffer */
static unsigned long add_to_write_buffer(tl_pipedata_t *pipedata, unsigned long bytes, const char *buffer )
{
	tl_write_buffer_t *pbuffer;
	int block = 0;

	thread_mutex_lock(pipedata->mutex);

	/* Make room in the buffer list */
	if( !(pipedata->write_buffers) ) {
		/* Allocate 64 buffers at a time */
		pipedata->write_buffers = malloc( sizeof( tl_write_buffer_t * ) * 64 );
		if( !pipedata->write_buffers  ) {
			thread_mutex_unlock(pipedata->mutex);
			return -1;
		}
		memset( pipedata->write_buffers, 0 , sizeof( tl_write_buffer_t *) * 64 );
		pipedata->write_buffers_count = 64;
	} else {
		int free_buffers = 1;

		if( pipedata->last_write_buffer == (pipedata->first_write_buffer-1)  ) {
			free_buffers = 0;
		} else if( pipedata->first_write_buffer == 0 &&
				   (pipedata->last_write_buffer == (pipedata->write_buffers_count-1) ) ) {
			free_buffers = 0;
		}

		if(!free_buffers) {

			/* Re-organise to make more room */
			int newnumber;
			int n, i;
			tl_write_buffer_t **newbuffer;
			newnumber = (((pipedata->write_buffers_count)/64)+1) * 64;

			newbuffer = malloc( sizeof( tl_write_buffer_t *) * newnumber );
			if( !newbuffer ) {
				thread_mutex_unlock(pipedata->mutex);
				return -1;
			}
			memset( newbuffer, 0, sizeof( tl_write_buffer_t *) * newnumber );

			/* Move the buffer pointers to the new array */
			i = 0;
			n = pipedata->first_write_buffer;
			while( n != pipedata->last_write_buffer ) {

				newbuffer[i++] = pipedata->write_buffers[n++];

				if( n >= pipedata->write_buffers_count ) {
					n = 0;
				}
			}

			/* switch over to the new array and free the old one */
			free( pipedata->write_buffers );
			pipedata->write_buffers = newbuffer;
			pipedata->first_write_buffer = 0;
			pipedata->last_write_buffer = pipedata->write_buffers_count -1;
			pipedata->write_buffers_count = newnumber;
		}
	}

	/* guaranteed to have room so now add the buffer to the list */
	pbuffer = malloc( sizeof(tl_write_buffer_t) );
	if( !pbuffer ) {
		thread_mutex_unlock(pipedata->mutex);
		return -1;
	}
	pbuffer->buffer = malloc( bytes );
	if( !pbuffer->buffer ) {
		free( pbuffer );
		thread_mutex_unlock(pipedata->mutex);
		return -1;
	}
	memcpy( pbuffer->buffer, buffer, bytes );
	pbuffer->size = bytes;
	pipedata->write_buffers[ pipedata->last_write_buffer ] = pbuffer;
	pipedata->bytes_buffered += pbuffer->size;

	if(++(pipedata->last_write_buffer) >= pipedata->write_buffers_count) {
		pipedata->last_write_buffer = 0;
	}
	assert( pipedata->last_write_buffer != pipedata->first_write_buffer );

	/* If we have exceeded the orginal pipe size requested by the user, block when refilling the write pipe */
	/* Otherwise just write if there is room and then return without blocking */
	if( (pipedata->limit_pipesize==0) || (pipedata->bytes_buffered >= pipedata->buffer_limit ) ) {
		block = 2;
	}

	thread_mutex_unlock(pipedata->mutex);

	/* refill the write pipe */
	refill_write_pipe( pipedata, block );

	return( bytes );
}

/* Write information to a FIFO pipe											  */
/* ph - Pipe handle															  */
/* bytes - Number of bytes in the buffer									  */
/* buffer - Bytes to be written												  */
/* Returns the number of bytes actually written					 			  */
unsigned long thread_pipe_write( pipe_handle_t ph, unsigned long bytes, const char *buffer )
{
	long result = 0;
	unsigned long byteswritten = 0;

	tl_pipedata_t *pipedata = (tl_pipedata_t *) ph;

	if( pipedata ) {
		unsigned long chunksize;
		unsigned long bytes_to_write = bytes;
		int error = 0;

		thread_mutex_lock(pipedata->mutex);

		pipedata->in_write = 1;

		while( bytes_to_write && !error ) {

			/* Number of bytes to write in each chunk */
			if( pipedata->limit_pipesize ) {
				chunksize = min( bytes_to_write, (pipedata->pipe_size) / 2 );
			} else {
				chunksize = bytes_to_write;
			}

			thread_mutex_unlock(pipedata->mutex);

			/* If the write pipe is active, put this buffer in there instead */
			if( pipedata->limit_pipesize ) {
				result = add_to_write_buffer( pipedata, chunksize, &(buffer[byteswritten]) );
				if( result == -1 ) {
					error = 1;
				}
			}
			else if( pipedata->phandles[1] ) {
				/* Using low level I/O */
				result = write( pipedata->phandles[1], &(buffer[byteswritten]), chunksize );
				if( result == -1 ) {
					error = 1;
				}
			} else if ( pipedata->fileh ) {
				/* Using streamed file I/O */
				result = fwrite( &(buffer[byteswritten]), sizeof(char), chunksize, pipedata->fileh );
				if( result != (long)chunksize ) {
					error = 1;
				}
			}
			thread_mutex_lock(pipedata->mutex);

			if( result > 0 ) {
				byteswritten += result;
				pipedata->byteswritten += result;
				bytes_to_write -= result;
			}
		}

		pipedata->in_write = 0;

		thread_mutex_unlock(pipedata->mutex);
	}

	return ( byteswritten );
}

/* Read information from a FIFO pipe										  */
/* ph - Pipe handle															  */
/* bytes - Number of bytes to read											  */
/* buffer - Storage for bytes read											  */
/* Returns the number of bytes actually read					 			  */
unsigned long thread_pipe_read( pipe_handle_t ph, unsigned long bytes, char *buffer )
{
	long result = 0;
	unsigned long bytes_to_read;
	unsigned long bytes_read;
	unsigned long chunksize;

	tl_pipedata_t *pipedata = (tl_pipedata_t *) ph;
	bytes_to_read = bytes;
	bytes_read = 0;

	if( pipedata ) {
		thread_mutex_lock(pipedata->mutex);

		/* Check for bytes in the peek buffer - return these first */
		if( pipedata->peek_buffer ) {
			unsigned long bytes_transfered;

			/* Transfer smallest of bytes requested and bytes available */
			bytes_transfered = pipedata->peek_buffer_size - pipedata->peek_next_read;
			if( bytes_transfered > bytes ) {
				bytes_transfered = bytes;
			}

			memcpy( buffer, &(pipedata->peek_buffer[pipedata->peek_next_read]), bytes_transfered );
			bytes_read += bytes_transfered;
			pipedata->peek_next_read += bytes_transfered;
			bytes_to_read -= bytes_transfered;

			/* See if the peek buffer is now empty */
			if( pipedata->peek_buffer_size - pipedata->peek_next_read <= 0 ) {
				/* Free the peek buffer */
				free( pipedata->peek_buffer );
				pipedata->peek_buffer = 0;
				pipedata->peek_buffer_size = 0;
				pipedata->peek_next_read = 0;
			}

		}

		while( !pipedata->eof && bytes_to_read ) {
			int eof = 0;
			result = 0;

			/* Number of bytes to read in each chunk */
			if( pipedata->limit_pipesize ) {
				chunksize = min( bytes_to_read, pipedata->pipe_size / 4);
			} else {
				chunksize = bytes_to_read;
			}

			thread_mutex_unlock(pipedata->mutex);
			/* Read chunksize bytes */
			if( pipedata->phandles[0] ) {
				/* Using low level I/O */
				result = read( pipedata->phandles[0], &(buffer[bytes_read]), chunksize );
				if( result <= 0 ) {
					/* End of file reached */
					eof = 1;
				}
			} else if ( pipedata->fileh ) {
				/* Using streamed file I/O */
				result = fread( &(buffer[bytes_read]), sizeof( char), chunksize, pipedata->fileh );
				if( result != (long)chunksize && feof(pipedata->fileh) ) {
					/* End of file reached */
					eof = 1;
				}
			}
#ifdef WIN32
			else if( pipedata->handle_in ) {
				DWORD read_count;
				result = 0;
				thread_mutex_lock(pipedata->mutex);
				if( ReadFile( pipedata->handle_in, &(buffer[bytes_read]), chunksize, &read_count, NULL ) ) {
					result = (long) read_count;
				} else {
					eof = 1;
				}
			}
#endif
			else {
				eof = 1;
			}
			thread_mutex_lock(pipedata->mutex);

			pipedata->eof = eof;

			if( result > 0 ) {
				if( pipedata->limit_pipesize ) {
					pipedata->bytes_in_pipe -= result;
				}
				bytes_read += result;
				bytes_to_read -= result;
				pipedata->bytesread += result;
			}

			/* refill the write pipe */
			if( !pipedata->in_write && !pipedata->bytes_in_pipe ) {
 				thread_mutex_unlock(pipedata->mutex);
				refill_write_pipe( pipedata, 0 );
 				thread_mutex_lock(pipedata->mutex);
			}
		}
		thread_mutex_unlock(pipedata->mutex);
	}

	return ( bytes_read );
}


/* Read information from a FIFO pipe without moving the next read position	  */
/* The peeked bytes are retained in a buffer and will be returned again		  */
/* when thread_pipe_read is called.											  */
/* ph - Pipe handle															  */
/* bytes - Number of bytes to read											  */
/* buffer - Storage for bytes read											  */
/* Returns the number of bytes actually read					 			  */
unsigned long thread_pipe_peek( pipe_handle_t ph, unsigned long bytes, char *buffer )
{
	tl_pipedata_t *pipedata = (tl_pipedata_t *) ph;
	unsigned long old_buffer_size = pipedata->peek_buffer_size;
	unsigned long bytes_to_read;
	unsigned long bytes_read;
	unsigned long chunksize;

	thread_mutex_lock(pipedata->mutex);

	if( pipedata->peek_buffer ) {

		/* peek buffer not empty, see if we have enough bytes in it already */
		unsigned long available_bytes = pipedata->peek_buffer_size - pipedata->peek_next_read;
		if( available_bytes < bytes ) {
			/* Need to do some shuffling around to make room for more bytes
			 * to be read into the peek buffer */

			/* Close the whole left by bytes already read */
			if( pipedata->peek_next_read && available_bytes ) {
				memmove( pipedata->peek_buffer, &(pipedata->peek_buffer[pipedata->peek_next_read]), available_bytes );
			}
			pipedata->peek_next_read = 0;

			/* See if we need to realloc the memory */
			if( bytes > pipedata->peek_buffer_size ) {
				pipedata->peek_buffer_size += bytes;
				pipedata->peek_buffer = realloc( pipedata->peek_buffer, pipedata->peek_buffer_size );
				if( !(pipedata->peek_buffer) ) {
					/* REALLY bad news! */
					pipedata->peek_buffer_size = 0;
					thread_mutex_unlock(pipedata->mutex);
					return 0;
				}
			}

		}
	} else {
		/* Peek buffer is empty, fill it up with bytes requested */

		pipedata->peek_buffer_size = bytes;
		pipedata->peek_next_read = 0;

		if( pipedata->peek_buffer_size < MIN_PEEK_BUFFERSIZE ) {
			/* If you're gonaa peek, you might as well have a good look */
			pipedata->peek_buffer_size = MIN_PEEK_BUFFERSIZE;
		}

		pipedata->peek_buffer = malloc( pipedata->peek_buffer_size );
		if( !(pipedata->peek_buffer) ) {
			/* REALLY bad news! */
			pipedata->peek_buffer_size = 0;
			thread_mutex_unlock(pipedata->mutex);
			return 0;
		}
	}


	/* Read bytes from the stream to fill up the peek buffer */
	if( old_buffer_size < pipedata->peek_buffer_size ) {
		long result = 0;
		int eof;
		char *readpos = &(pipedata->peek_buffer[ old_buffer_size ]);
		bytes_to_read = pipedata->peek_buffer_size - old_buffer_size;
		eof = pipedata->eof;
		bytes_read = 0;

		while( !eof && bytes_to_read ) {

			result = 0;

			/* Number of bytes to read in each chunk */
			if( pipedata->pipe_size ) {
				chunksize = min( bytes_to_read, pipedata->pipe_size );
			} else {
				chunksize = bytes_to_read;
			}

			thread_mutex_unlock(pipedata->mutex);
			/* Read chunksize bytes */
			if( pipedata->phandles[0] ) {
				/* Using low level I/O */
				result = read( pipedata->phandles[0], &(readpos[bytes_read]), chunksize );
				if( result != (long)chunksize ) {
					/* End of file reached */
					if( result == - 1 ) {
						result = 0;
					}
					eof = 1;
				}
			} else if ( pipedata->fileh ) {
				/* Using streamed file I/O */
				result = fread( &(readpos[bytes_read]), sizeof( char), chunksize, pipedata->fileh );
				if( result != (long) chunksize && feof(pipedata->fileh) ) {
					/* End of file reached */
					eof = 1;
				}
			}
			else {
				eof = 1;
			}
			thread_mutex_lock(pipedata->mutex);

			if( result > 0 ) {
				if( pipedata->limit_pipesize ) {
					pipedata->bytes_in_pipe -= result;
				}
				bytes_read += result;
				bytes_to_read -= result;
				pipedata->bytesread += result;
			}
		}

		/* If we didn't read enough, reduce the amount returned */
		bytes = bytes_read;
	}

	/* We should now have enough bytes in the peek buffer to satisfy the request */
	memcpy( buffer, &(pipedata->peek_buffer[pipedata->peek_next_read]), bytes );
	
	thread_mutex_unlock(pipedata->mutex);

	return bytes;
}

/* Get the pipe stats for progress monitoring								  */
/* ph - Pipe handle															  */
/* pread - Pointer to storage for number of bytes read	(unsigned 64bit)	  */
/* pwritten - Pointer to storage for number of bytes read (unsigned 64bit)	  */
void thread_pipe_stats( pipe_handle_t ph, pipe_stat_t *pread, pipe_stat_t *pwritten )
{
	tl_pipedata_t *pipedata = (tl_pipedata_t *) ph;

	*pread = pipedata->bytesread;
	*pwritten = pipedata->byteswritten;

}

/* Return the number of free bytes in the pipe write buffer
 * ph - Pipe handle
 * Returns number of bytes available in the buffer or -1 if there is no limit
 */

long thread_pipe_freespace( pipe_handle_t ph )
{
	tl_pipedata_t *pipedata = (tl_pipedata_t *) ph;

	if( pipedata->limit_pipesize ) {
		return( pipedata->buffer_limit - pipedata->bytes_buffered );
	}

	return (-1);
}

/* Return the number of bytes available for reading
 * ph - Pipe handle
 * Returns number of bytes available in the buffer or -1 if unknown (file pipe)
 */
long thread_pipe_bytes_available( pipe_handle_t ph )
{
	tl_pipedata_t *pipedata = (tl_pipedata_t *) ph;

	if( pipedata->limit_pipesize ) {
		return( pipedata->bytes_in_pipe + pipedata->bytes_buffered );
	}

	return (-1);
}

/* non-blocking attempt to write more bytes from the write buffer to the pipe */
/* Returns 0 if the write buffer is empty, otherwise the number of bytes still in the write buffer */
/* Returns -1 if an error occured */
long thread_pipe_buffer_topup( pipe_handle_t ph )
{
	long result = 0;

	tl_pipedata_t *pipedata = (tl_pipedata_t *) ph;

	if( pipedata ) {
		/* non-blocking refill the write pipe */
		refill_write_pipe( pipedata, 0 );
		result = pipedata->bytes_buffered;
	} else {
		result = -1;
	}

	return ( result );
}