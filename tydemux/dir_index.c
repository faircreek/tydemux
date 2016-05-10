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


dir_index_t * new_dir_index() {

	dir_index_t * dir_index;
	dir_index = (dir_index_t *)malloc(sizeof(dir_index_t));
	memset(dir_index, 0,sizeof(dir_index_t));
	dir_index->filename = NULL;
	dir_index->next = NULL;
	dir_index->previous = NULL;
	return(dir_index);
}

dir_index_t * add_dir_index(dir_index_t * dir_index_list, dir_index_t * dir_index) {
	
	dir_index_t * tmp_dir_index;

	if(!dir_index_list) {
		return(dir_index);
	} 

	tmp_dir_index = dir_index_list;

	while(tmp_dir_index->next) {
		tmp_dir_index =tmp_dir_index->next;
	}

	tmp_dir_index->next = dir_index;
	dir_index->previous = tmp_dir_index;

	return(dir_index_list);

}

void free_dir_index(dir_index_t * dir_index) {



	if(dir_index->filename) {
		free(dir_index->filename);
	}

}

void free_dir_index_list(dir_index_t * dir_index_list) {


	dir_index_t * dir_index;

	while(dir_index_list) {
		dir_index = dir_index_list;
		dir_index_list = dir_index_list->next;
		free_dir_index(dir_index);
	}

}

void print_dir_index(dir_index_t * dir_index) {



	if(dir_index->filename) {
		printf("%s\n", dir_index->filename);
	}

}

void print_dir_index_list(const dir_index_t * dir_index_list) {

	dir_index_t * dir_index;
	dir_index = (dir_index_t *)dir_index_list;

	while(dir_index) {
		print_dir_index(dir_index);
		dir_index = dir_index->next;
	}

}

void add_filename(dir_index_t * dir_index, char * filename) {	
	
	int size;

	size = strlen(filename);
	dir_index->filename = (char *)malloc(sizeof(char) * (size +1));
	memset(dir_index->filename, 0,sizeof(char) * (size +1));
	strcpy(dir_index->filename,filename);

}


