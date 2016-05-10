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

#ifndef __TYSTUDIO_NET__
#define __TYSTUDIO_NET__

/****************************** Windows *****************************/

#ifdef WIN32

#include <winsock.h>
#define sockopt_valptr_t const char *
#define socket_write(s,b,l) send( s,b,l,0)
#define socket_read( b,c,l,s )	recv( s, b, l, 0);
#define socket_open_handle(s,m) s;
#define socket_close_handle(s)
#define socket_close(s) closesocket(s);

typedef SOCKET SOCKET_FD;
typedef SOCKET SOCKET_HANDLE;

#define NULL_SOCKET_HANDLE 0

extern int socket_gets( char *buffer, int len, SOCKET_HANDLE socket );

/****************************** Unixen *****************************/

#else  /* Not WIN32 */

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define sockopt_valptr_t void *
#define socket_write(s,b,l) write(s,b,l)
#define socket_read( b,c,l,s )	fread(b, c, l, s);
#define socket_gets(b,l,s) fgets(b,l,s)
#define socket_open_handle(s,m) fdopen(s,m);
#define socket_close_handle(s)	fclose(s);
#define socket_close(s) close(s);

typedef int SOCKET_FD;
typedef FILE * SOCKET_HANDLE;

#define INVALID_SOCKET -1
#define NULL_SOCKET_HANDLE NULL

#endif /* Not WIN32 */

#endif /* __TYSTUDIO_NET__ */
