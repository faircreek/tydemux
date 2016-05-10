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

/* include this file in your project when you need it */

/* Quick and dirty fix - just get single characters until \n is seen */
static int socket_gets( char *buffer, int len, SOCKET_HANDLE socket )
{
	int done = 0, readcount = 0, res;
	while( readcount < len && !done ) {
		res = recv( socket, buffer, 1, 0 );
		if( res == 0 || res == SOCKET_ERROR ) {
			if( res == SOCKET_ERROR ) {
				LOG_ERROR1( "Winsock error in recv: %d\n", WSAGetLastError() );
			}
			done = 1;
			break;
		}
		if( *buffer == '\n' ) {
			done = 1;
		}
		++buffer;
		++readcount;
	}
	return( readcount );
}

