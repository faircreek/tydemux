/*
 * playitsam: Play, edit & save a TiVo video stream from file or MFS.
 *
 * ttyserver.c: Tystream server for TiVo only. $Revision: 1.23 $
 *
 * (c) 2002, Warren Toomey wkt@tuhs.org.
 * (c) 2003, 2015 Olaf Beck olaf_sc@yahoo,com
 *
 * Released under the Gnu General Public License version 2
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <ctype.h>

#include "../tydemux/tydemux.h"
#define LOG_MODULE "tyserver"
#include "tylogging.h"
#include "../libs/about.c"
#include "image.h"

#include <sys/wait.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "readfile.h"
#include <string.h>
#include <dirent.h>
#define _STRUCT_TIMESPEC
#include <asm/unistd.h>
#include <linux/sched.h>

/* Why isn't this defined in TiVo Linux? */
int snprintf(char *str, size_t size, const char *format,...);

_syscall3(int, sched_setscheduler, \
		pid_t, pid, int, policy, struct sched_param, *param);

extern int sched_setscheduler(pid_t pid, int policy, struct sched_param *param);

char buf[CHUNK_SIZE];
#define CHUNKX 10
char bufx[CHUNK_SIZE * CHUNKX];
int totalchunks;		/* Total chunks */
int infile = 0;			/* FD of input file */
int chunk_in_buf = -1;		/* # of the chunk in the buffer */
tystream_holder_t * tystream=NULL;
fsid_index_t * fsid_index = NULL;
int fsid = -1;

#define TYSERVER_VERSION "0.5.0"



void get_start_chunk_tivo()
{

	/* interation */
	int i;


	/* Marker */
	int gotit;

	/* Chunks */
	int chunk_lnr;
	chunk_t * chunk;
	vstream_t * vstream;


	chunk_lnr=0;
	gotit=0;
	vstream = new_vstream();
	/* Lets fake it and do a read buffer */
	vstream->start_stream = (uint8_t *)malloc(sizeof(uint8_t) * CHUNK_SIZE);
	vstream->size = sizeof(uint8_t) * CHUNK_SIZE ;
	vstream->current_pos = vstream->start_stream;
	vstream->end_stream = vstream->start_stream + vstream->size;
	tystream->vstream = vstream;


	while(1) {


		vstream->current_pos = vstream->start_stream;
		vstream->eof=0;

		read_a_chunk(infile, chunk_lnr, vstream->start_stream);

		chunk = read_chunk(tystream, (int64_t)chunk_lnr, 1);

		if(chunk != NULL) {
			for(i=0; i < chunk->nr_records; i++){
				if(chunk->record_header[i].type == tystream->right_audio){
					gotit=1;
					break;
				}
			}
			free_chunk(chunk);
		}


		if(!gotit) {
			chunk_lnr++;
		} else {
			break;
		}
	}

	free_vstream(vstream);
	tystream->vstream=NULL;


	tystream->start_chunk = (int64_t)chunk_lnr;

	tystream->number_of_chunks = 0;
}


int get_index_time_tivo(gop_index_t * gop_index)
{

	/* Iteration */
	int i;


	/* The chunk we read */
	chunk_t * chunk = NULL;

	/* The chunk linked list */
	chunk_t * chunks = NULL;

	/* vstream */
	vstream_t * vstream;

	int chunks_to_read;


	chunks_to_read = 2;

	/* Find out how many chunks we need to read
	if pes and i frame is in the same chunk then two chunks if not three chunks
	*/

	if(gop_index->chunk_number_i_frame_pes != gop_index->chunk_number_i_frame) {
		chunks_to_read = 3;
	}

	vstream = new_vstream();
	/* Lets fake it and do a read buffer */
	vstream->start_stream = (uint8_t *)malloc(sizeof(uint8_t) * CHUNK_SIZE);
	vstream->size = sizeof(uint8_t) * CHUNK_SIZE ;
	vstream->current_pos = vstream->start_stream;
	vstream->end_stream = vstream->start_stream + vstream->size;
	tystream->vstream = vstream;

	for(i=0; i < chunks_to_read; i++) {

		vstream->current_pos = vstream->start_stream;
		vstream->eof=0;
		read_a_chunk(infile, gop_index->chunk_number_i_frame_pes + i + tystream->start_chunk, vstream->start_stream);

		chunk = read_chunk(tystream, gop_index->chunk_number_i_frame_pes + i, 1);

		if(chunk) {
			chunks = add_chunk(tystream, chunk, chunks);
		} else {
			chunks_to_read++;
		}
	}

	free_vstream(vstream);
	tystream->vstream=NULL;

	gop_index->time_of_iframe = get_time_Y_video(gop_index->i_frame_pes_rec_nr, chunks, tystream);

	free_junk_chunks(chunks);

	return(1);
}


void create_index_tivo(int sockfd)
{

	char *okmsg = "200 Indexing stream\r\n";
	char *okmsg2 = "200 Still indexing stream\r\n";
	char *okmsg3 = "200 Finished indexing stream\r\n";
	char *okmsg4 = "200 Waring no index cache\r\n";

	char *errmsg = "400 Tystream not set!\r\n";

	/* Iteration */
	uint64_t ul;

	/* Gop number */
	int64_t gop_number;

	/* Markers */
	int reuse;

	/* Results of funcs */
	int result;

	/* Chunk numbering */
	chunknumber_t chunk_nr;

	/* Vstream */
	vstream_t * vstream;

	index_t * index;
	gop_index_t * gop_index;
	gop_index_t * last_gop_index;

	char txtbuf[100];

	FILE * inpipe;

	if(tystream->index) {
		printf("Allready index\n");
		return;
	}

	if(fsid == -1) {
		write(sockfd, errmsg, strlen(errmsg));
		return;
	}

	/* Okay lets see if we have a cached index */
	memset(txtbuf, '\0', 99);
	snprintf(txtbuf, 98,"/var/index/index/%i", fsid);

	inpipe = fopen(txtbuf,"r");

	if(inpipe) {
		write(sockfd, okmsg, strlen(okmsg));

		index = (index_t *)malloc(sizeof(index_t));
		index->gop_index = NULL;
		index->nr_of_gops = 0;
		parse_gop_index(inpipe, index, NULL);
		fclose(inpipe);
		write(sockfd, okmsg3, strlen(okmsg3));

		swrite_gop_index_list(sockfd, index);

		tystream->index = index;
		return;
	}


	write(sockfd, okmsg4, strlen(okmsg4));


	index = (index_t *)malloc(sizeof(index_t));

	index->nr_of_gops = 0;
	index->gop_index = NULL;

	reuse = 0;

	write(sockfd, okmsg, strlen(okmsg));


	vstream = new_vstream();
	/* Lets fake it and do a read buffer */
	vstream->start_stream = (uint8_t *)malloc(sizeof(uint8_t) * CHUNK_SIZE);
	vstream->size = sizeof(uint8_t) * CHUNK_SIZE ;
	vstream->current_pos = vstream->start_stream;
	vstream->end_stream = vstream->start_stream + vstream->size;
	tystream->vstream = vstream;

	gop_number = 1;
	for(ul=tystream->start_chunk, chunk_nr = 0; ul < totalchunks; ul++, chunk_nr++) {

		if(chunk_nr%200 == 0 && chunk_nr > 200) {
			write(sockfd, okmsg2, strlen(okmsg2));
		}

		if(!reuse) {
			gop_index = new_gop_index();
		} else {
			/* reset gop index */
			gop_index->gop_number=0;
			gop_index->chunk_number_seq = -1;
			gop_index->seq_rec_nr = -1;
			gop_index->chunk_number_i_frame_pes = -1;
			gop_index->i_frame_pes_rec_nr = -1;
			gop_index->chunk_number_i_frame = -1;
			gop_index->i_frame_rec_nr = -1;
			gop_index->time_of_iframe = 0;
			gop_index->display_time = 0;
			gop_index->reset_of_pts=0;
		}


		vstream->current_pos = vstream->start_stream;
		vstream->eof=0;

		read_a_chunk(infile, ul, vstream->start_stream);




		result = seq_preset_in_chunk(tystream, chunk_nr, gop_index, 0);

		if(result < 1) {
			reuse = 1;
			continue;
		}

		if(result == 3) {
			/* We got all of them SEQ, PES and I Frame
			add the gop_index to the linked list */
			last_gop_index = gop_index;
			add_gop_index(index, gop_index, gop_number);
			gop_number++;
			reuse = 0;
			continue;
		}

		if(result < 3) {
			vstream->current_pos = vstream->start_stream;
			vstream->eof=0;

			read_a_chunk(infile, ul + 1, vstream->start_stream);



			result = seq_preset_in_chunk(tystream, chunk_nr + 1, gop_index, 1);



			if(result < 1) {
				/* Giving up on this index */

				reuse = 1;
				continue;
			}

			if(gop_index->i_frame_pes_rec_nr != -1 && gop_index->i_frame_rec_nr != -1 ) {
				last_gop_index = gop_index;
				add_gop_index(index, gop_index, gop_number);
				gop_number++;
				reuse = 0;
				continue;
			} else {
				/* Scrap it and start over */
				reuse = 1;
			}
		}
	}

	free_vstream(vstream);
	tystream->vstream=NULL;


	index->last_gop_index = last_gop_index->previous;

	/* We may other wise hang on the last record FIXME Workaround */
	last_gop_index->previous->next = NULL;
	free(last_gop_index);

	gop_index = index->gop_index;
	ul = 0;

	while(gop_index) {
		if(ul%100 == 0 && ul > 100) {
			write(sockfd, okmsg2, strlen(okmsg2));
		}
		get_index_time_tivo(gop_index);
		ul++;
		gop_index = gop_index->next;
	}

	index->nr_of_gops = ul;

	write(sockfd, okmsg3, strlen(okmsg3));

	swrite_gop_index_list(sockfd, index);

	tystream->index = index;

}


void print_probe_tivo(int sockfd)
{

	char  msgbuf[100];
	char * msg1 = "200 Print probe\r\n";
  	char * msg2 = "200 Print probe finshed\r\n";


	char * msg3 = "200 Tivo Type\r\n";
	char * msg4 = "300 2\r\n";
	char * msg5 = "300 5\r\n";
	char * msg6 = "SA\r\n";
	char * msg7 = "DTIVO\r\n";

	char * msg8 = "200 Series\r\n";
	char * msg9 = "300 1\r\n";
	char * msg10 = "1\r\n";
	char * msg11 = "2\r\n";

	char * msg12 = "200 Audio\r\n";
	char * msg13 = "300 3\r\n";
	char * msg14 = "MPG\r\n";
	char * msg15 = "AC3\r\n";

	char * msg16 = "200 Audio Size\r\n";
	char * msg17 = "300 0 \r\n";

	char * msg18 = "200 Audio time\r\n";
	char * msg19 = "300 0 \r\n";

	char * msg20 = "200 Video Framerate\r\n";
	char * msg21 = "300 1\r\n";

	char * msg22 = "200 Number of chunks\r\n";
	char * msg23 = "300 0\r\n";


	write(sockfd, msg1, strlen(msg1));
	if(tystream->tivo_type == SA) {
		write(sockfd, msg3, strlen(msg3));
		write(sockfd, msg4, strlen(msg4));
		write(sockfd, msg6, strlen(msg6));
	} else {
		write(sockfd, msg3, strlen(msg3));
		write(sockfd, msg5, strlen(msg5));
		write(sockfd, msg7, strlen(msg7));
	}

	if(tystream->tivo_series == S2) {
		write(sockfd, msg8, strlen(msg8));
		write(sockfd, msg9, strlen(msg9));
		write(sockfd, msg11, strlen(msg11));
	} else {
		write(sockfd, msg8, strlen(msg8));
		write(sockfd, msg9, strlen(msg9));
		write(sockfd, msg10, strlen(msg10));
	}

	if(tystream->audio_type > DTIVO_AC3) {
		write(sockfd, msg12, strlen(msg12));
		write(sockfd, msg13, strlen(msg13));
		write(sockfd, msg14, strlen(msg14));
	} else {
		write(sockfd, msg12, strlen(msg12));
		write(sockfd, msg13, strlen(msg13));
		write(sockfd, msg15, strlen(msg15));
	}

	write(sockfd, msg16, strlen(msg16));
	write(sockfd, msg17, strlen(msg17));
	snprintf(msgbuf, 98, "%i\r\n", tystream->audio_frame_size);
	write(sockfd, msgbuf, strlen(msgbuf));

	write(sockfd, msg18, strlen(msg18));
	write(sockfd, msg19, strlen(msg19));
	snprintf(msgbuf, 98, "" I64FORMAT "\r\n", tystream->audio_median_tick_diff);
	write(sockfd, msgbuf, strlen(msgbuf));

	write(sockfd, msg20, strlen(msg20));
	write(sockfd, msg21, strlen(msg21));
	snprintf(msgbuf, 98, "%i\r\n", tystream->frame_rate);
	write(sockfd, msgbuf, strlen(msgbuf));

	write(sockfd, msg22, strlen(msg22));
	write(sockfd, msg23, strlen(msg23));
	snprintf(msgbuf, 98, "%i\r\n", totalchunks - (int)tystream->start_chunk);
	write(sockfd, msgbuf, strlen(msgbuf));
	printf("Total chunks is %i\n", totalchunks - (int)tystream->start_chunk);

	write(sockfd, msg2, strlen(msg2));

}



void exit_server(int sockfd)
{

  char *okmsg = "200 See you later\r\n";

  if(fsid_index) {
  	free_fsid_index_list(fsid_index);
	fsid_index = NULL;
  }


  write(sockfd, okmsg, strlen(okmsg));
  close(sockfd);
  exit(0);
}


void close_file(int sockfd) {

	char  msgbuf[100];

	if(tystream) {
		free_tystream(tystream);
		tystream = NULL;
	}

	infile = 0;
	chunk_in_buf = -1;
	totalchunks = 0;

	snprintf(msgbuf, 98, "200 Closed fsid %i\r\n", fsid);
	write(sockfd, msgbuf, strlen(msgbuf));
	fsid = -1;

}


void list_recordings_tydemux(int sockfd)
{
	FILE * inpipe;

	char *errmsg1 = "400 Faild executing /var/index/show.tcl\r\n";
	char *errmsg2 = "400 Currenlty no NowShowing info\r\n";
	char *okmsg   = "200 Warning no chached NowShowing info\r\n";



        inpipe = fopen("/var/index/nowshowing", "r");

        if(fsid_index) {
                free_fsid_index_list(fsid_index);
                fsid_index = NULL;
        }

        if(inpipe) {
                /* Good we can use the cached fsid list */
                fsid_index = parse_nowshowing_server(inpipe);
                fclose(inpipe);

                if(fsid_index) {
                        /* Okay print it to the client */
                        swrite_fsid_index_list(sockfd,fsid_index);
                        return;
                } else {
                        fsid_index = NULL;
                }
        }



	fsid_index = parse_nowshowing_tivo();

	if(fsid_index) {
		/* Okay print it to the client */
		swrite_fsid_index_list(sockfd,fsid_index);
		return;
	} else {
		write(sockfd, errmsg2, strlen(errmsg2));
		fsid_index = NULL;
		return;
	}
#if 0
	/* Hmm no nowshowing info  - create it on the fly */
	write(sockfd, okmsg, strlen(okmsg));

	system("/var/index/show.tcl > /var/index/nowshowing");

	/* Why does popen suck on tivo */
	//inpipe = popen("/var/index/show.tcl", "r");

	inpipe = fopen("/var/index/nowshowing", "r");

	if(!inpipe) {
		write(sockfd, errmsg1, strlen(errmsg1));
		close(sockfd);
		return;
	}

	fsid_index = parse_nowshowing_server(inpipe);
	fclose(inpipe);

	if(fsid_index) {
		/* Okay print it to the client */
		swrite_fsid_index_list(sockfd,fsid_index);
		return;
	}

	write(sockfd, errmsg2, strlen(errmsg2));
	fsid_index = NULL;

	return;
#endif

}



char * get_tystream_name(int fsid, fsid_index_t * fsid_index) {


	while(fsid_index) {
	        //printf("Filenames: %s\n", fsid_index->tystream);
		if(fsid_index->fsid == fsid) {
			return(fsid_index->tystream);
		}
		fsid_index = fsid_index->next;
	}

	return(0);
}




void open_file_tydemux(int sockfd, char *filename)
{

	FILE * inpipe;

	chunknumber_t probe_hz;
	chunknumber_t chunk_lnr;
	chunknumber_t int_chunk_nr;
	vstream_t * vstream;
	chunk_t * chunk;
	char * tystream_name;


	char *errmsg1 = "400 Cannot open requested stream\r\n";
	char *errmsg2 = "400 Cannot open more than one stream at the time\r\n";
	char *errmsg3 = "400 Probe of requested stream faild\r\n";
	char *errmsg4 = "400 Invalid filename\r\n";
	char *errmsg5 = "400 No fsid_index\r\n";

	char * msg1 = "200 Probing stream\r\n";
	char * msg2 = "200 Probing finished\r\n";


	char okbuf[100];

	if(strlen(filename) < 4) {
		write(sockfd, errmsg4, strlen(errmsg4));
		return;
	}

	if(strchr(filename, ',')) {
		write(sockfd, errmsg4, strlen(errmsg4));
		return;
	}

	fsid = atoi(filename);

	if(fsid <= 0) {
		write(sockfd, errmsg4, strlen(errmsg4));
		fsid = -1;
		return;
	}

	/*if(!fsid_index) {
		write(sockfd, errmsg5, strlen(errmsg5));
		return;
	}*/

	if(infile) {
		write(sockfd, errmsg2, strlen(errmsg2));
		return;
	}

        if(fsid_index) {
                free_fsid_index_list(fsid_index);
                fsid_index = NULL;
        }

	inpipe = fopen("/var/index/nowshowing", "r");

	if(inpipe) {

                /* Good we can use the cached fsid list */
                fsid_index = parse_nowshowing_server(inpipe);
                fclose(inpipe);

                if(!fsid_index) {
                        fsid_index = NULL;
			fsid_index = parse_nowshowing_tivo();

			if(!fsid_index) {
				write(sockfd, errmsg2, strlen(errmsg2));
				fsid_index = NULL;
				return;
			}
		}
        } else {

		fsid_index = parse_nowshowing_tivo();

		if(!fsid_index) {
			write(sockfd, errmsg2, strlen(errmsg2));
			fsid_index = NULL;
			return;
		}
	}



	tystream_name = get_tystream_name(fsid, fsid_index);
	//printf("The filname is %s:\n", tystream_name);

	if(!tystream_name) {
		write(sockfd, errmsg4, strlen(errmsg4));
		fsid = -1;
		return;
	}


	infile = open_input(tystream_name, 0);
	if (!infile) {
		write(sockfd, errmsg1, strlen(errmsg1));
		return;

	} /*else {
		snprintf(okbuf, 98, "200 Total chunks: %d \r\n", totalchunks);
		write(sockfd, okbuf, strlen(okbuf));
	}*/

	write(sockfd, msg1, strlen(msg1));

	tystream = new_tystream(DEMUX);

	tystream->tivo_probe = 1;

	probe_hz = totalchunks/10;

	tystream->tivo_probe = 1;
	tystream->tivo_version = V_2X;

	for(chunk_lnr=1, int_chunk_nr=0 ;  chunk_lnr < totalchunks; chunk_lnr = chunk_lnr + probe_hz, int_chunk_nr++ ){


		vstream = new_vstream();
		/* Lets fake it and do a read buffer */
		vstream->start_stream = (uint8_t *)malloc(sizeof(uint8_t) * CHUNK_SIZE);
		vstream->size = sizeof(uint8_t) * CHUNK_SIZE ;
		vstream->current_pos = vstream->start_stream;
		vstream->end_stream = vstream->start_stream + vstream->size;

		read_a_chunk(infile, chunk_lnr, vstream->start_stream);

		tystream->vstream = vstream;

		chunk = read_chunk(tystream, (int)chunk_lnr, 1);

		if(chunk != NULL) {
			tystream->chunks = add_chunk(tystream, chunk, tystream->chunks);
			//fprintf(stderr, "Chunk " I64FORMAT " has %i records\n", int_chunk_nr, chunk->nr_records);
		} else {
			//fprintf(stderr, "Chunk was null\n");
		}

		free_vstream(vstream);
		tystream->vstream=NULL;

	}



	if(!tivo_probe_tystream(tystream)) {
		write(sockfd, errmsg3, strlen(errmsg3));
		infile = 0;
		fsid = -1;
		free_tystream(tystream);
		tystream = NULL;
		return;
	}

	write(sockfd, msg2, strlen(msg2));

	get_start_chunk_tivo();

	snprintf(okbuf, 98, "200 Start chunk: %lld\r\n", tystream->start_chunk);
	write(sockfd, okbuf, strlen(okbuf));


	print_probe_tivo(sockfd);

	tystream->tivo_probe = 0;

	return;
}


void send_chunk_tydemux(int sockfd, int chk)
{
	char *okmsg  = "200 Ok, here is the chunk\r\n";
	char *okmsg2 = "300 131072\r\n";
	char *okmsg3 = "200 Chunk sent\r\n";
	char *errmsg = "400 Chunk not present in stream\r\n";
	char *errmsg2 = "400 Stream not opened\r\n";

	int numleft, numsent;
	unsigned char *bptr;
	int total_send;

	if(!infile) {
		write(sockfd, errmsg2, strlen(errmsg2));
	}

	chk = chk + (int)tystream->start_chunk;

	if(chk > totalchunks) {
		write(sockfd, errmsg, strlen(errmsg));
		return;
	}


	/* Read the chunk if we haven't already cached it! */
	if (chunk_in_buf != chk) {
		read_a_chunk(infile, chk, buf);
		chunk_in_buf = chk;
	}

	write(sockfd, okmsg, strlen(okmsg));
	write(sockfd, okmsg2, strlen(okmsg2));

	/* Write the chunk to the output fd */
	numleft = CHUNK_SIZE;
	bptr = buf;

	total_send = 0;
	while (numleft > 0) {
		//if(numleft < 536) {
			numsent = write(sockfd, bptr, numleft);
		/*} else {
			numsent = write(sockfd, bptr, 536);
		}*/

		//printf("Writing\n");
		//numsent = write(sockfd, bptr, numleft);
		//printf("Wrote %i bytes\n", numsent);
		if (numsent < 1) {
			//printf("Error in send\n");
			close_file(sockfd);
			exit_server(sockfd);
			return;
		}
		//total_send = total_send + numsent;
		//printf("Total send so far %i \n",total_send);
		numleft -= numsent;
		bptr += numsent;
	}

	//fsync(sockfd);
	//printf("Finished writing\n");

	write(sockfd, okmsg3, strlen(okmsg3));
	//fsync(sockfd);

	if(chunk_in_buf + 1 < totalchunks) {
		/* And cache the next chunk */
		read_a_chunk(infile, ++chunk_in_buf, buf);

	}

}



void send_chunk_tydemux_X(int sockfd, int chk)
{
	char *okmsg  = "200 Ok, here is the chunk\r\n";
	char *okmsg2 = "300 1310720\r\n";
	char *okmsg3 = "200 Chunk sent\r\n";
	char *errmsg = "400 Chunk not present in stream\r\n";
	char *errmsg2 = "400 Stream not opened\r\n";
	char *errmsg3 = "400 Failed to read chunks\r\n";

	int numleft, numsent;
	unsigned char *bptr;
	int total_send;

	if(!infile) {
		write(sockfd, errmsg2, strlen(errmsg2));
	}

	chk = chk + (int)tystream->start_chunk;

	if(chk + CHUNKX > totalchunks) {
		write(sockfd, errmsg, strlen(errmsg));
		return;
	}


	/* Read the chunk if we haven't already cached it! */
	if (chunk_in_buf != chk) {
		if(read_a_chunkX(infile, chk, bufx, CHUNKX) != 0) {
			chunk_in_buf = chk;
		} else {
			write(sockfd, errmsg3, strlen(errmsg3));
			return;
		}
	}

	write(sockfd, okmsg, strlen(okmsg));
	write(sockfd, okmsg2, strlen(okmsg2));

	/* Write the chunk to the output fd */
	numleft = CHUNK_SIZE * CHUNKX;
	bptr = bufx;

	total_send = 0;
	while (numleft > 0) {
		//if(numleft < 536) {
			numsent = write(sockfd, bptr, numleft);
		/*} else {
			numsent = write(sockfd, bptr, 536);
		}*/

		//printf("Writing\n");
		//numsent = write(sockfd, bptr, numleft);
		//printf("Wrote %i bytes\n", numsent);
		if (numsent < 1) {
			//printf("Error in send\n");
			close_file(sockfd);
			exit_server(sockfd);
			return;
		}
		//total_send = total_send + numsent;
		//printf("Total send so far %i \n",total_send);
		numleft -= numsent;
		bptr += numsent;
	}

	//fsync(sockfd);
	//printf("Finished writing\n");

	write(sockfd, okmsg3, strlen(okmsg3));
	//fsync(sockfd);

//	if(chunk_in_buf + CHUNKX < totalchunks) {
		/* And cache the next chunk */
	if(chunk_in_buf + CHUNKX  < totalchunks) {
		chunk_in_buf = chunk_in_buf + CHUNKX;
		if(read_a_chunkX(infile, chunk_in_buf, bufx, CHUNKX) == 0) {
			chunk_in_buf = -1;
		}
	}
//	}

}



void get_seq_frame(int sockfd)
{
	char *okmsg  = "200 Ok, here is the SEQ header\r\n";\
	char *okmsg3 = "200 SEQ sent\r\n";
	char *errmsg = "400 Faild to fetch SEQ header\r\n";
	char *errmsg2 = "400 Stream not opened\r\n";
	char *errmsg3 = "400 Stream not indexed\r\n";
	char msgbuf[100];


	int numleft, numsent;
	unsigned char *bptr;

	module_t * module;

	if(!infile) {
		write(sockfd, errmsg2, strlen(errmsg2));
	}

	if(!tystream->index) {
		write(sockfd, errmsg3, strlen(errmsg3));
	}


	module = init_image_tivo();

	if(!module) {
		write(sockfd, errmsg, strlen(errmsg));
		return;
	}

	write(sockfd, okmsg, strlen(okmsg));
	snprintf(msgbuf, 98, "300 %i\r\n", module->buffer_size);
	write(sockfd, msgbuf, strlen(msgbuf));


	/* Write the seq to the output fd */
	numleft = module->buffer_size;

	bptr = module->data_buffer;

	while (numleft > 0) {
		//if(numleft < 512) {
			numsent = write(sockfd, bptr, numleft);
		/*} else {
			numsent = write(sockfd, bptr, 512);
		}*/
		if (numsent < 1) {
			close_file(sockfd);
			exit_server(sockfd);
			free(module->data_buffer);
			free(module);
			return;
		}
		numleft -= numsent;
		bptr += numsent;
	}

	free_module(module);
	write(sockfd, okmsg3, strlen(okmsg3));
}

gop_index_t * get_gop_index(int64_t gop_index_nr) {

	gop_index_t * tmp_gop_index;

	tmp_gop_index = tystream->index->gop_index;

	while(tmp_gop_index) {
		if(tmp_gop_index->gop_number == gop_index_nr) {
			return(tmp_gop_index);
		}
		tmp_gop_index = tmp_gop_index->next;
	}

	return(0);
}



void get_i_frame(int sockfd, int64_t gop_index_nr)
{
	char *okmsg  = "200 Ok, here is the I-Frame \r\n";
	char *okmsg3 = "200 I-Frame sent\r\n";
	char *errmsg = "400 Faild to fetch I-Frame\r\n";
	char *errmsg2 = "400 Stream not opened\r\n";
	char *errmsg3 = "400 Stream not indexed\r\n";
	char msgbuf[100];


	int numleft, numsent;
	unsigned char *bptr;

	gop_index_t * gop_index;

	module_t * module;

	if(!infile) {
		write(sockfd, errmsg2, strlen(errmsg2));
	}

	if(!tystream->index) {
		write(sockfd, errmsg3, strlen(errmsg3));
	}


	gop_index = get_gop_index(gop_index_nr);

	if(!gop_index) {
		write(sockfd, errmsg, strlen(errmsg));
		return;
	}

	module = get_image_tivo(gop_index);

	if(!module) {
		write(sockfd, errmsg, strlen(errmsg));
		return;
	}

	write(sockfd, okmsg, strlen(okmsg));
	snprintf(msgbuf, 98, "300 %i\r\n", module->buffer_size);
	write(sockfd, msgbuf, strlen(msgbuf));


	/* Write the seq to the output fd */
	numleft = module->buffer_size;


	bptr = module->data_buffer;

	while (numleft > 0) {
		numsent = write(sockfd, bptr, numleft);
		if (numsent < 1) {
			close_file(sockfd);
			exit_server(sockfd);
			free(module->data_buffer);
			free(module);
			return;
		}
		numleft -= numsent;
		bptr += numsent;
	}

	free_module(module);
	write(sockfd, okmsg3, strlen(okmsg3));
}


void play_file_tydemux(int sockfd, int chunk_nr)
{
	int chk;
	int numleft, numsent;
	unsigned char *bptr;

	char *errmsg1 = "400 Cannot open requested stream\r\n";
	char *errmsg2 = "400 Stream not opened\r\n";
	char *okmsg =   "200 Ok, here is the stream\r\n";
	char *okmsg2 =  "300 0\r\n";

	if(infile == 0) {
		write(sockfd, errmsg2, strlen(errmsg2));
	}

	write(sockfd, okmsg, strlen(okmsg));
	write(sockfd, okmsg2, strlen(okmsg2));

	for (chk = tystream->start_chunk + chunk_nr; chk < totalchunks - 1; chk++ ) {

		read_a_chunk(infile, chk, buf);

		/* Write the chunk to the output fd */
		numleft = CHUNK_SIZE * CHUNKX;
		bptr = buf;
		while (numleft > 0) {
			numsent = write(sockfd, bptr, numleft);
			if (numsent < 1) exit(1);
			numleft -= numsent;
			bptr += numsent;
		}
	}

	if(tystream) {
		free_tystream(tystream);
		tystream = NULL;
	}

	infile = 0;
	chunk_in_buf = -1;
	totalchunks = 0;

	fsid = -1;

	if(fsid_index) {
		free_fsid_index_list(fsid_index);
		fsid_index = NULL;
	}

	close(sockfd);
	exit(0);
}


void handle_request(int sockfd)
{
#define BUFSIZE 1024

	char *errmsg1 = "400 Unrecognised command\r\n";
	char cmd[BUFSIZE];
	int rval;
	int i;


	/* Read a command from the socket */
	/* Ok, so I should read a whole buffer in one hit, who cares! */
	for (i = 0; i < BUFSIZE - 2; i++) {
		rval = read(sockfd, &cmd[i], 1);
		if ((rval == -1) || (cmd[i] == '\n')) {
			break;
		}
	}

	cmd[++i] = '\0';		/* Null-terminate the command */

	printf("Got command: %s\n", cmd);

	if (!strncmp(cmd, "EXIT", 4)) { exit_server(sockfd); return; }

	/* Walk the command and remove \r and \n */
	for (i = 0; i < BUFSIZE; i++) {
		if ((cmd[i] == '\r') || (cmd[i] == '\n')) { cmd[i] = '\0'; break; }
	}

	/* Tydemux commands */
	if (!strncmp(cmd, "LITY", 4)) { list_recordings_tydemux(sockfd); return; }
	if (!strncmp(cmd, "OPTY", 4)) { open_file_tydemux(sockfd, &cmd[5]); return; }
	if (!strncmp(cmd, "COTY", 4)) { close_file(sockfd); return; }
	if (!strncmp(cmd, "SCTY", 4)) { send_chunk_tydemux(sockfd, atoi(&cmd[5])); return; }
	if (!strncmp(cmd, "SCTX", 4)) { send_chunk_tydemux_X(sockfd, atoi(&cmd[5])); return; }
	if (!strncmp(cmd, "CITY", 4)) { create_index_tivo(sockfd); return; }
	if (!strncmp(cmd, "GITY", 4)) { get_i_frame(sockfd, atoll(&cmd[5])); return; }
	if (!strncmp(cmd, "GSTY", 4)) { get_seq_frame(sockfd); return; }
	if (!strncmp(cmd, "PLTY", 4)) { play_file_tydemux(sockfd, atoi(&cmd[5])); return; }

	write(sockfd, errmsg1, strlen(errmsg1));
	return;
}

void usage(void)
{
	printf("Usage: tyserver\n");
	printf("tyserver "TYSERVER_VERSION" %s", about);
	exit(1);
}


int main(int argc, char *argv[])
{
	int sock, length;
	struct sockaddr_in server;
	int pid, msgsock;
	int nodelay = 1;			/* Turn on TCP_NODELAY */
	int window = 32768;
	//int window = 65536;
	struct sched_param param;
	pid_t pidnr;

	if (argc != 1) usage();

	pidnr = getpid();
	param.sched_priority = 1;
	sched_setscheduler(pidnr,1,&param);


	logger_init( log_info, stdout, 0,0 );

	/* Create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("opening stream socket"); exit(1);
	}

	/* Name socket using wildcards */
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;

	/*
	 * Why 1150? T looks like 1, i looks like 1 V is 5 in roman numerals, o
	 * looks like 0
	 */
	server.sin_port = ntohs(1150);
	if (bind(sock, (struct sockaddr *) & server, sizeof(server))) {
		perror("binding stream socket"); exit(1);
	}

	/* Find out assigned port number and print it out */
	length = sizeof(server);
	if (getsockname(sock, (struct sockaddr *) & server, &length)) {
		perror("getting socket name"); exit(1);
	}

	/* Print out that we have the socket, and become a daemon */
	printf("Socket has port #%d\n", ntohs(server.sin_port));
	daemon(0,0);

	/* Start accepting connections */
	listen(sock, 5);

	while (1) {
		msgsock = accept(sock, 0, 0);
		if (msgsock == -1) {
 			perror("accept");
		}

		//if (nodelay==1) {
			//
		//}

		setsockopt(msgsock, SOL_SOCKET, SO_SNDBUF, &window, sizeof(int));
		setsockopt(msgsock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int));
#define FORK
#ifdef FORK
		pid = fork();
		if (pid == 0) {
			while (1) {
				handle_request(msgsock);
			}
		} else {
			close(msgsock);
			/* Clean up as many children as we can */
			waitpid(-1, &length, WNOHANG);
			waitpid(-1, &length, WNOHANG);
			waitpid(-1, &length, WNOHANG);
			waitpid(-1, &length, WNOHANG);
		}
#else
		while (1) handle_request(msgsock);
#endif
	}

	logger_free();
}




