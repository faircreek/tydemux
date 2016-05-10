
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

#define TYLS_VERSION "0.5.0"



char * parse_args_tyls(int argc, char *argv[]) {

	/* Args*/
	int flags;
	int error = 0;

	/* Switches */
	char * hostname = NULL;

	tygetopt_unlock();

	while ((flags = getopt(argc, argv, "?ht:")) != -1) {
		switch (flags) {
		case 't':
			if(optarg[0]=='-') {
				error++;
			} else {
				hostname = optarg;
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

	if(error) {
		return(0);
	}
	return(hostname);

}

void ls_fsid_index(const fsid_index_t * fsid_index) {


	if(fsid_index->episode) {
		printf("%s\t%s\t%s\t%s\t%s\t%i\n", fsid_index->title, fsid_index->episode, fsid_index->day,
			fsid_index->date, fsid_index->rectime, fsid_index->fsid );
	} else {
		printf("%s\t%s\t%s\t%s\t%s\t%i\n", fsid_index->title, "not a episode", fsid_index->day,
			fsid_index->date, fsid_index->rectime, fsid_index->fsid );
	}

}





void ls_fsid_index_list(const fsid_index_t * fsid_index_list) {

	fsid_index_t * start_fsid_index;

	start_fsid_index = (fsid_index_t *)fsid_index_list;


	while(start_fsid_index) {
		ls_fsid_index(start_fsid_index);
		start_fsid_index = start_fsid_index->next;
	}



}


int main(int argc, char *argv[]) {

	char * hostname = NULL;

	remote_holder_t * remote_holder=NULL;


	hostname = parse_args_tyls(argc, argv);

	if(!hostname) {
		printf("Usage: tyls -t hostname\n");
		printf("tyls "TYLS_VERSION" %s", about);
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

	} else {
		ls_fsid_index_list(remote_holder->fsid_index);
	}

	tydemux_close_tyserver(remote_holder);
	free_remote_holder(remote_holder);
	logger_free();
	exit(0);
}
