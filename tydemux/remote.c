
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

int main() {

	int video_file, i;
	tystream_holder_t * tystream = NULL;

	remote_holder_t * remote_holder=NULL;

	vstream_t * vstream = NULL;


	remote_holder = new_remote_holder("tivo");



	logger_init( log_info, stdout, 0, 0 );



	if(!tydemux_init_remote(remote_holder)){
		free_remote_holder(remote_holder);
		printf("Error connecting!!\n");
		return(0);
	}

	if(!remote_holder->fsid_index) {
		tydemux_close_tyserver(remote_holder);
		free_remote_holder(remote_holder);
		printf("No recordings avalible\n");
		return(0);

	} else {
		print_fsid_index_list(remote_holder->fsid_index);
	}

	tystream = tydemux_open_probe_remote(remote_holder, 1186850);

	if(tystream) {
		print_probe(tystream);
	} else {
		tydemux_close_remote_tystream(remote_holder);
		tydemux_close_tyserver(remote_holder);
		free_remote_holder(remote_holder);
		printf("Error Opening remote tystream\n");
		return(0);
	}

	/*
	printf("Indexing\n");

	if(!tydemux_index_remote(remote_holder, NULL)) {
		tydemux_close_remote_tystream(remote_holder);
		tydemux_close_tyserver(remote_holder);
		free_remote_holder(remote_holder);
		printf("Error indexing remote tystream\n");
		return(0);
	}

	printf("Indexing finished\n");
	*/

	video_file = open("test.ty", O_WRONLY|O_CREAT|O_TRUNC|OS_FLAGS, READWRITE_PERMISSIONS);

	/* okay lets test to get one chunk */
	for(i = 5; i < 300; i++) {
		vstream = tydemux_get_remote_chunk(remote_holder, tystream, (int64_t)i);
		if(!vstream) {
			printf("Error fetching remote chunk\n");
			free_vstream(vstream);
			vstream = NULL;
#ifdef WIN32
			assert( tystream->vstream != 0xdddddddd );
#endif
			continue;
		}
		printf("Fetched remote chunk %i \n", i);
		write(video_file, vstream->start_stream, vstream->size);
		free_vstream(vstream);
		vstream = NULL;
#ifdef WIN32
		assert( tystream->vstream != 0xdddddddd );
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
