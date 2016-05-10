/*
 * Copyright (C) 2003  John Barrett <johnbarretthcc@hotmail.com>
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

/* "$Id: tygetopt.c,v 1.1 2003/03/30 12:24:55 mrbassman Exp $" */
/* Thread protection for the getopt library */

/* Include files */
#include "../libs/threadlib.h"
#include <getopt.h>

#if !defined(TIVO)
/* Mutex used to lock getopt usage */
static mutex_handle_t getopt_mutex = 0;
#endif


/* Lock and initialise the getopt library */
void tygetopt_lock()
{
#if !defined(TIVO)
	if( !getopt_mutex ) {
		getopt_mutex = thread_mutex_create();
	}
#endif

#if !defined(TIVO)
	thread_mutex_lock( getopt_mutex );
#endif

	/* Initialise getopt on the next call */
	optind = 0;
}

/* Release the lock on getopt */
void tygetopt_unlock()
{
#if !defined(TIVO)
	if( getopt_mutex ) {
		thread_mutex_unlock( getopt_mutex );
	}
#endif

}


