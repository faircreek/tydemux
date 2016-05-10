
/*
 * Copyright (C) 2002, 2003, 2015  Olof <jinxolina@gmail.com>
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

#ifdef __cplusplus
extern "C" {
#endif

#include "enums.h"
#if !defined(TIVO) && !defined(TYP)
#include "../libs/threadlib.h"
#endif
#include "structures.h"


#if !defined(TIVO)
extern int transcode_thread( void *user_data );
#endif
#ifdef TIVO
long long atoll(const char *nptr);
#endif

#ifdef setbit
#undef setbit
#endif

#define TYDEMUX_VERSION "0.5.0"

/*****************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* From time.c
 * Functions to get time values from different components in a tystream */

/* FIXME ADD DOC */
ticks_t get_time_Y_video(const int record_nr, const chunk_t * chunk, tystream_holder_t * tystream);



/**
 * FIXME Code duplication doesn't work on S2!!!
 *
 * get_time_X will give you the time in ticks for a specific audio or video pes header in
 * in the chunk. NOTE: video and audio pes header are counted differently (use get_nr_audio/video_pes)
 * NOTE: Counting starts from ZERO hence to get the first pes header time pes_number == 0.
 *
 * @return Return time in ticks, on failiur 0 is returned (very unlikly that we have 0 ticks)
 * @params pes_number the pes you want the time from, chunk is the tyStream chunk and media is
 * either VIDEO or AUDIO depedning what pes header you are after.
 */

ticks_t get_time_X(const int pes_number, const chunk_t * chunk, const media_type_t media,
		tystream_holder_t * tystream );





/**
 * get_time will give you the PTS or DTS time present in a PES header - the pt pointer must
 * be located at the start of either the PTS or DTS field in the PES header. This function is
 * primarily used internaly but may be useful for other programs too. NOTE: There is no
 * error checking in this function what so ever.
 *
 * @return Returns the time in ticks present in the PTS or DTS field in a PES header
 * @params The pt buffer should hold a DTS or PTS time field aliged at byte 0.
 */

ticks_t get_time(uint8_t * pt);

/**
 * FIXME return value
 *
 * get_time_field_info will give you the field info and time of a specific tmp ref in
 * a seq_pes_holder - the pes that you feed this function must have a valid SEQ/GOP and I-Frame
 * note that the pes_holder returned from parse_chunk doesn't have that unless you have fixed it!
 * fixing of seq_pes_holders are done with fix_seq_header.
 *
 * @return Returns 1 on success and 0 on failure
 * @params Tystream, the tystream_holder you operate on, seq_pes_holder the seq that you are
 * searching the tmp ref in, tmp ref the tmp ref that you want to get time and field values from,
 * time is the PTS of this tmp ref, last_field is the field type of the last field, first_field
 * is the field type of the fist field and nr fields is the number of fields present in the tmp
 * ref (frame), frame array is either NULL or a frame array that you can use to speed up the prosses
 * of this func.
 */


int get_time_field_info(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, int tmp_ref,
		ticks_t *  time, field_type_t * last_field, field_type_t * first_field,
		int * nr_of_fields, tmp_ref_frame_t * frame_array);


/* Internal function
static int get_time_field_info_std_alone(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder,
		int tmp_ref, ticks_t *  time, field_type_t * last_field,
		field_type_t * first_field, int * nr_of_fields); */



/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* From tyrecord.c
 * Functions that is operation on a tyrecord */

/**
 * find_start_code_offset finds the start offset in the tyRecord to the start of a specific
 * MPEG/AC3 start_code. NOTE: DON'T USE THIS FUNCTION ON 0x2e0, 0x4c0, and 0x2c0 type records
 * it will not work and is not coded for it!! NOTE: AC3_AUDIO and MPEG_AUDIO is not yet supported!!
 *
 * @return Up on sucess the offset to the start of the start_code, Up on faliour -1 is returned
 * @params i == tyRecord NOTE counting starts at 0
 * chunks is the chunk that the tyRecord is belonging to, start_code_type is the start code you are after,
 */

int find_start_code_offset(int i, const chunk_t * chunks, const start_codes_t start_code_type,
			   int size, tystream_holder_t * tystream);


/**
 * get_video will give you a MPEG video record (module type) from a specific tyRecord (i) (count starts at 0)
 * in a specific chunk (chunks). NOTE: tyRecord and module must match - it will error out otherwise. Use modle
 * as you would with e.g. fstat it will hold a allocated databuffer and the size of that buffer (see the structure
 * of module_t. NOTE: The function will give you a whole module (e.g. B-Frame) even if it's split between to chunks.
 *
 * @return Up on success 1 is returned, Up on failur -1 one is returned
 * @params i == tyRecord, module is the parameter that will hold your data, module_type is the type of MPEG module
 * you want (e.g. a B-Frame).chunks is the chunk that the tyRecord belongs to.
 */

int get_video(int i, module_t * module, const chunk_t * chunks, start_codes_t  module_type, 
		tystream_holder_t * tystream);


/**
 * get_audio will give you a MPEG/AC3 audio record + pes header from a specific tyRecord (i) (count starts at 0)
 * in a specific chunk (chunks). NOTE: THE ONLY module_type alowed is MPEG_PES_AUDIO. NOTE: tyRecord type and module
 * type must match - it will error out otherwise. Use audio_module as you would with e.g. fstat it will hold a
 * allocated databuffer and pes_header data buffer and the size of each buffer (see the structure
 * of audio_module_t. NOTE: The function will give you a whole module even if it's split between to chunks.
 * NOTE it repairs the proparitary DTIVO MPEG pes header hence the pes_data_buffer holds a real pes header.
 *
 * @return Up on success 1 is returned, Up on failur -1 one is returned
 * @params i == tyRecord, audio_module is the parameter that will hold your data, module_type is the type of
 * MPEG module you want (IN THIS CASE ONLY AUDIO PES), chunks is the chunk that the tyRecord belongs to.
 */

int get_audio(int i, audio_module_t * audio_module, const chunk_t * chunks,
	 start_codes_t  module_type, tystream_holder_t * tystream);


/**
 * get_next_record will append the next record of media type AUDIO or VIDEO to the data_buffer, it will control
 * that the min size is meet. This means that you can say I want at least 400 bytes of the next VIDEO/AUDIO records.
 * If min_size is set to zero it will default to 40 (this is used when seeking start codes in a record) Media type
 * can either be AUDIO or VIDEO
 *
 * @return Up on sucess a reallocated data_buffer will be returned, up on failur NULL will be returned.
 * @params i == present tyRecord, chunk that the present tyRecord is present in, media is either VIDEO or AUDIO,
 * size is the initial size of the data_buffer you are feeding the funcion with, min_size is the minmum size you want.
 */

uint8_t * get_next_record(int i, const chunk_t * chunk, media_type_t media,
	int size, uint8_t * data_buffer, int min_size);


/* Internal functions
static int private_find_start_code_offset(int i, int first, const chunk_t * chunks,
				const start_codes_t start_code_type,
			   	int size, uint8_t **pdata_buffer, tystream_holder_t * tystream);

static int video_size_to_get(int i, last_record_info_t * info, const chunk_t * chunks,
				 tystream_holder_t * tystream);


static int audio_size_to_get(int i, last_record_info_t * info, const chunk_t * chunks,
				start_codes_t  module_type, tystream_holder_t * tystream);

static int get_pes_header_size(const uint8_t * pes_header);
*/



/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


/* From buffer.c
 * Functions that operatates on different buffers present in a tystream_holder_t
 * (most of the time it's operations on the payload_data buffer */

/**
 * find_start_code_offset in buffer finds the start offset  to the start of a specific
 * MPEG/AC3 start_code in the buffer. NOTE: AC3_AUDIO and MPEG_AUDIO is not yet supported!!
 *
 * @return Up on sucess the offset to the start of the start_code, Up on faliour -1 is returned
 * @params  start_code_type is the start code you are after, data_buffer is the buffer you are
 * searching in.
 */

int find_start_code_offset_in_buffer(const start_codes_t start_code_type, int size, uint8_t * data_buffer,
		 tystream_holder_t * tystream);


/**
 * find_extension in payload finds the offset  of a specific MPEG VIDEO extension in
 * the payload see13818-2 section 6.3.1 table 6-2 for the uint8 extension code. NOTE
 * don't use this one on a audio payload!! There is no checking for that!!
 *
 * @return Up on sucess the offset to the start of the extension, Up on faliour -1 is returned
 * @params  Tystream is the tystream holder you operate on, payload is the payload you want to find
 * the extension in and extension is the extension code see above.
 */

int find_extension_in_payload(tystream_holder_t * tystream, payload_t * payload, uint8_t extension);


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


/* From chunk.c
 * Main chunk functions to create and destroy chunks*/

/**
 * add_chunk will append a chunk to the chunks liked list. The function controls if the chunk is a duplicated
 * chunk and if so it will free the chunk and return NULL. The function will free the chunk if it detects a duplicated
 * chunk - HENCE DON'T FREE ON FAIL
 *
 * @return Up on sucess the chunks linked list will be returned Up on failur (duplicated chunk) NULL will be retured.
 * @params tystream the tystream holder, chunk the chunk to add, chunks the chunk linked list
 */

chunk_t * add_chunk(tystream_holder_t * tystream, chunk_t * chunk, chunk_t * chunks);

/**
 * free_chunk will free the chunk and it's underlaying structure - NOTE it only frees the chunk not the next or prev chunk
 *
 * @params chunk the chunk to free
 */

void free_chunk(chunk_t * chunk);

/**
 * free_junk_chunks will free the chunks linked list recursivly
 *
 * @params chunks the (junk) chunk buffer you want to free
 */

void free_junk_chunks(chunk_t * chunks);

/******************************************************************************/

/* From chunk_read.c
 * Function(s) used to read a chunk */

/**
 * get_chunk a combined function that will read the chunk with chunk number "chunk_nr"
 * check it and add it to the chunks linked list in the tystream holder
 *
 * @params tystream the tystream holder you which to add, get and insert the chunk from
 * @returns Returns 1 on sucess 0 when it fails to add or check the chunk - NOTE THIS DOESN'T
 * MEAN THAT IT TOTALLY faild see junk_chunk_buffer!!!
 */

int get_chunk(tystream_holder_t * tystream, chunknumber_t chunk_nr, int * progress_meter);

int adj_byte_offset(tystream_holder_t * tystream);


/**
 * read_chunk will read a typacket and return a fully parsed allocated chunk of type chunk_t. The file
 * offset for file_nr must point to the start of the chunk to make this function work properly
 *
 * @returns Up on sucess a fully parsed and allocated chunk_t will be returned, Up on failur NULL is retured
 * @params file_nr  the file desctriptor returned by the system call open,
 * chunk_nr the chunk number of this chunk, max_seek if used in a probe func you can set this value
 * to e.g. 2048 in order to seek the start of the chunk upto 2048 bytes from it's real pos - SET THIS
 * VALUE TO 1 DURING ALL OTHER OPERATIONS!!!
 */

chunk_t * read_chunk(tystream_holder_t * tystream, chunknumber_t chunk_nr, int max_seek);


record_header_t * read_record(chunk_t * chunk, int in_file, vstream_t * vstream);



/******************************************************************************/
/* From chunk_help_functions

Misc functions that is operating on chunks */

/**
 * get_nr_audio/video_pes gives you the number of video or audio
 * pes_header present in the chunk.
 *
 * @return Returns number of audio/video pes header records present in
 * the chunk.  FIXME - Returns -1 on error -
 * @params chunk specifies what chunk you want to act on
 *
 */

int get_nr_audio_pes(tystream_holder_t * tystream, const chunk_t * chunks);

int get_nr_video_pes(tystream_holder_t * tystream, const chunk_t * chunks);

/**
 * get_nr_audio_records
 * A V_13 only func that will return the total number of audio records in a
 * a chunk - note this is not the same as nr of audio pes!
 *
 * @return Returns number of audio records present in
 * the chunk.  FIXME - Returns -1 on error -
 * @params chunks specifies what chunk you want to act on
 */

int get_nr_audio_records(tystream_holder_t * tystream, const chunk_t * chunks);


/******************************************************************************/
/* From chunk_verify.c

Functions that verifies if two consecetive chunks are in the right order and that they
are from the same stream  FIXME this function is currently not used or tesed 
and it's probably not needed since we have such a good repair func */


/**
 * check_audio_sync controls that the audio is in sync from one chunk to the other
 * It's slow compared to check_chunk so only use it when unsure.
 *
 * @return Returns 0 when the chunks are in sync and 1 when they are out of sync,
 * returns -1 on failur
 * @params present_chunk specifies what chunk you have and check_chunk specifies the
 * next chunk that you want to test if it's in sync with present chunk, seek_backwards,
 * seek_forward specifies if you want to check neigbouring chunks if any of the chunks
 * doesn't have any audio records. Tystream is the tystream that will hold your chunks
 * later on.
 *
 */

/**
 * check_audio_sync controls the audio sync between two chunks. You will
 * use this function to check if there is a hole in the tyStream.
 *
 * @returns -1 == error, 0 == in sync and  1 == out of sync
 * @parmas present_chunk == the chunk you have, check_chunk is a future chunk you
 * want to test, audio_median_tick_diff is the time difference between audio frames,
 * seek_backwards if set (1) it will enable check_audio_sync will use a previous chunk
 * if present chunk doesn't have any audio chunks, seek_forward if set (1) it will enable
 * check_audio_sync will use a "next" chunk if check chunk doesn't have any audio chunks,
 * tystream is the tystream_holder.
 */

int check_audio_sync(const chunk_t * present_chunk, const chunk_t * check_chunk,
	int seek_backwards, int seek_forward, tystream_holder_t * tystream);

/******************************************************************************/

/* From chunk_check.c
 * Functions used to check a chunk FIXME lot's of code dup */

/**
 * check_chunk will check the integrety of the chunk compared to
 * the rest of the chunks previously checked.
 *
 * It will preform the following checks in desending order.
 * If the audio in the chunk is off correct type
 * If there is invalid record types in the chunk
 * If the video is out of sync with the previus chunk
 *
 * If any of the above fails it will free the chunk and return NULL,
 * other wise it will return the checked chunk.
 *
 * However since tystreams can be quite flaky there is some counter messures
 * in this function.
 *
 * If we find that we have droped JUNK_CHUNK_BUFFER_SIZE
 * number of chunks in a row we will go into a "repair" mode. Most probably
 * we either hit a reset of the PTS timestamps or we had a gigantic hole/gap
 * in the video stream. We have therefore saved all our droped chunks and will
 * now reexamine them - if we find that they are in sync they will be returned
 * by check chunk function so you can add them into the chunks linked list with
 * help of add chunk.
 *
 * Also if we find suspicion that we have gap in the stream the gap flag in the
 * chunk will be raised and we can later on check it better with the more time
 * consuming check_audio_sync function.
 *
 c Will return the chunk of succes will free the chunk and return NULL on
 * failur
 * @params tystream the tystream_holder, chunk the chunk to be checked.
 */

chunk_t * check_chunk(tystream_holder_t * tystream, chunk_t * chunk);

/* Internal
static void bsort(ticks_t video_time_array[], int size);
static chunk_t * check_audio_sync_V_13(tystream_holder_t * tystream, chunk_t * chunk);
*/


/******************************************************************************/

/* From chunk_check_junk.c */

/**
 * Internal function used by check_chunk DON'T USE IT.
 *
 * During check_chunk we will store the sync data in all chunks including
 * droped chunks - this function preforms a similar check that we did in
 * check chunk in order to test the integrety of the junk_chunk buffer. It will
 * do limited repairs and flag the gap flag in suspicios chunks but if we
 * find the buffer to be in really bad state we will abort and EXIT
 * FIXME will need to do a more graceful exit.
 */

chunk_t * check_junk_chunks(tystream_holder_t * tystream);

chunk_t * check_junk_chunks_V_13(tystream_holder_t * tystream);


/* Internal functions
static int check_junk(chunk_t * chunk);
static int check_junk_V_13(tystream_holder_t * tystream, chunk_t * chunk);
*/



/******************************************************************************/

/* From chunk_parse.c */

/**
 * parse_chunk will parse your chunk :) - and add the video and audio it
 * extracts to the tystream->pes_holder_video and tystream->pes_holder_audio
 * @returns returns the next chunk to parse or null if this was the last chunk
 * to parse.
 * @params tystream the tystream_holder, chunks the chunk to be parsed
 */

chunk_t * parse_chunk(tystream_holder_t * tystream, chunk_t * chunks);

/**
 * chunk_okay_to_parse will check if it's safe to parse a chunk - we need
 * at least three chunks to parse a chunk due to overlapping tyrecords
 *
 * @params chunks the linked list of chunks that you want to test
 * @returns Returns 1 if it's safe to parse, returns 0 if unsafe to parse 
 */
int chunk_okay_to_parse(const chunk_t * chunks);


/* Internal
- Video
- Used by parse_chunk_video_remainder_S1 to separate P-Frames from B-Frames in SA/UK tivos
static pes_holder_t * fix_pes_holder(tystream_holder_t * tystream, pes_holder_t * pes_holder);
- Main parsing functions for video
static int parse_chunk_video_remainder_S1(int i, tystream_holder_t * tystream, chunk_t * chunks, pes_holder_t * pes_holder);
static int parse_chunk_video_S1(tystream_holder_t * tystream, chunk_t * chunks);
static int parse_chunk_video_remainder_S2(int i, tystream_holder_t * tystream, chunk_t * chunks, pes_holder_t * pes_holder);
static int parse_chunk_video_S2(tystream_holder_t * tystream, chunk_t * chunks);
static int parse_chunk_video(tystream_holder_t * tystream, chunk_t * chunks);
- Audio
- Main parsing functions for audio
static int parse_chunk_audio_V_2X(tystream_holder_t * tystream, chunk_t * chunks);
static int parse_chunk_audio_remainder_V_13(int i, tystream_holder_t * tystream, chunk_t * chunks, pes_holder_t * pes_holder);

*/


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


/* From debug.c
 * Functions that only have debuging purpose */

/**
 * print_chunk prints the chunk for debugging purpose. Both the record header and record data/extended data is printed
 * NOTE: A maximum of 16 bytes of the record data is printed.
 *
 * @params chunk the chunk you want to print
 */

/**
 * print_chunk prints the chunk for debugging purpose. Both the record header and
 * record data/extended data is printed
 * NOTE:if record data is present then we print the first and last 64 bytes of data present in the
 * the record.
 *
 * @params chunk the chunk you want to print
 */

void print_chunk(const chunk_t * chunk);

/**
 * print_seq prints the seq/gop for debugging purpose - data about each frame present in the gop will
 * be printed - this includes seq nr, chunk nr, tmp ref and frame (payload) type
 *
 * @params pes_holder the pes_holder of the seq/gop you want to print
 */

void print_seq(const pes_holder_t * pes_holder);

/**
 * print_video_pes prints data about all video_pes_holders present in the tystream holder - it's not a very
 * good func FIXME :)
 *
 * @params  none besides the tystream
 */

void print_video_pes(tystream_holder_t * tystream);


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* From probe.c
 * Functions used to probe a tystream FIXME we will definitly need to work
 on thos func is a big mess at the moment!!*/


/**
 * probe_tivo_version will return the tivo software version number used to record the tystream with.
 * the file offset for file_nr must point to the start of the chunk to make this function work properly
 * hence you will need to seek to the start of the first "valid chunk" - this is especially important if
 * your extraction tool is extracting the master chunk of a tystream.
 *
 * @returns 0 == 1.3 or earlier 1 == 2.x and later
 * @params file_nr  the file desctriptor returned by the system call open
 */

// DON't USE THIS ONE since some streams can be really fucked up and doesn't
// and makes us set the tivo version to the wrong one
//int probe_tivo_version(tystream_holder_t * tystream);

/**
 * std_probe_tystream will do the nessesasy probing of the tysteam
 * to determine - tivo type (DTIVO/SA), audio type (AC3/MPEG), average
 * difference in ticks between to audio frames, average size of audio
 * record, and audio frame size. In the case of average audio record
 * size a audio frame size it's more of a check that this is a known
 * Tivo audio type.
 *
 * This function is MANDOTORY to do on a stream otherwise the demux will
 * fail!. Sample X chunks (recomend 20 or more) from a different parts of
 * the tystream with help of read_chunk and the chunk to the chunks linked
 * list in the tysteam with help of add_chunk. DON'T USE CHECK_CHUNK on any
 * of the chunks you add!!.
 *
 * !NOTE The sampled chunks are freed by this function and can't be used again
 */

int std_probe_tystream(tystream_holder_t * tystream);
int tivo_probe_tystream(tystream_holder_t * tystream);

/**
 * Function to advance to the start of the right audio in a stream
 */

void get_start_chunk(tystream_holder_t * tystream);



/*
static int probe_tystream_S2(tystream_holder_t * tystream);
static int probe_tystream_audio(tystream_holder_t * tystream);
static int probe_audio_tick_size(tystream_holder_t * tystream);
static uint8_t actual_frame_rate(ticks_t diff);
static int probe_tystream_frame_rate(tystream_holder_t * tystream);
static ticks_t get_time_pre_b_frame(tystream_holder_t * tystream, chunk_t * chunk, int i);
static ticks_t get_time_post_b_frame(tystream_holder_t * tystream, chunk_t * chunk, int i);
static chunk_t * copy_chunk(chunk_t * chunk);
static void clear_tystream_audio_probe(tystream_holder_t * tystream, int ready);
*/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


/* From misc.c *
 * All kind of misc functions mostly handling user feedback */

/**
 * set_debug_level sets the verbosity of the tydemux/remux library.
 *
 * @params debug_level the debug level you want.
 */

void set_debug_level(int debug_level);

/**
 * prog_meter prints the command line progress meter needs to be called every
 * time we read a chunk
 *
 * @params none besides the tystream
 */

void prog_meter(tystream_holder_t * tystream);
void old_prog_meter(tystream_holder_t * tystream);

#if !defined(TIVOSERVER)
/**
 * usage prints usage and exits NOTE exits not returns
 *
 * @params none
 */

int tydemux_usage();
#endif

/**
 * print_probe prints the results of our probe to the user in a easy
 * to understand format.
 *
 * @params none besides the tystream
 */

void print_probe(tystream_holder_t * tystream);

/* MOVE THIS ONE FIXME */

/**
 * nr_chunks_f returns the number of chunks we have in a chunk_t linked
 * list - mainly used to control that we are handling out chunk counting
 * correctly - FIXME we are handling that correctly so we need to fix that!!
 *
 * @returns returns the number of chunks in the chunk_t linked list
 * @params chunks the chunk linked list you want to count
 */

int nr_chunks_f(const chunk_t * chunks);

/**
 * print_audio_delay prints the A/V sync offset that you will need when muxing
 * the stream with e.g. mplex. You will call this function just before you exit
 * tydemux after a successful demuxing
 *
 * @params none besides the tystream
 */

int print_audio_delay(tystream_holder_t * tystream);

/**
 * Unix/MacOSX/Linux can only return a uint8_t while Windows can return a int
 * hence we can't return A/V offsets values for Unix this little func deal with 
 * this
 */

int  tydemux_return(int return_value);

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* From pes_holder_video_help.c */

/* is_okay checks if we can control the the gop/seq present in pes_holder
 * to_check_pes is a gop futher a head in the pes_holder linked list. This
 * function is used when checking sync, tmp_ref, etc, and is needed since
 * we can't check and mend e.g. drift before we checked the temporal reference
 *
 * @returns returns zero on fail and one or more on success.
 * @params pes_holder the pes holder you want to check (i.e. the one you want to check or fix
 *  to_check_pes is a pes_holder a head of the pes you want to check against.
 *
 */


int is_okay(const pes_holder_t * pes_holder, pes_holder_t * to_check_pes);
int is_okay_frames(const pes_holder_t * pes_holder, pes_holder_t * to_check_pes);

/******************************************************************************/


/* From pes_holder.c */

/**
 * get_pes_holders will fetch the pes_holders present in the tystream->chunks 
 * until there is only three chunks left in the linked list 
 * uses parse_chunk - parsed pes holders will be added to the video and audio
 * pes linked list 
 * 
 * @params none besides the tystream you operate on
 * @returns 1 on sucess and 0 on failure or no chunks to operate on
 */

int get_pes_holders(tystream_holder_t * tystream);

/**
 * pes_holders_nr_of_okay checks that the number of pes holders you have in
 * the pes_holder is above the treshhold you want to check
 *
 * @params pes_holder the linked pes_holder list you want to check, treshhold
 * the number of pes_holder you have set as a mimimum for your check
 * @returns 1 if you have treshhold or more pes holders in the linked list, 0 
 * if you have less
 */

int pes_holders_nr_of_okay(const pes_holder_t * pes_holder, int treshhold);


/**
 * new_payload - creates a new empty payload to use in a pes holder
 *
 * @returns - a new empty but allocated payload_t
 * @parames - none
 */

payload_t * new_payload();

/**
 * free_payload - frees a sigle payload
 *
 * @params payload the payload you want to free
 */

void free_payload(payload_t * payload);

/**
 * free_all_payload - frees all payload present in the payload linked list
 *
 * @params payload the payload you want to free
 */

void free_all_payload(payload_t * payload);

/**
 * free_pes_holder - frees a pes holder and it's payload if free_payload is set to one, if
 * set to 0 it will not free the payload
 *
 * @params pes_holder the pes holder you want to free, free_payload if you want to free the pay
 * load of the pes holder - 0 == don't free 1 == free.
 */

void free_pes_holder(pes_holder_t * pes_holder, int free_payload);

/**
 * free_all_pes_holders - frees all pes_holders present in the pes_holder linked list
 * including thier payload
 *
 * @params pes_holder the pes holder linked list you want to free
 */

void free_all_pes_holders(pes_holder_t * pes_holder);

/**
 * new_pes_holder - creates a new empty pes_holder
 *
 * @returns - a new empty but allocated pes_holder
 * @parames - the media type that you will use your pes holder for i.e. audio or video
 */

pes_holder_t * new_pes_holder(media_type_t media);

/**
 * write_pes_holders - will write as many pes holders (both video and audio) as it can and then return
 *
 * @returns - 1 on success 0 on fail or not allowed to write
 * @parames - none besides the tystream you operate on
 */

int write_pes_holders(tystream_holder_t * tystream, int * video_progress_meter, int * audio_progress_meter);

/**
 * write_pes_holder - will write a pes holder to either the video or audio ES depending on
 * the media type of the pes holder
 *
 * @returns - the next pes_holder in the linked list of pes holders - frees the current
 * previous pes holder that you wrote FIXME
 * @parames - pes_holder the pes holder you want to write to
 */

pes_holder_t * write_pes_holder(tystream_holder_t * tystream, pes_holder_t * pes_holder, int * video_progress_meter, int * audio_progress_meter);

/**
 * add_pes_holder adds the pes holder to the apropriate pes_holder present in the tystream
 * holder - depends on it's media type
 *
 * @params pes_holder the pes holder you want to add
 * @returns 1 on success and 0 on failur
 */
int add_pes_holder(tystream_holder_t * tystream, pes_holder_t * pes_holder);


/**
 * add_payload adds a payload to the pes holder
 *
 * @params pes_holder the pes holder you are adding the payload to, payload the payload you are
 * adding.
 */

void add_payload(pes_holder_t * pes_holder, payload_t * payload);

/**
 * duplicate_pes_holder does what it says however NOTE that it's a limited duplication
 * only working for video pes holders!!! FIXME and that it doesn't duplicate everything!!
 *
 * @params pes_holder the pes holder you want to duplicate
 * @returns returns a new and allocated duplication of the pes holder including it's payload
 */

pes_holder_t * duplicate_pes_holder(const pes_holder_t * pes_holder);


/**
 * duplicate_payload does what it says however NOTE that it's a limited duplication
 * only working for video payload!!! FIXME and that it doesn't duplicate everything!!
 *
 * @params payload the pes holder you want to duplicate
 * @returns returns a new and allocated duplication of the payload
 */

payload_t * duplicate_payload(const payload_t * payload);


/**
 * reinit_pes_holder, resets all values in a pes and counts payload once over again - this one
 * should only be used when doing a split of B/P frames in SA/UK S1 tivo streams - FIXME to work
 * with all pes
 *
 * @params pes_holder the pes holder you want to reinit
 */

void reinit_pes_holder(pes_holder_t * pes_holder);

/**
 * FIXME Better name on this func!
 * nr_throw_away_pes_holder gives you the number of pes holders present in a pes_holder linked
 * list starting with pes_holder.
 *
 * @params pes_holder the pes holder you want to start counting from
 * @returns number of pes holders present
 */


int nr_throw_away_pes_holder(const pes_holder_t * pes_holder);

payload_t * payload_fetch_seq_gop(const pes_holder_t * pes_holder);
void pes_holder_attache_drift_i_frame(const pes_holder_t * pes_holder, int64_t time, int64_t total_drift);


/* Internal 
static pes_holder_t * write_pes_holder_real_audio(tystream_holder_t * tystream, pes_holder_t * pes_holder);
static pes_holder_t * write_pes_holder_real_video(tystream_holder_t * tystream, pes_holder_t * pes_holder);
static int add_pes_holder_audio(tystream_holder_t * tystream, pes_holder_t * pes_holder);
static int add_pes_holder_video(tystream_holder_t * tystream, pes_holder_t * pes_holder);
static payload_t * fetch_next_correct_audio_payload(tystream_holder_t * tystream, const pes_holder_t * pes_holder);
*/


/******************************************************************************/


/* From pes_holder_video_check.c */


/**
 * next/previous_seq_holder will return the next/previous pes_holder that has a SEQ/GOP
 * header in it's payload
 *
 * @returns returns 0 in failure, if successful it returns the preivous or next seq pes_holder present
 * int pes_holder linked list
 * @params seq_pes_holder the present seq pes_holder that you want to find a the seq
 * pes_holder before or after
 *
 */


pes_holder_t * next_seq_holder(const pes_holder_t * seq_pes_holder);
pes_holder_t * previous_seq_holder(const pes_holder_t * seq_pes_holder);

/**
 * check_fix_pes_holder_video is the major function that is checking the integrety of the mpeg PES
 * video stream. It's correcting the B/I frame seq problem, fixing the time field in the p-frame on
 * S1 SA/UK Tivos, checkin the temporal reference and corrects it if there is a error including
 * missing frames, it check (but doesn't repair FIXME) the field alignment of frames, finally it's
 * checking the drift of A/V sync and mend it if it's out of bounds.
 *
 * You MUST feed this function with a pes_holder that has a SEQ and a GOP as payload, it will fail other
 * wise and there is no detection of this FIXME.
 *
 * @returns is always returning 1 FIXME
 * @params none besides the tystream
 */

int check_fix_pes_holder_video(tystream_holder_t * tystream);


/* Internal
static pes_holder_t * check_fix_p_frame(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
static pes_holder_t * check_fix_tmp_ref(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
static pes_holder_t * check_fix_fields(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
static pes_holder_t * check_fix_av_drift(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
*/

/******************************************************************************/


/* From pes_holder_insert */
/**
 * insert_pes_holder will insert a pes holder closely after the seq_pes_holder,
 * the tmp_ref of the pes_holder is not taken in to consideration HENCE YOU NEED
 * TO CALL FIX TMP REF after you are done inserting pes holders otherwise the decoder
 * order of the frames will be totally random.
 *
 * @params seq_pes_holder the pes_holder holding the SEQ/GOP that you want to insert the
 * pes_holder into, pes_holder the pes holder you are inserting, tmp_ref the temporal reference
 * of the payload present in the pes_holder you are inserting this FIXME me not used
 *
 * @returns always returning 1 FIXME
 */

int insert_pes_holder(const pes_holder_t * seq_pes_holder, pes_holder_t * pes_holder_insert, uint16_t tmp_ref);

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


/* From bit.c */

/**
 * set_bit sets bit X from a unit8_t array counted from left to right so to say
 * starting at bit 0 - you can e.g. set bit 57 - note no check what so ever if
 * you set a bit outside the array!!
 *
 * @params set the array you are modifying, the number of the bit you want to set
 * value either 1 or 0
 */

void setbit(uint8_t *set, int number, int value);

/**
 * get_bit gets the value of bit X from a unit8_t array counted from left to right so to say
 * starting at bit 0 - you can e.g. get bit 57 - note no check what so ever if
 * you get a bit outside the array!!
 *
 * @params set the array you are want to get the value from
 * @returns returns the value of the bit you ask for
 */


int getbit(uint8_t *set, int number);


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


/* From tystream.c */

/**
 * new_tystream creates a new fully allocated and initialzed tystream_holder_t
 * structure.
 *
 * @returns Returns a new fully allocated and initialzed tystream_holder_t
 */

tystream_holder_t * new_tystream(operation_mode_t mode);


/**
 * free_tystream frees the tystream_holder_t plus all underlaying structures.
 * in the tystream_holder_t.
 *
 * @params tystream the tystream_holder_t you want to free
 */

void free_tystream(tystream_holder_t * tystream);


/**
 * tystream_set_*_file sets in and out files needed by the demux/remuxing process
 *
 * @params - file handles for your files, and the file size of the tystream that you
 * are demuxing
 *
 * @returns Always returning 1 on success 0 on failure
 */

int tystream_set_out_files(tystream_holder_t * tystream, int audio_file, int video_file);

int tystream_set_in_file(tystream_holder_t * tystream, int in_file, off64_t in_file_size);

int tystream_set_out_file(tystream_holder_t * tystream, int out_file);

/******************************************************************************/


/* From tystream_repair.c */

/**
  * FIXME THIS IS THE TEXT FOR repair_tystream_real
 * repair_tystream will repair a tystream after a hughe gap is detected - this is nessesary
 * otherwise we will loose sync.
 * FIXME this function needs even better calulations in order to minimize the drift NOTE You need a
 * high number of video/audio pes for this function to work. Hence as soon as you detect that the
 * tystream is in repair mode you should stop writing and stop checking video pes holders and continue
 * to read and parse chunks until you have a fairly high number of pes holders.
 *
 * @params none besided the tystream
 * @returns 0 on failure - exit if this happens 1 on success!!
 */

int repair_tystream(tystream_holder_t * tystream);

/* Internal
int repair_tystream_real(tystream_holder_t * tystream);
static int nr_pes_holders_after_gap(const pes_holder_t * pes_holder);
static pes_holder_t * find_seq_holder(pes_holder_t * seq_pes_holder, data_times_t * audio_times);
static pes_holder_t * find_pi_frame_holder(pes_holder_t * pi_frame_pes_holder, data_times_t * audio_time,
		data_times_t * video_times);
static int check_gaps(const pes_holder_t * seq_pes_holder);
static int get_audio_times(const tystream_holder_t * tystream, data_times_t * audio_times);
static int fix_audio_pes_holder_list(tystream_holder_t * tystream, pes_holder_t * stop_pes_holder,
		 pes_holder_t * start_pes_holder);
static int repair_audio(tystream_holder_t * tystream, data_times_t * audio_times, data_times_t * video_times);
*/

/******************************************************************************/


/* from tystream_init.c */
/* FIXME Code dup from tystream_repair.c
   FIXME may loop into for ever !!
*/


/**FIXME TXT from static int tystream_real_init(tystream_holder_t * tystream);
 * tystream_init will init your tystream - it finds the first useable seq header and aling
 * the audio with it. Note you need at least 200 pes holders for this one to work!!
 *
 * @params none besides the tystream
 * @returns returns 1 on sucess and 0 on failure
 */

int tystream_init(tystream_holder_t * tystream);


/* Internal
static int tystream_real_init(tystream_holder_t * tystream);
static pes_holder_t *  init_find_seq_holder(pes_holder_t * seq_pes_holder);
static pes_holder_t * init_find_audio(tystream_holder_t * tystream, pes_holder_t * audio_pes_holder, data_times_t * video_times);
*/

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


/* From gop.c */
/**
 * gop_make_closed is used by tystream_init and tystream_repair it makes opengop closed by removing any frame that
 * is dependent on the previous gop - it sets the gop header and fixes the tmp ref in the gop.
 * NOTE: S2 SA has a closed gop as the first gop but it's not marked as closed!!
 * FIXME this func is not good see comments
 *
 * @params pes_holder - the seq/gop that you want to make closed, video_times is a struct that will hold the
 * start time of the gop
 * @returns the NEW seq_pes_holder that has been closed
 */

pes_holder_t * gop_make_closed(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder, data_times_t * video_times);

/**
 * is_closed_gop - checks if a gop is closed or open by reading the gop header
 *
 * @params pes_holder the pes_holder that has the GOP you want to check as payload
 * @returns 1 if the gop is closed 0 if the gop is open NOTE: Tivo S2 SA has closed gop's marked as open!!
 */
int is_closed_gop(const pes_holder_t * pes_holder);

/**
 * move_seq_to_i_frame S1 tivo has the SEQ and GOP header in the PES holder
 * of the last B-Frame of the previous
 * SEQ/GOP (idiotic) this func is moving the SEQ/GOP headet to the pes
 * holder of the first I frame in the SEQ/GOP.
 *
 * @parmas seq_pes_holder the current pes holder that holds the SEQ (i.e. the B-FRAME).
 * @returns returns the new seq pes holder or null on failure
 */

pes_holder_t * move_seq_to_i_frame(pes_holder_t *seq_pes_holder);

/**
 * check_fix_progressive_seq - sets the progressive_sequence flag in the sequence_extension header
 * this one is used in functions that turns a 29.97 f/s 3:2 downpulled movie mpeg into 23.976 f/s progressive
 * mpeg movie - FIXME NOT USED
 *
 * @params payload the payload holding the seq header
 * @returns 1 on success and 0 on failure
 */

int check_fix_progressive_seq(tystream_holder_t * tystream, payload_t * payload);


int set_smpte_field(const tystream_holder_t * tystream, const pes_holder_t * pes_holder);
int count_frame_fix_gop(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/


/* From picture.c */

/**
 * get_picture_info populates the picture_info struct with the info about repeate_first_field,
 * top_field_first and  progressive frame - see 6.3.10 in ISO-13818-2
 *
 * @params payload the payload holding the frame that you want the picture info from.
 * picture info the struct that will hold the info
 * @returns 1 on success and 0 on failure
 */
int get_picture_info(tystream_holder_t * tystream, payload_t * payload, picture_info_t * picture_info);

/**
 * check_fix_progessive_frame - forces a frame to be a progressive picture that is top_field first
 * and not repeatedFIXME lot more work to do
 * here - if we want this to func the right way.
 *
 * @params the payload who's frame you want to alter
 * @returns 1 on success and 0 on failure
 */
int check_fix_progressive_frame(tystream_holder_t * tystream, payload_t * payload);

/**
 * set_repeate_first_field sets/rests the repeate first field bit and also if nessesary the progressive frame
 * bit see 6.3.10 in ISO-13818-2 FIXME need to do a better job when we want to turn a non progessive
 * frame into a progressive one!!
 *
 * @params the payload who's frame you want to affect, value 0 or 1 if you want to reset or set the field
 * @returns 1 on success and 0 on failure
 */
int set_repeat_first_field(tystream_holder_t * tystream, payload_t * payload, int value);


/**
 * set_top_field_first sets/rests the top field first bit see 6.3.10 in ISO-13818-2
 *
 * @params the payload who's frame you want to affect, value 0 or 1 if you want to reset or set the field
 * @returns 1 on success and 0 on failure
 */
int set_top_field_first(tystream_holder_t * tystream, payload_t * payload, int value);

/**
 * get_repeate_first_field fetches the repeate first field bit see 6.3.10 in ISO-13818-2
 *
 * @params the payload frame you want to examine
 * @returns 1 if the top field is first 0 if bottom field is first
 */

int get_repeat_first_field(tystream_holder_t * tystream, payload_t * payload);

/**
 * check_fields_in_seq controls if the fields in a seq mathces up and the we don't have a combination
 * like this TBT TBT or TB BT  - it also checks the  progressive frame/repeate_first_field deps
 * 
 * NOTE:!!! YOU MUST DO A TMP_REF CHECK/REPAIR BEFORE USING THIS FUNC IT WILL NOT FUNCTION PROPERLY
 * OTHERWISE!!!
 *
 * @params seq_pes_holder the seq you want to check
 * @returns check_fields_in_seq returns the number of errors found 0 and up or -1 on failure
 */
int check_fields_in_seq(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);

/**
 * get_picture_info_tmp_ref populates the picture_info struct with the info about repeate_first_field,
 * top_field_first and  progressive frame - see 6.3.10 in ISO-13818-2 for a specific tmp_ref in the
 * seq/gop
 *
 * @params seq_pes_holder the seq holding the tmp ref you want the pic info from, tmp_ref the tmp ref
 * you want the info from, picture info the struct that will hold the info.
 * @returns 1 on success and 0 on failure
 */

int get_picture_info_tmp_ref(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder,
	int tmp_ref, picture_info_t * picture_info );

/**
 * set_repeate_first_field sets/rests the repeate first field bit and also if nessesary the progressive frame
 * bit see 6.3.10 in ISO-13818-2 of a specific tmp ref in the seq/gop
 * FIXME need to do a better job when we want to turn a non progessive
 * frame into a progressive one!!
 *
 * @params seq_pes_holder the seq holding the tmp ref you want to affect, the tmp_ref who's frame you want
 * to affect, value 0 or 1 if you want to reset or set the field
 * @returns 1 on success and 0 on failure
 */

int set_repeat_first_field_of_tmp_ref(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder,
	uint16_t tmp_ref, int value);
/**
 * set_top_field_first sets/rests the top field first bit see 6.3.10 in ISO-13818-2
 * of a specific tmp ref in the seq/gop
 *
 * @params seq_pes_holder the seq holding the tmp ref you want to affect, the tmp_ref who's frame you want
 * to affect, value 0 or 1 if you want to reset or set the field
 * @returns UNKNOWN_FIELD on error, on succes it returns the field type of the last field in the frame
 */

field_type_t set_top_field_first_of_tmp_ref(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder,
	 uint16_t tmp_ref, field_type_t  last_field);


/*NOTE!!
 * Please see the
	int get_time_field_info(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, int tmp_ref,
		ticks_t *  time, field_type_t * last_field, field_type_t * first_field,
		int * nr_of_fields, tmp_ref_frame_t * frame_array);
 * Function if you want more than just picture info - i.e. time
 */


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* Functions that operates on the tmp ref of a gop/seq NOTE!!! please see
 * functions from  picture.c and time.c for more tmp_ref based functions
 */

/* From tmp_ref.c */

/**
 * get_temporal_reference gets the temporal_reference of a payload_data
 * frame - NOTE NO ERROR CHECKING WHAT SO EVER
 *
 * @params picture_frame the payload_data array that holds the mpeg frame
 * @returns the tmp ref of the frame
 */

uint16_t get_temporal_reference(const uint8_t * picture_frame);

/**
 * set_temporal_reference sets the temporal_reference of a payload_data
 * frame - NOTE NO ERROR CHECKING WHAT SO EVER
 *
 * @params picture_frame the payload_data array that holds the mpeg frame
 */

void set_temporal_reference(uint8_t * picture, uint16_t temp_ref);

/******************************************************************************/

/* From tmp_ref_check.c */

/**
 * check_temporal_reference checks if the seq/gop's tmp_ref integrety and returns the number of
 * errors found.
 *
 * NOTE: IF YOU GET A ERROR IN THIS FUNC ALWAYS DO A check_nr_frames_tmp_ref TO DETERMINE THE
 * TYPE OF TMP REF ERROR (i.e. either tmp ref error or a over/under flow of frames
 *
 * NOTE: YOU MUST DO A fix_seq_header BEFORE YOU CAN CHECK THE TMP REF OTHER WISE THIS FUNCTION
 * WILL FAIL!!!
 *
 * @params seq_pes_holder the pes holding the seq you want to check
 * @returns the number of errors found or -1 on failure
 */

int check_temporal_reference(const pes_holder_t * seq_pes_holder);

/**
 * check_nr_frames_tmp_ref checks the number of frames found in the seq/gop is matching
 * the temporal reference for the GOP.
 *
 * Please see correct_underflow/overflow_in_frames
 *
 * NOTE: YOU MUST DO A fix_seq_header BEFORE YOU CAN CHECK THE TMP REF OTHER WISE THIS FUNCTION
 * WILL FAIL!!!
 *
 * @params seq_pes_holder the pes holding the seq you want to check
 * @returns the number of errors found (positive means a over flow in frames negative is a
 * under flow in frames - frames missing) or -1 on failure
 */

int check_nr_frames_tmp_ref(const pes_holder_t * seq_pes_holder);

/******************************************************************************/

/* From tmp_ref_help.c */

/**
 * get_highest_tmp_ref is returning the higherst temporal reference found in the SEQ/GOP
 * get_highest_tmp_ref_special is basically the same as the normal but you can seek
 * it anywhere in the SEQ/GOP - not as reliable as the normal one but handy.
 *
 * @params seq_pes_holder the seq you want to get the highest_tmp ref from
 * @returns the highest tmp ref found  - 0 is most likely a error
 */

uint16_t get_highest_tmp_ref(const pes_holder_t * seq_pes_holder);
uint16_t get_highest_tmp_ref_special(const pes_holder_t * seq_pes_holder);


/**
 * get_second_highest_tmp_ref is returning the second higherst temporal reference found in the SEQ/GOP
 *
 * @params seq_pes_holder the seq you want to get the second highest_tmp ref from
 * tmp_ref is the highest tmp ref that is present in the seq (use get_highest_tmp_ref to get this value).
 * @returns the second highest tmp ref found  - 0 is most likely a error
 */

uint16_t get_second_highest_tmp_ref(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref);

/**
 * get_time_of_tmp_ref returns the PTS of a specific tmp ref in the seq
 *
 * NOTE ON ANY SA/UK YOU MUST DO A check_fix_p_frame if you want to fetch the time
 * for a P frame!!!
 *
 * @params seq_pes_holder the seq that holds the tmp ref you want the time for,
 * tmp_ref the tmp ref you want the time for
 * @returns the PTS time stamp of the tmp ref - NOTE 0 is a failure since this is
 * more or less impossible to have as a PTS
 */

ticks_t get_time_of_tmp_ref(const pes_holder_t * seq_pes_holder, int tmp_ref );

/**
 * set_time_of_tmp_ref sets the PTS of a specific tmp ref in the seq
 *
 *
 * @params seq_pes_holder the seq that holds the tmp ref you want to set the time for,
 * tmp_ref the tmp ref you want to set the time for, time is the PTS you want to set
 * @returns  0 on failure and 1 on success
 */

int set_time_of_tmp_ref(const pes_holder_t * seq_pes_holder, int tmp_ref, ticks_t time);

/**
 * get_frame_type_of_tmp_ref returns the payload type of a specific tmp ref in the seq
 *
 *
 * @params seq_pes_holder the seq that holds the tmp ref you want the payload type of,
 * tmp_ref the tmp ref you want the payload type of
 *
 * @returns the payload of the tmp ref or UNKNOWN_PAYLOAD on failure
 */

payload_type_t get_frame_type_of_tmp_ref(const pes_holder_t * seq_pes_holder, int tmp_ref);

/**
 * remove_frame removes a frame that has the specified tmp_ref in the seq
 * NOTE: You must use fix_tmp_ref_removing_frame to restore/fix the tmp ref of
 * the seq/gop after the removal
 *
 * @params seq_pes_holder the seq you want to operate on, tmp_ref the tmp ref of the frame
 * you want to remove
 * @returns 1 on success and 0 on failure
 */

int remove_frame(pes_holder_t * seq_pes_holder, uint16_t tmp_ref);

/**
 * add_frame adds a frame to the seq/gop inserting it at the specified tmp_ref in the seq
 *
 * NOTE: You must use fix_tmp_ref_adding_frame to restore/fix the tmp ref of
 * the seq/gop after the adding
 * NOTE: THIS FUNCTION IS DUPLICATING THE SPEC TMP REF !! FIXME NOT REALLY ADDING
 *
 * @params seq_pes_holder the seq you want to operate on, tmp_ref the tmp ref of the frame
 * you want to add
 * @returns 1 on success and 0 on failure
 */

int add_frame(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder, uint16_t tmp_ref);


/**
 * FIXME CODE DUP!!
 * tmp_ref_of_payload returns the tmp ref of the frame present in the payload_data in the payload
 *
 * @params payload the payload you want the tmp ref of
 * @returns the tmp_ref of the payload 
 */

uint16_t tmp_ref_of_payload(const payload_t * payload);


/******************************************************************************/


/* tmp_ref_fix.c */

/**
 * fix_tmp_ref_after_closed_gop is adjusting the temporal reference in the SEQ/GOP
 * when you preform a losed gop operation - it's used by the make_closed_gop function
 *
 * @params seq_pes_holder the pes that you just adjusted to closed gop, nr_b_frames
 * the number of B-frames removed between the first I and P/I frame pair.
 */
void fix_tmp_ref_after_close_gop(const pes_holder_t * seq_pes_holder, uint16_t nr_b_frames);

/**
 * fix_tmp_ref_removing_frame will correct the tmp ref in a seq/gop after you have removed a frame
 *
 * @params pes_holder the pes holder holding the seq/gop that you preformed the remove on, nr_of_removed
 * amount of removed frames NOTE Only one is allowed, tmp_ref_removed the tmp ref of the removed frame
 */

void fix_tmp_ref_removing_frame(const pes_holder_t * pes_holder, uint16_t nr_of_removed, uint16_t temp_ref_removed);

/**
 * fix_tmp_ref_adding_frame will correct the tmp ref in a seq/gop after you have added a frame
 *
 * @params pes_holder the pes holder holding the seq/gop that you preformed the add on, nr_of_added
 * amount of additional frames NOTE Only one is allowed, tmp_ref_added the tmp ref of the addtional frame
 */

void fix_tmp_ref_adding_frame(const pes_holder_t * pes_holder, uint16_t nr_of_added, uint16_t temp_ref_added);

/**
 * fix_tmp_ref will correct a tmp ref error in a seq/pes that has no under or over flow of frames!!
 * NOTE DON'T TRY TO FIX TMP REF UNLESS YOU HAVE THE NUMBER OF FRAMES THAT THE SEQ/GOP IS SUPPOSED TO HAVE
 *
 * @params pes_holder the pes that holds the seq/tmp ref that you want to repair.
 * @returns 1on sucess 0 on failure
 */

int fix_tmp_ref(tystream_holder_t * tystream, pes_holder_t * pes_holder);

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/**
 * This is frame rate convert functions that will be used when we turn a 29.97
 * 3:2 downpulled interlaced move into 23.97 f/s progressive movie - it's was
 * earlier used in a futile attempt to adjust frame rates
 */
#ifdef FRAME_RATE
/* From frame_rate.c */
uint8_t control_frame_rate(const pes_holder_t * pes_holder);
uint8_t actual_frame_rate(ticks_t diff);
int set_frame_rate(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
int fix_frame_rate(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
int fix_frame_rate_remove(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
int fix_frame_rate_add(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
int check_frame_rate_fix_header(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
int check_frame_rate(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);
#endif

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* From sync_drift.c */

/**
 * check_av_drift_in_seq controls the drift in the seq present in seq_pes_holder
 * and adds it to the tystream->drift variable
 *
 * @params seq_pes_holder the pes holder holding the seq/gop you want to control
 * @return -1 on failure 0 on success and 1 and up on minor errors
 */

int check_av_drift_in_seq(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);


/**
 * When the drift is above they tystream->drift_threshold you should mend it with
 * this function - this is the last thing you should do before writing seq to
 * disk or sedning it way for internal remuxing.
 *
 * @params seq_pes_holder the seq that you will mend the drift in
 * @returns always returning 1 FIXME!!
 */
int fix_drift(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/



/* From seq_header_fix.c */

/**
 * fix_seq_header will check you (all) pes holders that holds a SEQ/GOP and move it to the
 * next i-frame
 *
 * @params seq_pes_holder the first seq pes holder that need to be fixed - it it's allready
 * fixed the next seq_holder will be returned and you can then check that one
 * @returns when finished it will return 0 when unfinished next sep pes holder will be returned
 */

pes_holder_t *  fix_seq_header(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder);


/* Internal
static payload_t * find_last_i_frame(const pes_holder_t * pes_holder);
*/

/******************************************************************************/
/* FIXME 1: Hardcoded funcs!! 2: Return values!! */

/* From seq_frame_correction.c */

/**
 * correct_underflow_in_frames will fix a under flow in frames (frames missing)
 *
 * @parmas seq_pes_holder the the seq that has a underflow in frames - nr_missing_frames, last_p_frame_missing currently not used
 * @returns 0 on sucess 1 and up on error and -1 on failure
 */

int correct_underflow_in_frames(tystream_holder_t * tystream, pes_holder_t * seq_pes_holder, int nr_missing_frames, int last_p_frame_missing);

/**
 * correct_overflow_in_frames will fix a overflow flow in frames
 * NOTE Always check for underflow when the overflow is corrected since this is always a missing seq more or less
 *
 * @parmas seq_pes_holder the the seq that has a overflow in frames - nr_of_overflow_frames currently not used
 * @returns 0 on sucess 1 and up on error and -1 on failure
 */

int correct_overflow_in_frames(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, int nr_of_overflow_frames);

/* Internal
static int control_i_frame_present(const pes_holder_t * seq_pes_holder);
static int repair_frame_time(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder, tmp_ref_frame_t * frame_array);
static int remove_dup_frame(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref);
static uint16_t tmp_ref_present(tystream_holder_t * tystream, const pes_holder_t * seq_pes_holder,
	uint16_t tmp_ref, tmp_ref_frame_t * frame_array);
static int is_duplicate(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref);
static pes_holder_t * find_first_frame_missing_seq(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref);
static pes_holder_t * fetch_next_avaliable_frame(const pes_holder_t * seq_pes_holder, uint16_t tmp_ref, payload_type_t frame_type);
*/


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* From cutpoints.c */

/**
 * print_cutpoints - prints all cutpoints 
 */

//void print_cutpoints();

/* Release memory used by the cutpoints
 * Call this to reset the cutpoints and when exiting the program
*/
//void cutpoint_free();

/* Read a cut file produced by jdiner's gopchopper program
 * inputs - full path to file
 * returns - 0 on success, error code on failure
 */
//int cutpoint_read_gopeditor_file( const char *szfilename );

/* Add additional cut points
 * inputs - Time ranges in the format timestring-timestring
 *               timestring can be any one of hh:mm:ss.mmm
 *											  hh:mm:ss
 *											  hh:mm
 *											  mm
 *			Type of cut point
 * returns - 0 on success, error code on failure
 */
//int cutpoint_add_cut( const char *sztimerange, ecut_t cuttype );

/* As above but time range strings already split */
//int cutpoint_add_cut2( const char *szstarttime, const char *szendtime, ecut_t cuttype );

/* Set the start time of the stream in ticks */
//void cutpoint_set_stream_start_time( streamtime_t start );

/* Check if a stream time is inside a remove cutpoint
 * inputs - Current stream time since the stream start
 * returns - 0 if not in a cut point
 *			 The stream time at which output should resume
 */
//streamtime_t cutpoint_is_in_remove_cut( streamtime_t streamtime );

/* TODO Document */

//cutpoint_t * cutpoint_incut_ticks_cutpoint(tystream_holder_t * tystream, int64_t ticks);
int cutpoint_add_cut_to_array (tystream_holder_t * tystream, cutpoint_t * cutpoint );
int64_t cutpoint_incut_ticks_cutpoint(tystream_holder_t * tystream, int64_t ticks);
int cutpoint_incut_ticks(tystream_holder_t * tystream, int64_t ticks);
int cutpoint_incut_chunk_record(tystream_holder_t * tystream, chunk_t * chunk);
//int cutpoint_incut_chunk(tystream_holder_t * tystream, chunk_t * chunk);
int cutpoint_incut_chunk(tystream_holder_t * tystream, chunknumber_t  chunk_number);
ticks_t cutpoint_convert_time_to_ticks( const char *szstartTime);
gop_index_t * cutpoint_find_gop_index(tystream_holder_t * tystream, ticks_t time);
int cut_point_add_ticks(tystream_holder_t * tystream, ticks_t time_start, ticks_t time_end, ecut_t cuttype);
int cutpoint_add_manual_time_cut( tystream_holder_t * tystream, const char *sztime_range, ecut_t cuttype);
int cutpoint_read_gopeditor_file( tystream_holder_t * tystream, const char *szfilename );
int cutpoint_add_cut(tystream_holder_t * tystream, const gop_index_t * start_gop_index,
	const gop_index_t * stop_gop_index, ecut_t cuttype);
int cutpoint_create_from_tyeditor_string( tystream_holder_t * tystream, const char *cutstring );
void cutpoint_free_all(tystream_holder_t * tystream);
cutpoint_t * cutpoint_new();



/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/* From index_edit.c */
/* Under construction :) */

gop_index_t * new_gop_index();
int seq_preset_in_chunk(tystream_holder_t * tystream, chunknumber_t chunk_nr, gop_index_t * gop_index, int i_frame);
void add_gop_index(index_t * index, gop_index_t * gop_index, int64_t gop_number);
void free_index(index_t * index);
index_t * create_index(tystream_holder_t * tystream, int interactive, int *pcprogress, char *szprogress );
int get_first_seq(tystream_holder_t * tystream, gop_index_t * gop_index, module_t * module);
int get_index_time(tystream_holder_t * tystream, gop_index_t * gop_index);
int get_image_and_time(tystream_holder_t * tystream, gop_index_t * gop_index, module_t * module);
module_t * get_image(tystream_holder_t * tystream, gop_index_t * gop_index, module_t * init_image);
module_t * init_image(tystream_holder_t * tystream);
void print_index(tystream_holder_t * tystream);
int set_seq_low_delay(tystream_holder_t * tystream, module_t * module);
index_t * scan_index(tystream_holder_t * tystream, index_t * index);


/* From index_edit_help.c */
void print_gop_index(const gop_index_t * gop_index);
void write_gop_index(int fd, const gop_index_t * gop_index);
void swrite_gop_index(SOCKET_FD sockfd, const gop_index_t * gop_index);

void print_gop_index_list(const index_t * index);
void write_gop_index_list(int fd, const index_t * index);
void swrite_gop_index_list(SOCKET_FD sockfd, const index_t * index);

void count_gop_index_list(index_t * index);

void add_chunk_number_seq(int64_t chunk_number_seq, gop_index_t *  gop_index);
void add_seq_rec_nr(int seq_rec_nr, gop_index_t *  gop_index);
void add_chunk_number_i_frame_pes(int64_t chunk_number__i_frame_pes, gop_index_t *  gop_index);
void add_i_frame_pes_rec_nr(int i_frame_pes_rec_nr, gop_index_t *  gop_index);
void add_chunk_number_i_frame(int64_t chunk_number__i_frame, gop_index_t *  gop_index);
void add_i_frame_rec_nr(int i_frame_rec_nr, gop_index_t *  gop_index);
void add_time_of_iframe(ticks_t time_of_iframe, gop_index_t *  gop_index);

void  parse_gop_index(SOCKET_HANDLE inpipe, index_t * index, int *progress);



/* FRom vstream.c */
void free_module(module_t * module);
size_t tydemux_read_vstream(void * prt, size_t size, size_t nmemb, vstream_t * vstream);
size_t tydemux_write_vstream(const void * ptr,size_t size, size_t nmemb, vstream_t * vstream);
vstream_t * new_vstream();
void free_vstream(vstream_t * vstream);
int tydemux_seek_vstream(vstream_t * vstream, int offset, int whence);
int tydemux_tell_vstream(vstream_t * vstream);



/* Don't use those once */
void tydemux_bits_seek_start(vstream_t * vstream);
uint32_t tydemux_bits_getbyte_noptr(vstream_t * vstream);
uint32_t tydemux_bits_getbit_noptr(vstream_t * vstream);
int tydemux_feof(vstream_t * vstream);
uint32_t tydemux_bits_showbits32_noptr(vstream_t * vstream);
uint32_t tydemux_bits_getbits(vstream_t * vstream, int nr_of_bits);
uint32_t tydemux_bits_showbits(vstream_t * vstream, int nr_of_bits);


/* From tydemux,c */


int parse_args(int argc, char *argv[], tystream_holder_t * tystream);
#if !defined(TIVO)
int tydemux(tystream_holder_t * tystream, demux_start_params_t *params);
#else
int tydemux(tystream_holder_t * tystream);
#endif

#if !defined(TIVO)
int tydemux_thread( void * user_data );
#endif


/* from fsid.c */
fsid_index_t * new_fsid_index();
void free_fsid_index(fsid_index_t * fsid_index);

void print_fsid_index(const fsid_index_t * fsid_index);
void write_fsid_index(int fd, const fsid_index_t * fsid_index);
void swrite_fsid_index(SOCKET_FD sockfd, const fsid_index_t * fsid_index);

void free_fsid_index_list(fsid_index_t * fsid_index_list);

void print_fsid_index_list(const fsid_index_t * fsid_index_list);
void write_fsid_index_list(int fd, const fsid_index_t * fsid_index_list);
void swrite_fsid_index_list(SOCKET_FD sockfd, const fsid_index_t * fsid_index_list);

fsid_index_t * add_fsid_index(fsid_index_t * fsid_index_list, fsid_index_t * fsid_index);

void add_fsid(int fsid, fsid_index_t *  fsid_index);
void add_tystream(char * tystream, fsid_index_t *  fsid_index);
void add_state(int state, fsid_index_t *  fsid_index);
void add_year(char * year,fsid_index_t *  fsid_index);
void add_air(char * air,fsid_index_t *  fsid_index);
void add_day(char * day,fsid_index_t *  fsid_index);
void add_date(char * date,fsid_index_t *  fsid_index);
void add_rectime(char * rectime,fsid_index_t *  fsid_index);
void add_duration(char * duration,fsid_index_t *  fsid_index);
void add_title(char * title,fsid_index_t *  fsid_index);
void add_episode(char * episode,fsid_index_t *  fsid_index);
void add_episodenr(char * episodenr,fsid_index_t *  fsid_index);
void add_description(char * description,fsid_index_t *  fsid_index);
void add_actors(char * actors,fsid_index_t *  fsid_index);
void add_gstar(char * gstar,fsid_index_t *  fsid_index);
void add_host(char * host,fsid_index_t *  fsid_index);
void add_director(char * director,fsid_index_t *  fsid_index);
void add_eprod(char * eprod,fsid_index_t *  fsid_index);
void add_prod(char * prod,fsid_index_t *  fsid_index);
void add_writer(char * writer,fsid_index_t *  fsid_index);
void add_index_avalible(int index,fsid_index_t *  fsid_index);

fsid_index_t * parse_nowshowing_server(SOCKET_HANDLE inpipe);
fsid_index_t * parse_nowshowing_client(SOCKET_HANDLE inpipe);

/* from dir_index.c */
dir_index_t * new_dir_index();
dir_index_t * add_dir_index(dir_index_t * dir_index_list, dir_index_t * dir_index);
void free_dir_index(dir_index_t * dir_index);
void free_dir_index_list(dir_index_t * dir_index_list);
void print_dir_index(dir_index_t * dir_index);
void print_dir_index_list(const dir_index_t * dir_index_list);
void add_filename(dir_index_t * dir_index, char * filename);

/* from tydemux_remote.c */
remote_holder_t * new_remote_holder(char * hostname);
int tydemux_init_remote(remote_holder_t * remote_holder);
tystream_holder_t * tydemux_open_probe_remote(remote_holder_t * remote_holder, int fsid);

int tydemux_refresh_fsid(remote_holder_t * remote_holder);
index_t * tydemux_index_remote(tystream_holder_t * tystream, remote_holder_t * remote_holder, int *pcprogress, char *szProgress );
vstream_t * tydemux_get_remote_chunk(remote_holder_t * remote_holder, tystream_holder_t * tystream, int64_t chunk_number);
vstream_t * tydemux_get_remote_chunk_X(remote_holder_t * remote_holder, tystream_holder_t * tystream, int64_t chunk_number);
int tydemux_get_remote_stream(tystream_holder_t * tystream, int * progress, remote_holder_t * remote_holder, int file);
int tydemux_remote_play_stream(tystream_holder_t * tystream, chunknumber_t chunk_nr);
vstream_t *  tydemux_get_remote_play_chunk(remote_holder_t * remote_holder, tystream_holder_t * tystream);

module_t * tydemux_get_remote_seq(remote_holder_t * remote_holder, tystream_holder_t * tystream);
module_t * tydemux_get_remote_i_frame(remote_holder_t * remote_holder, tystream_holder_t * tystream, int64_t gop_number);

void tydemux_close_remote_tystream(remote_holder_t * remote_holder);
void tydemux_close_tyserver(remote_holder_t * remote_holder);
void free_remote_holder(remote_holder_t * remote_holder);



#ifdef __cplusplus
}
#endif

