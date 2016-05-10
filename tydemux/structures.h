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

/* Remote tyserver dep structures */

#ifndef __TYDEMUX_STRUCTURES_H__
#define __TYDEMUX_STRUCTURES_H__

#include "tynet.h"

typedef struct Fsid_Index_t fsid_index_t;

struct Fsid_Index_t {

	/* ID of a recording */
	int fsid;

	/* Tystream file name */
	char * tystream;

	/* State - rec or not */
	int state;

	/* Year */
	char * year;

	/* Air date */
	char * air;

	/* Rec Day */
	char * day;

	/* Rec Date */
	char * date;

	/* Time */
	char * rectime;

	/* Duration */
	char * duration;


	/* Title of recoding */
	char * title;

	/* Episode of recoding if present */
	char * episode;

	/* Episode number */
	char * episodenr;

	/* Description of recoding */
	char * description;

	/* Actors  if present */
	char * actors;

	/* Gueststar */
	char * gstar;

	/* Host */
	char * host;

	/* Director */
	char * director;

	/* ExecProd */
	char * eprod;

	/* Producer */
	char * prod;

	/* Writer */
	char * writer;

	/* Remote index 
	avalible or not */
	int index_avalible;


	fsid_index_t * next;
	fsid_index_t * previous;

};


typedef struct {

	/* Fsid index */
	fsid_index_t * fsid_index;

	/* Hostname */
	char * hostname;

	/* Socket to remote host */
	SOCKET_FD sockfd;

	/* Stream */
	SOCKET_HANDLE inpipe;
	SOCKET_HANDLE outpipe;

} remote_holder_t;


/*************************************************
* Functions and structs to read a X bytes/bits
* from memory - just like a file
************************************************/

typedef struct {

	/* Size of stream in bytes */
	off64_t size;

	/* Start of stream */
	uint8_t * start_stream;

	/* End of stream */
	uint8_t * end_stream;

	/* Current pos */
	uint8_t * current_pos;

	int current_bit;

	int eof;

} vstream_t;


/* Struct to hold index info
to be used by the cut function
*/


typedef struct Gop_index_t gop_index_t;

struct Gop_index_t {

	/* Note it starts on 1 */
	int64_t gop_number;

	/* Chunk number of seq */
	chunknumber_t chunk_number_seq;

	/* Record number for SEQ */
	int seq_rec_nr;

	/* Chunk number of I/SEQ PES*/
	chunknumber_t chunk_number_i_frame_pes;

	/* Record number of I/SEQ PES */
	int i_frame_pes_rec_nr;

	/* Chunk number of I-Frame */
	chunknumber_t chunk_number_i_frame;

	/* Record number of I-Frame */
	int i_frame_rec_nr;

	/* PTS of I frame */
	ticks_t time_of_iframe;

	/* Display time of GOP real_start_gop_index->display_time is 0 */
	ticks_t display_time;
	
	/* For future use - we want to detect cutpoints */
	int reset_of_pts;

	/* Access pointers */
	gop_index_t * next;
	gop_index_t * previous;

};


/* Top access holder of gop index */

typedef struct {

	/* number of gops present in the
	index */
	int64_t nr_of_gops;

	/* The index it self */
	gop_index_t * gop_index;

	/* The last index point */
	gop_index_t * last_gop_index;

	/* The real start with - consideration 
	taken to resets of PTS */
	gop_index_t * real_start_gop_index;

	/* The real end with - consideration
	taken to resets of PTS */
	gop_index_t * real_end_gop_index;

} index_t;


typedef struct {

	ticks_t start_drift;
	ticks_t end_drift;

} drift_t;


typedef struct {

	ticks_t cutstart;
	ticks_t cutstop;

	chunknumber_t chunkstart;
	int rec_nr_start;

	chunknumber_t chunkstop;
	int rec_nr_stop;

	int cut_is_start_of_stream;
	int cut_is_end_of_stream;

	int audiostart;
	int audiostop;

	ecut_t cuttype;

} cutpoint_t;

typedef cutpoint_t * cutpointptr_t;

typedef struct {

	int nr_of_cutpoints;
	cutpointptr_t * cut_array;

} cuts_t;



/**
 * The chunk i.e. the TyPacket
 * and information nessesary for
 * the spliting operation
 */


/**
 * Record Header.
 *
 * The record header, holds information
 * about it's coresponing record.
 */

typedef struct {

	/* Size of the record */
	uint32_t  size;
	/* The type of record see
	the TyStream white paper */
	uint16_t  type;
	/* If the type is a data
	type the data will be stored
	in this array */
	uint8_t * extended_data;

	/* See the TyStream white paper
	for a description - not used */
	//uint32_t  junk1;
	//uint64_t  junk2;
} record_header_t;

/**
 * Record
 *
 * And array holding the data payload of a
 * for a coresponding record header
 */

typedef struct {
	uint8_t * record;
} record_t;


/**
 * The chunk (ty_packet) it self
 */

typedef struct Chunk_t chunk_t;

struct Chunk_t {

	/* Video times in ticks
	Used when doing a rough
	checking of sync  */
	ticks_t med_ticks;
	ticks_t last_tick;
	ticks_t first_tick;

	/* Indicator that this is
	a junk chunk in the junk
	chunk buffer */
	int junk;

	/* Indicator of gap/hole
	between this chunk and the
	previous chunk*/
	int gap;
	int small_gap;

	/* ???? FIXME */
	indicator_t indicator;

	/* Indicator if SEQ header is
	prsent in chunk */
	int seq_present;

	/* Chunk number from start
	of tystream */
	chunknumber_t chunk_number;

	/* Chunk info */
	uint16_t nr_records;
	uint16_t seq_start;

	/* Records and record headers */
	record_header_t * record_header;
	record_t * records;

	/* Access pointers */
	chunk_t * next ;
	chunk_t * previous;
};


typedef struct Payload_t payload_t;

struct Payload_t {

	/* Size of payload */
	int size;

	/* Type of payload */
	payload_type_t payload_type;

	/* Temporal reference */
	uint16_t tmp_ref;

	/* Temporal referece added */
	int tmp_ref_added;

	/* Payload data buffer */
	uint8_t * payload_data;
	
	field_t fields;

	/* Access pointers */
	payload_t * next;
	payload_t * previous;

};

typedef struct Pes_Holder_t pes_holder_t;

struct Pes_Holder_t {

	/* Type of data */
	media_type_t media;

	/* PES PTS time */
	ticks_t time;

	/* Total size of
	pes including pes header */
	int total_size;

	/* Size of payload */
	int size;

	/* Indicator of gap/hole
	between this pes and the
	previous pes*/
	int gap;

	chunknumber_t chunk_nr;

	/* Indicator that video pes
	holder is fixed /checked
	for framerate / temp ref */
	int fixed;
	int seq_added;
	int seq_fixed;
	int this_seq;
	seqnumber_t seq_number;

	/*Indicator if we have made a repair */
	int repaired;

	/* Indicator if we need to make it a closed
	gop - this is when we make a cut */
	int make_closed_gop;


	/* Video indicators */
	int seq_present;
	int gop_present;
	int i_frame_present;
	int p_frame_present;
	int b_frame_present;
	int audio_present;

	/* Data indicator */
	int data_present;

	/* If we need to repeat audio */
	int repeat;

	/* Drift object */
	ticks_t drift;

	/* Access pointer to payload
	linked list */
	payload_t * payload;

	/* Access points */
	pes_holder_t * next;
	pes_holder_t * previous;

};

typedef struct Start_Code_Array_t start_code_array_t;

struct Start_Code_Array_t {

	int size;
	uint8_t * start_code;
};



typedef struct Tystream_Holder_t tystream_holder_t;

struct Tystream_Holder_t {

	/* The chunk linked list */
	chunk_t * chunks;

	/* The junk chunk linked list */
	chunk_t * junk_chunks;

	/* The video data linked list */
	pes_holder_t * pes_holder_video;

	/* Pointer to last SEQ that has
	 fixed SEQ/GOP moved to I frame */
	pes_holder_t * lf_seq_pes_holder;

	/* Pointer to last SEQ that has
	fixed/check temporal reference */
	pes_holder_t * lf_tmp_ref_pes_holder;

	/* Pointer to last SEQ that has
	fixed P frame on SA 2.x */
	pes_holder_t * lf_p_frame_pes_holder;

	/* Pointer to last SEQ that has
	fixed/check frame rate */
	pes_holder_t * lf_field_pes_holder;

	/* Pointer to last SEQ that has
	fixed/check a/v sync drift */
	pes_holder_t * lf_av_drift_pes_holder_video;

	/* Pointer to last SEQ that has
	fixed the GOP header */
	pes_holder_t * lf_gop_frame_counter_pes_holder_video;

	/* The audio data linked list */
	pes_holder_t * pes_holder_audio;


	/**
	 * Global data needed by various functions
	 */

	/* Tivo type SA or DTIVO */
	tivo_type_t tivo_type;

	/* Tivo version */
	tivo_version_t tivo_version;

	/* Tivo series */
	tivo_series_t tivo_series;


	/**
	 * Audio data
	 */

	/* Audio check */
	uint16_t wrong_audio;
	uint16_t right_audio;

	/* Average audio record size */
	int med_size;

	/* Known standard audio frame size*/
	int audio_frame_size;

	/* Know standard audio record size */
	int std_audio_size;

	/* Average ticks between audio frames */
	ticks_t audio_median_tick_diff;

	/* Tivo audio type */
	tivo_audio_type_t  audio_type;

	/* Start code for audio pes */
	start_codes_t audio_startcode;

	/* Marker of DTIVO broken pes header */
	int DTIVO_MPEG;


	/**
	 * Video data
	 */

	/* Video sync */
	ticks_t med_ticks;
	ticks_t last_tick;


	/**
	 * Misc
	 */

	/* Junk chunk count */
	chunknumber_t cons_junk_chunk;


	/* Have we probed the tystream or not */
	int probed;

	/* Start code array used to find start
	codes in the ty_stream*/
	start_code_array_t * start_code_array;

	/* How many chunks we have read */
	chunknumber_t number_of_chunks;

	/* Audio out file desciptor in demux mode */
	int audio_out_file;

	/* Video out file desciptor in demux mode */
	int video_out_file;

	/* Out file desciptor in remux mode */
	int out_file;

	/* Input tystream */
	int in_file;

	/* File names of in and out files in std alone mode */
	char * infile;
	char * audio_out;
	char * video_out;


	/* Size of file */
	off64_t in_file_size;

	/* Operation mode */
	operation_mode_t mode;

	/*Start chunk - used in get_start_chunk */
	chunknumber_t start_chunk;


	/* Repair mode */
	int repair;

	/* Have we found starting SEQ header */
	int find_seq;

	/* Time diff between audio/video start */
	ticks_t time_diff;

	/* Indicator if audio starts early (positive)
	or late (negative) in relation to video */
	indicator_t indicator;

	/* Frame Rate */
	uint8_t frame_rate;


	/* Times used for checking sync drift FIXME */
	ticks_t last_time;

	/* Odd or even number of frames in a SEQ/GOP */
	int reminder;
	field_type_t last_field;
	ticks_t  reminder_field;


	/* The sync drift it self */
	ticks_t drift;

	/* FIXME Still used in repair Frame rate tick diff used to
	messure sync drift with NOTE:
	it's the tick diff between frame 1
	and 3 in a string of three frames */
	ticks_t tick_diff;

	/* Median diff between to frames at frame
	rate X */
	ticks_t frame_tick;

	/* Threshold for a/v drift when to add remove
	frames */
	ticks_t drift_threshold;

	/* FIXME S2 when cut and S2 DTivo The first GOP on a S2 is closed even
	if it's not maked as closed */
	int first_gop;

	/* Write to file or not */
	int write;

	/* SEQ counter */
	seqnumber_t seq_counter;

	/*Byte offset if the chunks are
	miss alinged */
	off64_t byte_offset;
	int miss_alinged;

	/* Holder of index and cut list */
	cuts_t * cuts;

	index_t * index;

	/* Audio cut markers */
	int audiocut;
	int64_t audio_start_drift;

	/* Vstream used if we do probing
	from stdin */
	vstream_t * vstream;

	/* Tivo probe indicator */
	int tivo_probe;


#if !defined(TIVO)
	pipe_handle_t input_pipe;			/* Input pipe to use */
	pipe_handle_t audio_pipe;			/* Audio output pipe to use */
	pipe_handle_t video_pipe;			/* Video output pipe to use */
#endif

	/* Are we running this as a lib or not */
	int std_alone;
	/* Will we stream the tystream as cbr mpeg */
	int multiplex;


	/* Remote management */
	int64_t nr_of_remote_chunks;
	remote_holder_t * remote_holder;
	int fsid;

	
	/* Frame count and needed by 
	check_fix_gop_frame_counter */
	int frame_counter; /* The PTS will be rest a long time before this one - falls over */
	int frame_field_reminder;


#ifdef FRAME_RATE
	/* NOT USED */

	/* test of add del b frames */

	chunknumber_t nr_b_frames_drop;
	chunknumber_t nr_b_frames_add;

	/* Variables to print frame rate changes */
	int print_frame_rate;
	uint8_t present_frame_rate;

	/* Frame counter for removal add of
	frames when correcting frame rate */
	uint16_t frame_counter;

	/* Times for checking frame rate */
	int time_init;
	int time_init_second;
	int time_init_third;

	/* Frame added/removed - to make
	sure we balance removal/adding of
	frames */
	int frame_removed;
	int frame_added;
#endif

};





typedef struct {

	int pes_buffer_size;
	uint8_t * pes_data_buffer;
	int data_buffer_size;
	uint8_t * data_buffer;

} audio_module_t;



typedef struct {
	int size;
	int last_record_size;
	int last_record_offset;
} last_record_info_t;


typedef struct {
	int buffer_size;
	uint8_t * data_buffer;
} module_t;


typedef struct {
	ticks_t data_stop;
	ticks_t data_start;
} data_times_t;



/**
 * Structure to holde picture info
 * that is used during sync, frame
 * and intergity checks
 */

typedef struct {


	/* Is the first field of
	a frame the top field */
	uint8_t top_field_first;

	/*Should be repeate the first
	field - used during 3:2 down
	pull */

	uint8_t repeat_first_field;

	/* Is the frame progressive or not */
	uint8_t progressive_frame;

} picture_info_t;



/* Stuctur to hold info
 if a frame is missing or not
 if not and the frame type */



typedef struct {

	int present;

	payload_type_t frame_type;

	picture_info_t * picture_info;

} tmp_ref_frame_t;

#if !defined(TIVO)

typedef struct {
	int	argc;					/* Additional tydemux parameters from the command line */
	char **argv;					/* As above */

	int running;					/* Set to true once the tydemux thread starts */
	mutex_handle_t thread_lock;			/* Used to lock the main program until tydemux has finished */

	pipe_handle_t input_pipe;			/* Input pipe to use */
	pipe_handle_t audio_pipe;			/* Audio output pipe to use */
	pipe_handle_t video_pipe;			/* Video output pipe to use */

	char **cutlist;					/* List of cuts */
	int cutcount;					/* Number of cuts in the list */

	int *wait_for_audio_offset;			/* If set, mplex will block waiting us to change it to zero */
	int *audio_offset;				/* Audio offset to be used in mplex */
	int *progress;

	remote_holder_t * remote_holder;		/* holder of open pipes to the tyserver */
	int fsid;					/* The fsid we will demux */

	log_hook_function		loghookfn;
	loglevel_t			loglevel;
	FILE	*			logfile;

} demux_start_params_t;

#endif



typedef struct Dir_Index_t dir_index_t;

struct Dir_Index_t {

	char * filename;

	dir_index_t * next;
	dir_index_t * previous;

};

#endif /* __TYDEMUX_STRUCTURES_H__ */

