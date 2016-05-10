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

/* "$Id: logging.c,v 1.20 2003/03/30 04:41:43 olaf_be Exp $" */
/* Logging library */

/* Include files */

#include <stdarg.h>
#include <string.h>

#include "tycommon.h"


#define LOG_MODULE "logger"
#include "tylogging.h"
#include "tymalloc.h"

#if !defined(TIVO)
#include "threadlib.h"
#endif

typedef struct {
	loglevel_t level;
	FILE *logfile;
	log_hook_function hookfn;
	int inthread;
} logger_params_t;

#if defined(_WIN32)
TY_THREAD_LOCAL static logger_params_t *s_params = NULL;
#else
#if defined(TIVO)
static logger_params_t *s_params = NULL;
#else 
#include <pthread.h>
static pthread_key_t s_threadkey = 0;
#endif
#endif

static logger_params_t default_params = {
	log_warn,
	NULL,
	NULL,
	0
};

#if !defined(TIVO)
/* Mutex used to lock logging file output */
static mutex_handle_t log_mutex = 0;
#endif


/* Initialise the logger
 * level - log level
 * logfile - logging file - you can use a real file or stdout or stderr use 0 for no file output
 * hookfn - Hook function if you want to handle the logging yourself
 * Returns 0 on success, else an error code */
int logger_init( loglevel_t level, FILE *logfile, log_hook_function hookfn, int inthread )
{
	logger_params_t *params = (logger_params_t *) malloc( sizeof(logger_params_t) );
	params->level = level;
	params->logfile = logfile;
	params->hookfn = hookfn;
	params->inthread = inthread;

#if defined(_WIN32) || defined(TIVO)
	/* TLS storage so this is safe or not multi-threaded so there will be only one anyway */
	s_params = params;
#else
	/* Have to use pthread keys to make the logger variables thread local */
	if( !s_threadkey ) {
 		pthread_key_create( &s_threadkey, NULL );
	}
	pthread_setspecific( s_threadkey, params ); 
#endif
	return 0;
}

static logger_params_t *getParams()
{
	logger_params_t *params;
#if defined(_WIN32) || defined(TIVO)
	params = s_params;
#else
	if( s_threadkey ) {
		params = (logger_params_t *) pthread_getspecific( s_threadkey ); 
	} else {
		params = 0;
	}
#endif
	if( !params ) {
		params = &default_params;
		params->logfile = stderr;
	}

	return( params );
}

/* Uninitialise the logger */
void logger_free()
{
	logger_params_t *params;
#if defined(_WIN32) || defined(TIVO)
	params = s_params;
	s_params = NULL;
#else
	if( s_threadkey ) {
		params = (logger_params_t *) pthread_getspecific( s_threadkey ); 
	} else {
		params = 0;
	}
#endif
	if( params ) {
		free( params );
	}
}

/* Change the logging level at run - time
 * level - new log level
 * Returns: previous log level */
loglevel_t logger_setlevel( loglevel_t level )
{
	loglevel_t oldlevel;
	logger_params_t *params = getParams();
	
	oldlevel = params->level;
	params->level = level;

	return( oldlevel );
}

/* Return the current log level */
loglevel_t logger_get_log_level()
{
	logger_params_t *params = getParams();
	return (params->level);
}

void logger_log( loglevel_t level, const char *module, const char *file,
					unsigned int line, const char *msg, ... )
{
	int log_to_file = 1;
	char outputmsg[ MAX_LOG_MESSAGE_LENGTH + 1 ];
	int headerlen = 0;
	logger_params_t *params = getParams();

	va_list argptr;
	va_start(argptr, msg);
#if !defined(TIVO)
	if( !log_mutex )
		log_mutex = thread_mutex_create();
#endif

	if( level <= params->level ) { 

#if !defined(TIVO)
		thread_mutex_lock( log_mutex );
#endif
		if( level != log_progress ) {
			/* output the source of the log message */
#ifdef NDEBUG
			snprintf( outputmsg, MAX_LOG_MESSAGE_LENGTH, "%s: ", module );
#else
			/* strip off any path element on the file */
			if( strrchr( file, '\\' ) ) {
				file = strrchr( file, '\\' ) + 1;
			}
			if( strrchr( file, '/' ) ) {
				file = strrchr( file, '/' ) + 1;
			}
			snprintf( outputmsg, MAX_LOG_MESSAGE_LENGTH, "%s:%s:%d: ", module, file, line );
#endif
			headerlen = strlen( outputmsg );
		}

		/* Now output the message itself */
		vsnprintf( &(outputmsg[headerlen]), MAX_LOG_MESSAGE_LENGTH - headerlen, msg, argptr );

		if( params->hookfn ) {
			log_to_file = (*(params->hookfn))( level, module, outputmsg );
		}
		if( log_to_file && params->logfile ) {
			fprintf( params->logfile, outputmsg );
			fflush( params->logfile );
		}
#if !defined(TIVO)
		thread_mutex_unlock( log_mutex );
#endif

		/* Exit if the error is fatal */
		if( level == log_fatal ) {
#if !defined(TIVO)
			if( params->inthread ) {
				thread_exit( -1 );
			} else {
#endif
				exit(-1);
#if !defined(TIVO)
			}
#endif
		}
	}
}


