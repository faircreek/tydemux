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

#ifndef __TYDEMUX_ENUMS_H__
#define __TYDEMUX_ENUMS_H__

#include "tytypes.h"

#define MAJOR_VER 0
#define MINOR_VER 5
#define SUB_VER 0

/* Defined values */
#define PTS_CLOCK 90000
#define PTS_TIME_OFFSET 9
#define PES_MIN_PTS_SIZE 14
#define CHUNK_SIZE 131072
#define STD_VIDEO_MED_TICK 4505
#define JUNK_CHUNK_BUFFER_SIZE 30
#define INITAL_MIN_TICKS 10000
#define TYRECORD_HEADER_SIZE 16
#define START_CODE_ARRAY 12
#define START_CODE_SIZE 4
#define MAX_RECORDS 1000

typedef enum {
	DEMUX = 0,
	REMUX = 1
} operation_mode_t;

typedef enum {
	DTIVO = 1,
	SA    = 2,
	UNDEFINED_TYPE = 100
} tivo_type_t;

typedef enum {
	V_13   = 1,
	V_2X   = 2,
	UNDEFINED_VERSION = 100
} tivo_version_t;

typedef enum {
	S1 = 0,
	S2 = 1,
	UNDEFINED_SERIES = 100
} tivo_series_t;


typedef enum {
	NEGATIVE = 1,
	POSITIVE  = 0
} indicator_t;

typedef enum {
	TBT,
	BTB,
	TB,
	BT,
	UNKNOWN_FIELD
} field_t;


typedef enum {

	/* NR of elements == START_CODE_ARRAY */

	/* MPEG video pes packet start code */
	MPEG_PES_VIDEO   = 0,

	/* MPEG Video start codes */
	MPEG_SEQ   = 1,
	MPEG_GOP   = 2,
	MPEG_I     = 3,
	MPEG_P     = 4,
	MPEG_B     = 5,

	/* MPEG /AC3 Audio start codes */
	MPEG_PES_AUDIO = 6,
	AC3_PES_AUDIO  = 7,
	MPEG_AUDIO     = 8,
	AC3_AUDIO      = 9,
	
	/* MPEG Extensions */
	MPEG_EXT       = 10,

	UNDEFINED_CODE      = 100

} start_codes_t;


typedef enum {
	VIDEO = 1,
	AUDIO = 0
} media_type_t;

typedef enum {

	/* VIDEO TYPES */
	PES_VIDEO  = 0,
	SEQ_HEADER = 1,
	GOP_HEADER = 2,
	I_FRAME	   = 3,
	P_FRAME	   = 4,
	B_FRAME	   = 5,

	/* AUDIO TYPES */
	PES_MPEG   	 = 6,
	MPEG_AUDIO_FRAME = 7,
	PES_AC3		 = 8,
	AC3_AUDIO_FRAME	 = 9,

	/* DATA TYPES */
	CLOSED_CAPTION	 = 10,
	XDS_DATA	 = 11,
	IPREVIEW_DATA	 = 12,
	TELETEXT_DATA	 = 13,

	UNKNOWN_PAYLOAD	 = 100

} payload_type_t;


typedef enum {

	/* TIVO AUDIO TYPES */

	/* DTivo */
	DTIVO_AC3 = 1,
	DTIVO_MPEG_1 = 2,
	DTIVO_MPEG_2 = 3,
	DTIVO_MPEG_3 = 4,
	DTIVO_MPEG_4 = 5,

	/* SA */
	SA_MPEG = 6,

	AUDIO_TYPE_UNKNOW = 100

} tivo_audio_type_t;

typedef enum {

	/* Type of last field in a frame */
	BOTTOM_FIELD 	= 0,
	TOP_FIELD	= 1,
	UNKNOW_FIELD	= 100
} field_type_t;

typedef enum {
	CUTPOINT_NONE = 0,
	CUTTYPE_REMOVE,
	CUTTYPE_CHAPTER,   /* Future use */
	CUTTYPE_SYSTEM
} ecut_t;

#if !defined(TIVO)
/* 64 bit file access */
#ifdef _WIN32
typedef struct _stati64 stat64_t;
#define uint64_t unsigned __int64
#endif
#endif

typedef uint64_t streamtime_t;

/* Chunk number - for large files */
typedef int64_t chunknumber_t;

/* Ticks are actually a 33bit unsigned int hence a int64_t will be just fine */
typedef int64_t ticks_t;

/* SEQ number */
typedef int64_t seqnumber_t;

#endif /* #define __TYDEMUX_ENUMS_H__ */
