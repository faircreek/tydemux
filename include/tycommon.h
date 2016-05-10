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

#ifndef __TYSTUDIO_COMMON__
#define __TYSTUDIO_COMMON__

#include <stdlib.h>
#include <fcntl.h>
/****************************** ******* *****************************/
/****************************** Windows *****************************/
/****************************** ******* *****************************/

#ifdef WIN32

#define NEWLINE "\r\n"

#include <windows.h>
#include <memory.h>
#include <share.h>
#include <io.h>

#define llabs(ll) ( ((ll)<0) ? (ll)*-1 : (ll) )

#define read _read
#define write _write
#define open _open
#define creat _creat
#define sopen _sopen
#define fsopen _fsopen
#define lseek _lseek
#define dup _dup
#define atoll _atoi64
#define snprintf _snprintf
#define vsnprintf _vsnprintf

#define sleep(s) Sleep(s * 1000)

#define S_ISREG(m) (m & _S_IFREG)

/* _O_BINARY mode when opening files */

#define OS_FLAGS _O_BINARY
#define READ_MODE "rt"
#define WRITE_MODE "wt"
#define APPEND_MODE "at"
#define READWRITE_PERMISSIONS (_S_IREAD | _S_IWRITE )
#define READ_PERMISSIONS (_S_IREAD)
#define SHARE_DENY_NONE (_SH_DENYNO)

#define OPEN_WRITE_FLAGS ( _O_WRONLY | _O_CREAT | _O_APPEND | _O_BINARY)
#define OPEN_CREATE_FLAGS (_O_WRONLY | _O_TRUNC | _O_CREAT | _O_BINARY)
#define OPEN_READONLY_FLAGS (O_RDONLY | _O_BINARY)
#define OPEN_WRITE_PERMISSION (_S_IREAD | _S_IWRITE )

/* 64 bit file access */

#define fstat64(h,s) _fstati64(h,s)
#define lseek64(h,off,orig) _lseeki64(h,off,orig)

#define strncasecmp(a, b, c) strnicmp(a, b, c) 
#define vsnprint _vsnprintf

#ifndef MAXPATHLEN
#define MAXPATHLEN MAX_PATH
#endif

#define I64FORMAT "%I64d"

#define TY_THREAD_LOCAL __declspec( thread )

/****************************** ****** *****************************/
/****************************** Unixen *****************************/
/****************************** ****** *****************************/


#else  /* Not Win32 */

#define NEWLINE "\n"
#include <errno.h>
#include <unistd.h>

#include <sys/param.h>


/* ISSUE --thread is supposed to be supported in gcc 3.2.2
 * but i still get compile errors - need to resolve this
 * MrBassMan
#define TY_THREAD_LOCAL __thread
*/
#define TY_THREAD_LOCAL

#define I64FORMAT "%lld"


#define SH_COMPAT 0 
#define SH_DENYRW 0 
#define SH_DENYWR 0 
#define SH_DENYRD 0 
#define SH_DENYNO 0 
#define sopen(name,oflag,shflag) open(name,oflag)
#define fsopen(name,oflag,shflag) fopen(name,oflag)

#define OS_FLAGS 0
#define READ_MODE "r"
#define WRITE_MODE "w"
#define APPEND_MODE "a"
#define READWRITE_PERMISSIONS (S_IRUSR|S_IWUSR)
#define READ_PERMISSIONS (S_IRUSR)
#define SHARE_DENY_NONE (SH_DENYNO)

#define OPEN_WRITE_FLAGS ( O_WRONLY | O_CREAT | O_APPEND )
#define OPEN_CREATE_FLAGS ( O_WRONLY | O_TRUNC | O_CREAT)
#define OPEN_READONLY_FLAGS O_RDONLY
#define OPEN_WRITE_PERMISSION ( S_IREAD | S_IWRITE )



#ifndef MAXPATHLEN
#if defined(__linux__)
#include <linux/limits.h>
#endif
#ifdef PATH_MAX
#define MAXPATHLEN PATH_MAX
#else
#define MAXPATHLEN 256
#endif /*PATH_MAX*/
#endif /*MAXPATHLEN*/

#if !defined(__bsdi__)
/* Fix causes compile error on modern systems*/
//#define fstat64(h,s) fstat(h,s)
#define lseek64(h,off,orig) lseek(h,off,orig)

#endif


/****************************** Max OS X *****************************/

#if defined(__APPLE__)

#define atoll(nptr) strtoll(nptr, (char **) NULL, 10);
#define llabs(ll) ( ((ll)<0) ? (ll)*-1 : (ll) )

#endif


#endif /* Not WIN32 */

#endif /* __TYSTUDIO_COMMON__ */
