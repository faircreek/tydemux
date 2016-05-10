/*
 * Copyright (C) 2003, 2015  Olof <jinxolina@gmail.com>
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <ctype.h>

#define LOG_MODULE "tyindex"
#include "../tydemux/tydemux.h"
#include "tylogging.h"
#include "../libs/about.c"
#include "tyindex.h"
#include "readfile.h"
#include <sys/wait.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#define _STRUCT_TIMESPEC
#include <asm/unistd.h>
#include <linux/sched.h>
#define TYINDEX_VERSION "0.5.0"

extern int snprintf(char *str, size_t size, const char *format,...);

_syscall3(int, sched_setscheduler, \
		pid_t, pid, int, policy, struct sched_param, *param);

extern int sched_setscheduler(pid_t pid, int policy, struct sched_param *param);



char buf[CHUNK_SIZE];
int totalchunks;		/* Total chunks */
int infile = 0;			/* FD of input file */
int chunk_in_buf = -1;		/* # of the chunk in the buffer */




void get_start_chunk_tivo(tystream_holder_t * tystream) {

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
		chunk_in_buf = chunk_lnr;

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


int get_index_time_tivo(tystream_holder_t * tystream, gop_index_t * gop_index) {

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
		chunk_in_buf = gop_index->chunk_number_i_frame_pes + i + tystream->start_chunk;

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


void create_index_tivo(tystream_holder_t * tystream) {


	/* Iteration */
	int64_t ul;

	/* Markers */
	int reuse;
	
	/* Gop number */
	int64_t gop_number;

	/* Results of funcs */
	int result;

	/* Chunk numbering */
	chunknumber_t chunk_nr;

	/* Vstream */
	vstream_t * vstream;

	index_t * index;
	gop_index_t * gop_index;
	gop_index_t * last_gop_index;

	index = (index_t *)malloc(sizeof(index_t));

	index->nr_of_gops = 0;
	index->gop_index = NULL;

	reuse = 0;

	printf("Start Indexing\n");

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
			printf("Still indexing\n");
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


		chunk_in_buf = ul;


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

			chunk_in_buf = ul + 1;


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
			printf("Still indexing\n");
		}
		get_index_time_tivo(tystream, gop_index);
		ul++;
		gop_index = gop_index->next;
	}

	index->nr_of_gops = ul;

	printf("Indexing finished " I64FORMAT " entries in index\n", index->nr_of_gops);

	tystream->index = index;

}


int open_file_tydemux(tystream_holder_t * tystream, char *filename) {

	chunknumber_t probe_hz;
	chunknumber_t chunk_lnr;
	chunknumber_t int_chunk_nr;
	vstream_t * vstream;
	chunk_t * chunk;


	if(infile) {
		printf("Error input file allready open\n");
		return(0);
	}
	//printf("%s:\n", filename);

	infile = open_input(filename, 0);
	if (infile == -1) {
		totalchunks= -1;		/* Total chunks */
		infile = 0;			/* FD of input file */
		chunk_in_buf = -1;

		printf("Error opening file\n");
		return(0);

	}


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
		chunk_in_buf = chunk_lnr;

		if(chunk != NULL) {
			tystream->chunks = add_chunk(tystream, chunk, tystream->chunks);
		}

		free_vstream(vstream);
		tystream->vstream=NULL;

	}



	if(!tivo_probe_tystream(tystream)) {
		totalchunks= -1;		/* Total chunks */
		infile = 0;			/* FD of input file */
		chunk_in_buf = -1;
		return(0);
	}

	get_start_chunk_tivo(tystream);

	tystream->tivo_probe = 0;

	/* We assume that the first chunk to be used is chunk 0 */
	read_a_chunk(infile, (int)tystream->start_chunk, buf);
  	chunk_in_buf = (int)tystream->start_chunk;
  	return(1);
}


void usage(void) {
  printf("Usage: tyindex\n");
  printf("tyindex "TYINDEX_VERSION" %s ", about);
  exit(1);
}

int make_index(fsid_index_t * fsid_index) {


	char txtbuf[100];
	tystream_holder_t * tystream;
	int fd;

	tystream = new_tystream(DEMUX);
	if(!open_file_tydemux(tystream,fsid_index->tystream)) {
		printf("Faild open tystream\n");
		
		memset(txtbuf, '\0', 99);
		snprintf(txtbuf, 98,"/var/index/index/%i-no",fsid_index->fsid);
		printf("Making fsid as unindexable - %i\n",fsid_index->fsid);
		fd = open(txtbuf, O_WRONLY|O_CREAT|O_TRUNC|OS_FLAGS, READWRITE_PERMISSIONS);
		free_tystream(tystream);
		return(1);
	}

	create_index_tivo(tystream);


	memset(txtbuf, '\0', 99);
	snprintf(txtbuf, 98,"/var/index/index/%i",fsid_index->fsid);
	printf("Making index file %i\n",fsid_index->fsid);
	fd = open(txtbuf, O_WRONLY|O_CREAT|O_TRUNC|OS_FLAGS, READWRITE_PERMISSIONS);

	if(fd == -1) {
		free_tystream(tystream);
		return(1);
	}

	write_gop_index_list(fd, tystream->index);


	close(fd);
	free_tystream(tystream);
	totalchunks= -1;		/* Total chunks */
	infile = 0;			/* FD of input file */
	chunk_in_buf = -1;

	return(0);

}

dir_index_t * remove_old_files(fsid_index_t * fsid_index) {

	char txtbuf[100];


	DIR *directory_pointer;

	struct dirent *entry;
	dir_index_t * dir_index_list = NULL;
	dir_index_t * dir_index;

	if ((directory_pointer = opendir("/var/index/index")) == NULL) {
		return(0);
	}


	while ((entry = readdir(directory_pointer))) {
		if(strncmp(entry->d_name, ".", 1) != 0) {
			dir_index = new_dir_index();
			add_filename(dir_index, entry->d_name);
			dir_index_list = add_dir_index(dir_index_list, dir_index);
		}
	}

	closedir(directory_pointer);

	dir_index = dir_index_list;

	while(dir_index) {
		if(!file_present(dir_index, fsid_index)) {
			printf("Removing old index /var/index/index/%s\n",dir_index->filename);
			memset(txtbuf, '\0', 99);
			snprintf(txtbuf, 98,"/var/index/index/%s",dir_index->filename );
			if( unlink(txtbuf) == -1) {
				/* Okay this may be a fsid-no index file */
				printf("Removing old no index /var/index/index/%s-no\n",dir_index->filename);
				memset(txtbuf, '\0', 99);
				snprintf(txtbuf, 98,"/var/index/index/%s-no",dir_index->filename );
				unlink(txtbuf);
			}
		}
		dir_index = dir_index->next;
	}

	return(dir_index_list);

}



int count_files() {

	int nr_of_files;

	DIR *directory_pointer;

	struct dirent *entry;

	if ((directory_pointer = opendir("/var/index/index")) == NULL) {
		return(-1);
	}

	nr_of_files = 0;
	while ((entry = readdir(directory_pointer))) {
		if(strncmp(entry->d_name, ".", 1) != 0) {
			nr_of_files++;
		}
	}

	closedir(directory_pointer);


	return(nr_of_files);

}




int file_present(dir_index_t * dir_index, fsid_index_t * fsid_index) {


	char txtbuf[100];
	char * filename;
	int gotit;


	filename = dir_index->filename;

	gotit = 0;
	while(fsid_index) {
		memset(txtbuf, '\0', 99);
		snprintf(txtbuf, 98,"%i",fsid_index->fsid);

		if(strncmp(txtbuf,filename, 7) == 0) {
			gotit = 1;
			break;
		}
		fsid_index = fsid_index->next;
	}

	if(!gotit) {
		return(0);
	}
	return(1);
}



int create_index_files(fsid_index_t * fsid_index,dir_index_t * dir_index, int files) {

	fsid_index_t * tmp_fsid_index;
	int error;

	tmp_fsid_index = fsid_index;

	if(!files) {
		files = 1;
	} else {
		files = 0;
	}

	error = 0;
	while(tmp_fsid_index) {
		if((tmp_fsid_index->state && !index_present(tmp_fsid_index, dir_index)) ||
			(tmp_fsid_index->state && files)) {
			printf("Need to make %i index \n", tmp_fsid_index->fsid);
			error = make_index(tmp_fsid_index);
		}
		tmp_fsid_index = tmp_fsid_index->next;
	}


	if(error) {
		return(0);
	} else {
		return(1);
	}

}


int index_present(fsid_index_t * fsid_index, dir_index_t * dir_index) {

	char filename[100];
	int gotit;

	memset(filename, '\0', 99);
	snprintf(filename, 98,"%i",fsid_index->fsid);

	if(!dir_index) {
		return(0);
	}

	gotit = 0;
	while(dir_index) {
		if(strncmp(dir_index->filename,filename, 7) == 0) {
			gotit = 1;
			break;
		}
		dir_index = dir_index->next;
	}

	if(!gotit) {
		return(0);
	}
	return(1);
}





int write_now_showing(fsid_index_t * fsid_index) {

	int fd;

	fd = open("/var/index/nowshowing", O_WRONLY|O_CREAT|O_TRUNC|OS_FLAGS, READWRITE_PERMISSIONS);

	if(fd == -1) {
		return(0);
	}

	write_fsid_index_list(fd, fsid_index);
	close(fd);

	return(1);
}



int main(int argc, char *argv[]) {

	FILE * inpipe;

	fsid_index_t * fsid_index;
	dir_index_t * dir_index;
	int files;
	int fd;
	pid_t pid;
	int policy;
	char txtbuf[100];
	struct sched_param param;

	if (argc != 1) usage();


	pid = getpid();
	param.sched_priority = 1;
	sched_setscheduler(pid,1,&param);

	logger_init( log_info, stdout, 0,0 );



	daemon(0,0);

	/* Start indexing */

	while (1) {

		fsid_index = NULL;
		dir_index = NULL;

		fsid_index = parse_nowshowing_tivo();

		if(!fsid_index) {
			printf("Warning no FSID index\n");
			sleep(90);
			continue;
		}

		fd = open("/var/index/nowshowing", O_WRONLY|O_CREAT|O_TRUNC|OS_FLAGS, READWRITE_PERMISSIONS);
		if(fd == -1) {
			LOG_ERROR("Unable to write /var/index/nowshowing");
			exit(1);
		}

		write_fsid_index_list(fd, fsid_index);
		close(fd);

		dir_index = remove_old_files(fsid_index);

		files = 1;
		if(!dir_index) {
			/* Okat there might not be any files */
			if( (files = count_files()) != 0) {
				printf("No dir index\n");
				exit(1);
			}
		}

		if((create_index_files(fsid_index, dir_index, files)) == 0) {
			//free_fsid_index_list(fsid_index);
			//free_dir_index_list(dir_index);
			printf("Error indexing one or more recordings\n"
				"It may be that it got deleted during the indexing\n");
			//exit(1);
		}


		//write_now_showing(fsid_index);

		free_fsid_index_list(fsid_index);

		if(dir_index) {
			free_dir_index_list(dir_index);
		}

		/* Sleep for 15 min */
		sleep(900);
	}

	logger_free();
	return(0);

}
