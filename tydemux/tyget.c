
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

#include "common.h"
#include "global.h"
#include "../libs/about.c"
#include <tygetopt.h>

#define TYGET_VERSION "0.5.0"


int fsid=0;
char * outfile = NULL;

char * parse_args_tyls(int argc, char *argv[]) {

	/* Args*/
	int flags;
	int error = 0;

	/* Switches */
	char * hostname = NULL;


	tygetopt_lock();

	while ((flags = getopt(argc, argv, "?ht:o:f:")) != -1) {
		switch (flags) {
		case 't':
			if(optarg[0]=='-') {
				error++;
			} else {
				hostname = optarg;
			}
			break;
		case 'o':
			if(optarg[0]=='-') {
				error++;
			} else {
				outfile = optarg;
			}
			break;
		case 'f':
			if(optarg[0]=='-') {
				error++;
			} else {
				fsid = atoi(optarg);
			}
			break;
		case '?':
			error++;
			break;
		case 'h':
			error++;
			break;
		default:
			error++;
		}
	}

	tygetopt_unlock();

	if(!outfile) {
		return(0);
	}

	if(!fsid) {
		return(0);
	}

	if(error) {
		return(0);
	}

	return(hostname);

}



void tyget_prog(int64_t i) {


		if (i != 0) {

			if(i%100 == 0 ) {
				printf("%6lld", i);
				if(i%500 == 0 ) {
					printf("\n");
				}

			} else if ( i%10 == 0 ) {
				printf(".");
			}
		}
		fflush(stdout);

}


int main(int argc, char *argv[]) {

	int64_t i;
	int video_file;
	char * hostname;
	tystream_holder_t * tystream = NULL;

	remote_holder_t * remote_holder=NULL;

	vstream_t * vstream = NULL;

	hostname = parse_args_tyls(argc, argv);

	if(!hostname) {
		printf("Usage: tyget -t hostname -o outfile -f fsid\n");
		printf("tyget "TYGET_VERSION" %s", about);
		exit(1);
	}

	remote_holder = new_remote_holder(hostname);

	logger_init( log_info, stdout, 0, 0 );

	if(!tydemux_init_remote(remote_holder)){
		free_remote_holder(remote_holder);
		printf("Error connecting!!\n");
		return(1);
	}

	if(!remote_holder->fsid_index) {
		tydemux_close_tyserver(remote_holder);
		free_remote_holder(remote_holder);
		printf("No recordings avalible\n");
		return(1);

	} 

	tystream = tydemux_open_probe_remote(remote_holder, fsid);

	if(!tystream) {
		tydemux_close_remote_tystream(remote_holder);
		tydemux_close_tyserver(remote_holder);
		free_remote_holder(remote_holder);
		printf("Error Opening remote tystream\n");
		return(1);
	}


	video_file = open(outfile, O_WRONLY|O_CREAT|O_TRUNC|OS_FLAGS, READWRITE_PERMISSIONS);

	if(video_file == -1) {
		printf("Error opening %s\n", outfile);
		return(1);
	}

	printf("Fetching " I64FORMAT " chunks from fsid %i present on %s\n", tystream->nr_of_remote_chunks, fsid, hostname);

	//tydemux_get_remote_stream(tystream, NULL, remote_holder, video_file);


 	/* okay lets test to get one chunk */
	for(i = 0; i < tystream->nr_of_remote_chunks - 10; i = i + 10) {

		tyget_prog(i);

		vstream = tydemux_get_remote_chunk_X(remote_holder, tystream, i);
		if(!vstream) {
			printf("Error fetching remote chunk\n");
			free_vstream(vstream);
			vstream = NULL;
#ifdef WIN32
			assert( tystream->vstream != (vstream_t *) 0xdddddddd );
#endif
			continue;
		}

		write(video_file, vstream->start_stream, (unsigned int)vstream->size);
		free_vstream(vstream);
		vstream = NULL;
#ifdef WIN32
		assert( tystream->vstream != (vstream_t *) 0xdddddddd );
#endif
	}
	close(video_file);

	tydemux_close_remote_tystream(remote_holder);
	tydemux_close_tyserver(remote_holder);
	free_remote_holder(remote_holder);
	free_tystream(tystream);
	logger_free();
	exit(0);
}
