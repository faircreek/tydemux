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

/* "$Id: threadlib.h,v 1.18 2003/03/26 11:52:40 mrbassman Exp $" */
/* Threading and Inter-thread communication library */

#ifndef __THREADLIB_H__
#define __THREADLIB_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>

#include "tytypes.h"

typedef unsigned long thread_handle_t;
typedef unsigned long pipe_handle_t;
typedef unsigned long mutex_handle_t;

#ifdef WIN32
typedef unsigned __int64 pipe_stat_t;
#else
#include <pthread.h>
typedef uint64_t pipe_stat_t;
#define min(a,b)      ((a) < (b) ? (a) : (b))
#endif

/* Definitions */
typedef int (*thread_start_function_t)( void *user_data );

/* Functions */

/* Start the given function on a new thread passing the given user data		   */
/* Returns 0 on failure otherwise a thread handle							   */
/* Note that if user_data is a pointer to memory, that memory should not be	   */
/* freed until you are sure the thread has finished with it. It is safest to   */
/* make the thread start function responsible for freeing this memory		   */
thread_handle_t thread_start( thread_start_function_t start_function, void *user_data );

/* Stops the given thread  */
/* Returns 0 on success otherwise an error code */
int thread_stop( thread_handle_t thread_id, int exit_code );

/* Get the exit code from a thread (not supported in Unix) */
int thread_exit_code( thread_handle_t thread_id);

/* The following function should ONLY be called from within a thread			 */
/* Stops the caller's thread - equivalent of calling exit() in a main() function */
/* Note that the exit code is not supported in Unix								 */
void thread_exit( int exit_code );

/* Create a mutex to protect shared memory accesses							 */
/* NOTE: The mutex should only be created once, The handle can be passed	 */
/* in a structure pointed to by the thread user_data parameter				 */
/* Returns a mutex handle or 0 if the call fails							 */
mutex_handle_t thread_mutex_create();

/* Lock a mutex to protect shared memory accesses							 */
/* Returns 0 on success otherwise an error code								 */
int thread_mutex_lock( mutex_handle_t mh );

/* Unlock a mutex after shared memory accesses								 */
/* Returns 0 on success otherwise an error code								 */
int thread_mutex_unlock( mutex_handle_t mh );

/* Free a mutex																 */
/* Returns a mutex handle or 0 if the call fails							 */
int thread_mutex_free( mutex_handle_t mh );

/* Create a FIFO pipe, used to push information to or pull information from   */
/* a thread																	  */
/* NOTE: The pipe should only be created once, The pipe handle can be passed  */
/* in a structure pointed to by the thread user_data parameter				  */
/* buffersize - size of the pipe (ignored in Unix)							  */
/* Returns the pipe handle, or 0 if error						 			  */
pipe_handle_t thread_pipe_create( unsigned long buffersize );

/* Create a read only FIFO pipe, from a file								  */
/* Returns the pipe handle, or 0 if error						 			  */
pipe_handle_t thread_pipe_create_from_file( const char *filename );

/* Create a write only FIFO pipe, to a file									  */
/* Returns the pipe handle, or 0 if error						 			  */
pipe_handle_t thread_pipe_create_to_file( const char *filename, int truncate );

/* Create a read only FIFO pipe, from a file handle							  */
/* Returns the pipe handle, or 0 if error						 			  */
pipe_handle_t thread_pipe_create_with_handle( FILE *fh );

/* Mark the end of file for a FIFO pipe										  */
/* Called by the sending side of the pipe to indicate there is no more data	  */
/* i.e. Reached the end of the input										  */
/* Returns 0 on success otherwise an error code								  */
int thread_pipe_eof( pipe_handle_t ph, int flush );

/* Check for the end of file for a FIFO pipe */
/* Returns 1 on end of file otherwise 0	  */
int thread_pipe_iseof( pipe_handle_t ph );

/* Free a FIFO pipe														      */
/* NOTE: The pipe should only be freed once. You should ensure all threads	  */
/* using the pipe have finished with it before trying to close it			  */
/* Returns 0 on success otherwise an error code								  */
int thread_pipe_free( pipe_handle_t ph );

/* Write information to a FIFO pipe											  */
/* ph - Pipe handle															  */
/* bytes - Number of bytes in the buffer									  */
/* buffer - Bytes to be written												  */
/* Returns the number of bytes actually written					 			  */
unsigned long thread_pipe_write( pipe_handle_t ph, unsigned long bytes, const char *buffer );

/* Read information from a FIFO pipe										  */
/* ph - Pipe handle															  */
/* bytes - Number of bytes to read											  */
/* buffer - Storage for bytes read											  */
/* Returns the number of bytes actually read					 			  */
unsigned long thread_pipe_read( pipe_handle_t ph, unsigned long bytes, char *buffer );

/* Read information from a FIFO pipe without moving the next read position	  */
/* The peeked bytes are retained in a buffer and will be returned again		  */
/* when thread_pipe_read is called.											  */
/* ph - Pipe handle															  */
/* bytes - Number of bytes to read											  */
/* buffer - Storage for bytes read											  */
/* Returns the number of bytes actually read					 			  */
unsigned long thread_pipe_peek( pipe_handle_t ph, unsigned long bytes, char *buffer );

/* Get the pipe stats for progress monitoring								  */
/* ph - Pipe handle															  */
/* pread - Pointer to storage for number of bytes read	(unsigned 64bit)	  */
/* pwritten - Pointer to storage for number of bytes read (unsigned 64bit)	  */
void thread_pipe_stats( pipe_handle_t ph, pipe_stat_t *pread, pipe_stat_t *pwritten );

/* Return the number of free bytes in the pipe write buffer
 * ph - Pipe handle
 * Returns number of bytes available in the buffer or -1 if there is no limit
 */
long thread_pipe_freespace( pipe_handle_t ph );

/* Return the number of bytes available for reading
 * ph - Pipe handle
 * Returns number of bytes available in the buffer or -1 if unknown (file pipe)
 */
long thread_pipe_bytes_available( pipe_handle_t ph );

/* non-blocking attempt to write more bytes from the write buffer to the pipe */
/* Returns 0 if the write buffer is empty, otherwise the number of bytes still in the write buffer */
/* Returns -1 if an error occured */
long thread_pipe_buffer_topup( pipe_handle_t ph );

/* Return the total number of bytes the stream can return */
/* Returns 0 if unknown */
uint64_t thread_pipe_streamsize( pipe_handle_t ph );

#ifdef __cplusplus
}
#endif

#endif // __THREADLIB_H__
