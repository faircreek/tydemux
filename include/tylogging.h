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

/* "$Id: tylogging.h,v 1.3 2003/03/29 13:28:19 mrbassman Exp $" */
/* Logging library */

#ifndef __TYLOGGING_H__
#define __TYLOGGING_H__

#ifdef __cplusplus
extern "C"{
#endif 

#include <stdio.h>

/* Module specific log line prefix, #define this before including this header file
 * i.e.
 * #define LOG_MODULE "transcode"
 */
#ifndef LOG_MODULE
#define LOG_MODULE ""
#endif

#define MAX_LOG_MESSAGE_LENGTH 1024

typedef enum {
	log_fatal = 0,
	log_error,
	log_warn,
	log_progress,
	log_info,
	log_diag,
	log_function,
	log_data
} loglevel_t;

/* User defined hook function
 * Return zero from this function if you do not want the message to be logged to
 * the log file */
typedef int (*log_hook_function) (int level, const char *module, const char *msg );

/* Initialise the logger
 * level - log level
 * logfile - logging file - you can use a real file or stdout or stderr use 0 for no file output
 * hookfn - Hook function if you want to redirect log messages elsewhere pass 0 to disable
 * Returns 0 on success, else an error code */
extern int logger_init( loglevel_t level, FILE *logfile, log_hook_function hookfn, int inthread );

/* shut down the logger */
extern void logger_free();

/* Change the logging level at run - time
 * level - new log level
 * Returns: previous log level */
extern loglevel_t logger_setlevel( loglevel_t level );

/* Return the current log level */
extern loglevel_t logger_get_log_level();

/* log some information - USE THE MACROS DEFINED BELOW WHENEVER POSSIBLE */
/* The file and line information is only logged in the debug version of the program */
extern void logger_log( loglevel_t level,
				    const char *module,
					const char *file,
					unsigned int line,
					const char *msg, ... );

/* This will pass the information on to the logging function together
 * with the file, function and line number where it occurs. 
 * LOG_MODULE is a macro the developer should define before #including this file
 */
#define LOG( level, msg ) \
	logger_log( level, LOG_MODULE, __FILE__, __LINE__, msg )
#define LOG1( level, msg, p1 ) \
	logger_log( level, LOG_MODULE, __FILE__, __LINE__, msg, p1 )
#define LOG2( level, msg, p1, p2 ) \
	logger_log( level, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2 )
#define LOG3( level, msg, p1, p2, p3 ) \
	logger_log( level, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2, p3 )
#define LOG4( level, msg, p1, p2, p3, p4 ) \
	logger_log( level, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2, p3, p4 )

/* macros to call LOG with the appropriate level */

/* Really serious errors which will cause an immediate program exit.
 * This function does not return to the caller.
 * always output */
#define LOG_FATAL( Msg ) \
	{ logger_log( log_fatal, LOG_MODULE, __FILE__, __LINE__, Msg ); exit(-1); }
#define LOG_FATAL1( msg, p1 ) \
	{ logger_log( log_fatal, LOG_MODULE, __FILE__, __LINE__, msg, p1 ); exit(-1); }
#define LOG_FATAL2( msg, p1, p2 ) \
	{ logger_log( log_fatal, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2 ); exit(-1); }
#define LOG_FATAL3( msg, p1, p2, p3 ) \
	{ logger_log( log_fatal, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2, p3 ); exit(-1); }
#define LOG_FATAL4( msg, p1, p2, p3, p4 ) \
	{ logger_log( log_fatal, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2, p3, p4 ); exit(-1); }

/* Errors which will cause a program exit but the exit is handled by the caller
 * output when log level >= log_error */
#define LOG_ERROR( Msg ) \
	logger_log( log_error, LOG_MODULE, __FILE__, __LINE__, Msg )
#define LOG_ERROR1( msg, p1 ) \
	logger_log( log_error, LOG_MODULE, __FILE__, __LINE__, msg, p1 )
#define LOG_ERROR2( msg, p1, p2 ) \
	logger_log( log_error, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2 )
#define LOG_ERROR3( msg, p1, p2, p3 ) \
	logger_log( log_error, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2, p3 )
#define LOG_ERROR4( msg, p1, p2, p3, p4 ) \
	logger_log( log_error, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2, p3, p4 )
#define LOG_ERROR5( msg, p1, p2, p3, p4, p5 ) \
	logger_log( log_error, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2, p3, p4, p5 )

/* Warnings about situations the program can recover from
 * output when log level >= log_warn */
#define LOG_WARNING LOG_WARN
#define LOG_WARNING1 LOG_WARN1
#define LOG_WARNING2 LOG_WARN2
#define LOG_WARNING3 LOG_WARN3
#define LOG_WARNING4 LOG_WARN4

#define LOG_WARN( Msg ) \
	logger_log( log_warn, LOG_MODULE, __FILE__, __LINE__, Msg )
#define LOG_WARN1( msg, p1 ) \
	logger_log( log_warn, LOG_MODULE, __FILE__, __LINE__, msg, p1 )
#define LOG_WARN2( msg, p1, p2 ) \
	logger_log( log_warn, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2 )
#define LOG_WARN3( msg, p1, p2, p3 ) \
	logger_log( log_warn, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2, p3 )
#define LOG_WARN4( msg, p1, p2, p3, p4 ) \
	logger_log( log_warn, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2, p3, p4 )

/* Comfort messages for the user.
 * output when log level >= log_progress */
#define LOG_PROGRESS( Msg ) \
 logger_log( log_progress, LOG_MODULE, __FILE__, __LINE__, Msg )
#define LOG_PROGRESS1( msg, p1 ) \
	logger_log( log_progress, LOG_MODULE, __FILE__, __LINE__, msg, p1 )
#define LOG_PROGRESS2( msg, p1, p2 ) \
	logger_log( log_progress, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2 )
#define LOG_PROGRESS3( msg, p1, p2, p3 ) \
	logger_log( log_progress, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2, p3 )
#define LOG_PROGRESS4( msg, p1, p2, p3, p4 ) \
	logger_log( log_progress, LOG_MODULE, __FILE__, __LINE__, msg, p1, p2, p3, p4 )

/* Placed at the top of major functions to log function entry
 * output when log level >= log_function */
#define LOG_FUNCTION( n ) \
 logger_log( log_function, LOG_MODULE, __FILE__, __LINE__, "Function entry: %s", n )

/* Placed at the exit point of major functions to log function return values.
 * output when log level >= log_function */
#define LOG_RESULT_INT( n ) \
 logger_log( log_function, LOG_MODULE, __FILE__, __LINE__, "Result: %d\r\n", n )
#define LOG_RESULT_UINT( n ) \
 logger_log( log_function, LOG_MODULE, __FILE__, __LINE__, "Result: %u\r\n", n )
#define LOG_RESULT_LONG( l ) \
 logger_log( log_function, LOG_MODULE, __FILE__, __LINE__, "Result: %l\r\n", l )
#define LOG_RESULT_ULONG( l ) \
 logger_log( log_function, LOG_MODULE, __FILE__, __LINE__, "Result: %lu\r\n", l )
#define LOG_RESULT_INT64( ll ) \
 logger_log( log_function, LOG_MODULE, __FILE__, __LINE__, "Result: %I64d\r\n", ll )
#define LOG_RESULT_STRING( s ) \
 logger_log( log_function, LOG_MODULE, __FILE__, __LINE__, "Result: %s\r\n", s )
#define LOG_RESULT_POINTER( p ) \
 logger_log( log_function, LOG_MODULE, __FILE__, __LINE__, "Result: %p\r\n", p )

/* User diagnostics, packet header information etc.
 * output when log level >= log_info */
#define LOG_USERDIAG( Msg ) \
 logger_log( log_info, LOG_MODULE, __FILE__, __LINE__, Msg )
#define LOG_USERDIAG1( Msg, p1 ) \
 logger_log( log_info, LOG_MODULE, __FILE__, __LINE__, Msg, p1 )
#define LOG_USERDIAG2( Msg, p1, p2 ) \
 logger_log( log_info, LOG_MODULE, __FILE__, __LINE__, Msg, p1, p2 )
#define LOG_USERDIAG3( Msg, p1, p2, p3 ) \
 logger_log( log_info, LOG_MODULE, __FILE__, __LINE__, Msg, p1, p2, p3 )
#define LOG_USERDIAG4( Msg, p1, p2, p3, p4 ) \
 logger_log( log_info, LOG_MODULE, __FILE__, __LINE__, Msg, p1, p2, p3, p4 )

/* Developer diagnostics, variable values etc.
 * output when log level >= log_diag */
#define LOG_DEVDIAG( Msg ) \
 logger_log( log_diag, LOG_MODULE, __FILE__, __LINE__, Msg )
#define LOG_DEVDIAG1( Msg, p1 ) \
 logger_log( log_diag, LOG_MODULE, __FILE__, __LINE__, Msg, p1 )
#define LOG_DEVDIAG2( Msg, p1, p2 ) \
 logger_log( log_diag, LOG_MODULE, __FILE__, __LINE__, Msg, p1, p2 )
#define LOG_DEVDIAG3( Msg, p1, p2, p3 ) \
 logger_log( log_diag, LOG_MODULE, __FILE__, __LINE__, Msg, p1, p2, p3 )
#define LOG_DEVDIAG4( Msg, p1, p2, p3, p4 ) \
 logger_log( log_diag, LOG_MODULE, __FILE__, __LINE__, Msg, p1, p2, p3, p4 )
#define LOG_DEVDIAG5( Msg, p1, p2, p3, p4, p5 ) \
 logger_log( log_diag, LOG_MODULE, __FILE__, __LINE__, Msg, p1, p2, p3, p4, p5 )

/* Seriously heavy logging such as Packet data dumps.
 * In the release version, this macro is nulled for performance reasons.
 * output when log level >= log_data */
#ifdef NDEBUG
#define LOG_DEVDATA( msg )
#else
#define LOG_DEVDATA( msg ) \
 logger_log( log_data, LOG_MODULE, __FILE__, __LINE__, msg )
#endif

#ifdef __cplusplus
}
#endif

#endif // __TYLOGGING_H__
