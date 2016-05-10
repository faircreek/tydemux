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

#ifndef __TYSTUDIO_TYPES__
#define __TYSTUDIO_TYPES__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/****************************** Windows ******************************/

#ifdef WIN32

#ifndef _INT_TYPES_H_
#define _INT_TYPES_H_

typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;

#endif /* _INT_TYPES_H_ */

#define off64_t	__int64
typedef struct _stati64 stat64_t;

/******************************** BSDI *******************************/

#elif defined(__bsdi__)

#define uint64_t u_int64_t
#define uint32_t u_int32_t
#define uint16_t u_int16_t
#define uint8_t u_int8_t

/****************************** Max OS X *****************************/

#elif defined(__APPLE__)

#include <inttypes.h>

#define uint64_t u_int64_t
#define uint32_t u_int32_t
#define uint16_t u_int16_t
#define uint8_t u_int8_t
#define off64_t off_t

typedef struct stat stat64_t;

/**************************** linux & TiVo ***************************/
#else 


#if !defined(TIVO)
#include <inttypes.h>
#else

#define int64_t long long 
#define int32_t int
#define int16_t short
#define int8_t char
#define uint64_t unsigned long long 
#define uint32_t unsigned int 
#define uint16_t unsigned short
#define uint8_t  unsigned char
#endif

#define off64_t off_t
typedef struct stat stat64_t;

#endif /* Not WIN32 */

#endif /* __TYSTUDIO_TYPES__ */
