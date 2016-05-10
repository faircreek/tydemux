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

/* "$Id: test.c,v 1.9 2003/03/17 20:46:01 mrbassman Exp $" */

/* Threading and Inter-thread communication library */
/* This is a test program to excersise the library. It also serves as sample code */
/* to show developers how to use the library */

#ifdef WIN32
#include <windows.h>

#define sleep(s) Sleep(s * 1000)
#else
#include <unistd.h>
#include <inttypes.h>

#endif

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include "threadlib.h"

static mutex_handle_t print_mutex = 0;
static time_t tstart = 0;
#define PIPE_SIZE 0x10
#define PIPE_READ_CHUNK 10240

int elapsed()
{
	time_t tnow = time(0);

	if( tstart == 0 )
		tstart = time(0);

	return( (int) (tnow - tstart) );
}

void locked_printf( char *fmt, ... )
{
	va_list argptr;
	va_start(argptr, fmt);

	if( !print_mutex )
		print_mutex = thread_mutex_create();

	thread_mutex_lock( print_mutex );

	vprintf( fmt, argptr);

	thread_mutex_unlock( print_mutex );
}

typedef struct
{
	int argc;
	char **argv;
} myuserdata_t;

int test_thread1( void *user_data )
{
	myuserdata_t *pData;
	int i;

	pData = (myuserdata_t *) user_data;

	locked_printf( "In test thread 1 with %d arguments\n", pData->argc );

	for( i = 0; i < pData->argc; ++i ) {
		locked_printf( "   Argument %d: %s\n", i, pData->argv[i] );
	}

	locked_printf( "Waiting for termination\n" );
	while(1) {
		sleep(1);
	}

	/* Never actually reach here */
	return 0;
}

int test_thread2( void *user_data )
{
	int exit_code;

	exit_code = (int) user_data;

	locked_printf( "In test thread 2 with parameter %d\n", exit_code );

	locked_printf( "Terminating thread 2 with exit code %d\n", exit_code );

	thread_exit(exit_code);

	/* Never actually reach here */
	return 0;
}

int test_thread3( void *user_data )
{
	mutex_handle_t mutex;

	mutex = (mutex_handle_t) user_data;

	locked_printf( "%03d: In test thread 3 trying to lock mutex\n", elapsed() );

	if( thread_mutex_lock( mutex ) != 0 ) {
		locked_printf( "%03d: Thread 3 mutex lock failed\n", elapsed() );
	} else {
		locked_printf( "%03d: Thread 3 mutex obtained lock on the mutex\n", elapsed() );
		locked_printf( "%03d: Thread 3 sleeping 5 seconds\n", elapsed() );
		sleep(5);
		locked_printf( "%03d: Thread 3 mutex releasing my lock on the mutex\n", elapsed() );
		thread_mutex_unlock( mutex );
	}

	locked_printf( "%03d: Thread 3 finished\n", elapsed() );

	return 0;
}

typedef struct {
	int			  threadnumber;
	pipe_handle_t pipe_in;
	pipe_handle_t pipe_out;
} test4params_t;

int test_thread4( void *user_data )
{
	int nbytes_read;
	int nbytes_written;
	char buffer[PIPE_READ_CHUNK];

	test4params_t *params;

	params = (test4params_t *) user_data;
	locked_printf( "In test thread 4, copy %d passing data through a pipe\n", params->threadnumber );


	while(!thread_pipe_iseof(params->pipe_in) ) {
//		nbytes_read = thread_pipe_peek( params->pipe_in, 10, buffer );
//		buffer[nbytes_read] = 0;
//		locked_printf( "Thread 4, copy %d peeked %d bytes: %s\n", params->threadnumber, nbytes_read, buffer );
		nbytes_read = thread_pipe_read( params->pipe_in, PIPE_READ_CHUNK, buffer );
//		locked_printf( "Thread 4, copy %d read %d bytes\n", params->threadnumber , nbytes_read );
		if( nbytes_read ) {
//			sleep(1);  /* Simulate a delay processing the data */
			nbytes_written = thread_pipe_write( params->pipe_out, nbytes_read, buffer );
			if( nbytes_written != nbytes_read ) {
				locked_printf( "WARNING: Thread 4, copy %d only wrote %d bytes\n", params->threadnumber, nbytes_written );
			}
		}
	}

	locked_printf( "Thread 4, copy %d marking end of the pipe we were writing to\n", params->threadnumber );
	thread_pipe_eof( params->pipe_out, 1 );

	locked_printf( "Thread 4, copy %d finished\n", params->threadnumber  );

	return 0;
}

char *pipedata[] = {
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"This is data for the pipe\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	"The quick brown fox jumped over the lazy dog\n",
	0
};

/* Three seperate tests are run by this program */
/* The first tests starting a couple of threads passing them parameters */
/* The second tests mutexes by deliberately blocking between the main program and a thread */
/* The third creates a couple of identical threads with pipes between them */
/* Data is pushed to the first thread by the main program, the first thread pushes */
/* the data to the second thread and then the main program pulls back the data from the */
/* second thread and displays it */

int main( int argc, char **argv )
{
	int result = 0;
//	int i, bytes_read;
//	char buffer[1024];
	test4params_t t4params_1, t4params_2;
	thread_handle_t th1, th2, th3;
	myuserdata_t user_data;
	mutex_handle_t mutex;

	/* Test 1 - create a couple of threads and pass them data */
	/* Set up user data. In this case the memory is on the stack the entire time the */
	/* child thread is running. If this was not the case, we would need to allocate  */
	/* memory for the data here and get the thread to free it when it was finished   */
	user_data.argc = argc;
	user_data.argv = argv;

	locked_printf( "Starting thread 1\n" );
	th1 = thread_start( test_thread1, &user_data );

	if( !th1 ) {
		locked_printf( "Thread 1 startup failed\n" );
		return( -1 );
	}

	locked_printf( "Starting thread 2\n" );
	th2 = thread_start( test_thread2, (void *) 45);

	if( !th2 ) {
		locked_printf( "Thread 2 startup failed\n" );
		return( -1 );
	}

	locked_printf( "Delaying 2 seconds\n" );
	sleep(2);

	locked_printf( "Stopping Thread 1\n" );
	result = thread_stop( th1, 32 );
	if( result ) {
		locked_printf( "Stop failed with %d\n", result );
	}

	locked_printf( "Delaying 2 seconds\n" );
	sleep(2);

	result = thread_exit_code( th1 );
	locked_printf( "Thread 1 exit code was %d\n", result );

	result = thread_exit_code( th2 );
	locked_printf( "Thread 2 exit code was %d\n", result );

	/* Test 2 - create a thread and check mutexes are working */

	locked_printf( "%03d: Creating a mutex for thread 3\n", elapsed() );
	mutex = thread_mutex_create();
	if( !mutex ) {
		locked_printf( "mutex create failed\n" );
		return( -1 );
	}

	locked_printf( "%03d: main locking the mutex\n", elapsed() );
	if( thread_mutex_lock( mutex ) != 0 ) {
		locked_printf( "main mutex lock failed\n" );
		return( -1 );
	}

	locked_printf( "%03d: Starting thread 3\n", elapsed() );
	th3 = thread_start( test_thread3, (void *) mutex );
	if( !th3 ) {
		locked_printf( "Thread 3 startup failed\n" );
		return( -1 );
	}

	locked_printf( "%03d: main sleeping 5 seconds\n", elapsed() );
	sleep(5);
	locked_printf( "%03d: main releasing lock on the mutex\n", elapsed() );
	thread_mutex_unlock( mutex );

	locked_printf( "%03d: main sleeping 1 second\n", elapsed() );
	sleep(1);
	locked_printf( "%03d: main re-locking the mutex\n", elapsed() );
	thread_mutex_lock( mutex );

	locked_printf( "%03d: main releasing lock on the mutex\n", elapsed() );
	thread_mutex_unlock( mutex );

	thread_mutex_free( mutex );

	/* Test 3 - create multiple copies of a thread and pipe information between them */
	locked_printf( "Creating linked pipes for multiple copies of thread 4\n" );
//	t4params_1.pipe_in = thread_pipe_create( PIPE_SIZE );
	t4params_1.pipe_in = thread_pipe_create_from_file( "test_in.txt" );
	t4params_1.pipe_out = thread_pipe_create( PIPE_SIZE );
	t4params_2.pipe_in = t4params_1.pipe_out;
//	t4params_2.pipe_out = thread_pipe_create( PIPE_SIZE );
	t4params_2.pipe_out = thread_pipe_create_to_file( "test_out.txt", 1 );
	if( !t4params_1.pipe_in || !t4params_1.pipe_out || !t4params_2.pipe_in || !t4params_2.pipe_out ) {
		locked_printf( "pipe create failed\n" );
		return( -1 );
	}

	locked_printf( "Starting thread 4, copy 1\n" );
	t4params_1.threadnumber = 1;
	th1 = thread_start( test_thread4, (void *) &t4params_1 );
	if( !th1 ) {
		locked_printf( "Thread 4, copy 1 startup failed\n" );
		return( -1 );
	}

	locked_printf( "Starting thread 4, copy 2\n" );
	t4params_2.threadnumber = 2;
	th2 = thread_start( test_thread4, (void *) &t4params_2 );
	if( !th2 ) {
		locked_printf( "Thread 4, copy 2 startup failed\n" );
		return( -1 );
	}

//	locked_printf( "main, writing data to pipe\n" );
//	i = 0;
//	while( pipedata[i] ) {
//		thread_pipe_write( t4params_1.pipe_in, strlen( pipedata[i] ), pipedata[i] );
//		++i;
//	}

//	locked_printf( "main, closing the pipe we were writing to\n" );
//	thread_pipe_eof( t4params_1.pipe_in, 1 );

//	locked_printf( "main, reading data from pipe\n" );

//	while( !thread_pipe_iseof(t4params_2.pipe_out) ) {
//		bytes_read = thread_pipe_read( t4params_2.pipe_out, 1024, buffer );
//		if( !bytes_read ) {
//			break;
//		}
//		buffer[bytes_read] = 0;
//		locked_printf( "main, %d bytes read from pipe:\n%s\n", bytes_read, buffer );
//	}

	while(1)
		sleep(1);

	locked_printf( "Stoping copies of thread 4\n" );
	thread_stop( th1, 0 );
	thread_stop( th2, 0 );

	locked_printf( "Closing pipes\n" );
	thread_pipe_free( t4params_1.pipe_in );
	thread_pipe_free( t4params_1.pipe_out );
	thread_pipe_free( t4params_2.pipe_out);

	return 0;
}
